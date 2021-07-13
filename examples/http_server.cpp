#include <arpa/inet.h>
#include <error.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/trace/propagation/detail/context.h>
#include <splunk/opentelemetry.h>
#include <string>

#include "http_common.h"

/*
 * A minimal server to be used with the http_client example to demonstrate
 * extracting parent context from request headers.
 */

namespace nostd = opentelemetry::nostd;

const int kPort = 9123;
const char* kHost = "127.0.0.1";

std::string ParsePropagationHeader(const std::string& request) {
  const std::string prefix = "traceparent: ";
  /* Finds the following header with structure:
   * traceparent: 00-5e086555d0afe8b4b5c2a65430d4e78a-32e23db8a8133506-01
   */
  size_t headerBegin = request.find(prefix);

  if (headerBegin == std::string::npos) {
    return {};
  }

  size_t headerEnd = request.find("\r\n", headerBegin);
  size_t valueBegin = headerBegin + prefix.size();
  return std::string(&request[valueBegin], headerEnd - valueBegin);
}

struct HeaderCarrier : opentelemetry::context::propagation::TextMapCarrier {
  nostd::string_view Get(nostd::string_view key) const noexcept override {
    if (key == "traceparent") {
      return traceparent;
    }

    return nostd::string_view();
  }

  void Set(nostd::string_view key, nostd::string_view value) noexcept override {
    /* Not used in the server example */
  }

  std::string traceparent;
};

int main(int argc, char** argv) {
  /* Initialize OpenTelemetry */
  splunk::OpenTelemetryOptions otelOptions =
    splunk::OpenTelemetryOptions().WithServiceName("my-service");
  splunk::InitOpentelemetry(otelOptions);

  /* Server setup */
  int sfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sfd == -1) {
    perror("socket create:");
    return 1;
  }

  {
    int enable = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  }

  struct sockaddr_in serverAddr = {0};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(kPort);
  serverAddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sfd, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr)) == -1) {
    perror("bind:");
    return 1;
  }

  if (listen(sfd, 128) == -1) {
    perror("listen:");
    return 1;
  }

  for (;;) {
    struct sockaddr_in clientAddr = {0};
    socklen_t clientAddrLength = 0;
    int clientFd = accept(sfd, (struct sockaddr*)&clientAddr, &clientAddrLength);

    if (clientFd == -1) {
      perror("accept:");
      return 1;
    }

    char requestBuffer[512] = {0};
    ssize_t bytesRead = read(clientFd, requestBuffer, sizeof(requestBuffer));

    if (bytesRead <= 0) {
      close(clientFd);
    }

    /* Parse the parent span context */
    HeaderCarrier carrier;
    carrier.traceparent = ParsePropagationHeader(std::string(requestBuffer, bytesRead));

    auto currentContext = opentelemetry::context::RuntimeContext::GetCurrent();
    auto parentContext =
      opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator()->Extract(
        carrier, currentContext);

    /* Start the server span */
    auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("server-tracer");

    opentelemetry::trace::StartSpanOptions startOptions;
    startOptions.kind = opentelemetry::trace::SpanKind::kServer;

    auto parentSpan = opentelemetry::trace::propagation::GetSpan(parentContext);

    startOptions.parent = parentSpan->GetContext();

    // Note: method, status not parsed.
    auto span = tracer->StartSpan(
      "server-operation",
      {{"http.method", "GET"},
       {"http.target", "/"},
       {"http.status_code", 200},
       {"http.host", "localhost"}},
      startOptions);

    const std::string response = "HTTP/1.1 200 OK\r\n\r\n";

    SocketWrite(clientFd, response.data(), response.size());

    span->End();
  }

  return 0;
}
