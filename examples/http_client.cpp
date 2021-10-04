#include <arpa/inet.h>
#include <error.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include <splunk/opentelemetry.h>
#include <opentelemetry/context/propagation/global_propagator.h>
#include <string>

#include "http_common.h"

namespace nostd = opentelemetry::nostd;

const int kPort = 9123;
const char* kHost = "127.0.0.1";

struct HeaderCarrier : opentelemetry::context::propagation::TextMapCarrier {
  nostd::string_view Get(nostd::string_view key) const noexcept override {
    /* Unused, the client only injects propagation headers. */
    return nostd::string_view();
  }

  void Set(nostd::string_view key, nostd::string_view value) noexcept override {
    openTelemetryHeaders.insert(openTelemetryHeaders.end(), key.begin(), key.end());
    openTelemetryHeaders += ": ";
    openTelemetryHeaders.insert(openTelemetryHeaders.end(), value.begin(), value.end());
    openTelemetryHeaders += "\r\n";
  }

  std::string openTelemetryHeaders;
};

int main(int argc, char** argv) {
  /* Initialize OpenTelemetry */
  splunk::OpenTelemetryOptions otelOptions =
    splunk::OpenTelemetryOptions().WithServiceName("my-service");
  splunk::InitOpentelemetry(otelOptions);

  /* Start the span */
  auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("simple");

  opentelemetry::trace::StartSpanOptions startOptions;
  startOptions.kind = opentelemetry::trace::SpanKind::kClient;

  opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span =
    tracer->StartSpan("example-request", startOptions);

  span->SetAttribute("http.method", "GET");
  span->SetAttribute("http.host", kHost);
  span->SetAttribute("http.flavor", "1.1");

  auto scope = tracer->WithActiveSpan(span);
  auto currentContext = opentelemetry::context::RuntimeContext::GetCurrent();

  /* Inject propagation headers */
  HeaderCarrier carrier;
  opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator()->Inject(
    carrier, currentContext);

  /* Do the HTTP request */
  int sfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sfd == -1) {
    span->SetStatus(opentelemetry::trace::StatusCode::kError);
    span->End();
    perror("socket create:");
    return 1;
  }

  struct sockaddr_in serverAddr = {0};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(kPort);
  inet_aton(kHost, &serverAddr.sin_addr);

  if (connect(sfd, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr)) == -1) {
    span->SetStatus(opentelemetry::trace::StatusCode::kError);
    span->End();
    perror("connect:");
    return 1;
  }

  const char* requestFormat = "GET / HTTP/1.1\r\n"
                              "Host: %s:%d\r\n"
                              "%s\r\n"
                              "\r\n";

  char request[512] = {0};
  int requestLength = snprintf(
    request, sizeof(request), requestFormat, kHost, kPort, carrier.openTelemetryHeaders.c_str());

  SocketWrite(sfd, request, requestLength);

  char response[512] = {0};
  ssize_t responseLength = read(sfd, response, sizeof(response));

  if (responseLength == -1) {
    span->SetStatus(opentelemetry::trace::StatusCode::kError);
    span->End();
    return 1;
  }

  /* Simplified, assume we always get HTTP 200 */
  span->SetAttribute("http.status_code", 200);
  span->End();

  printf("response:\n%.*s\n", (int)responseLength, response);

  return 0;
}
