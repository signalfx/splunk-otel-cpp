#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <splunk/opentelemetry.h>

#include "../common/verify.h"

namespace sdktrace = opentelemetry::sdk::trace;

int main(int argc, char** argv) {
  if (argc < 2) {
    return 1;
  }

  auto verification = VerifyBegin(argv[1]);

  splunk::OpenTelemetryOptions otelOptions = splunk::OpenTelemetryOptions()
                                               .WithServiceName("foo-service")
                                               .WithServiceVersion("1.42")
                                               .WithDeploymentEnvironment("test");
  auto provider = splunk::InitOpentelemetry(otelOptions);
  auto tracer = provider->GetTracer("sample");

  opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> parentSpan =
    tracer->StartSpan("parent-op");

  opentelemetry::trace::StartSpanOptions startOptions;
  startOptions.kind = opentelemetry::trace::SpanKind::kServer;
  startOptions.parent = parentSpan->GetContext();

  opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span =
    tracer->StartSpan("child-op", {}, startOptions);

  span->End();
  parentSpan->End();

  auto sdkProvider = dynamic_cast<sdktrace::TracerProvider*>(provider.get());
  sdkProvider->ForceFlush(std::chrono::seconds(1));

  verification.resource = &sdkProvider->GetResource();
  verification.spans = {span.get(), parentSpan.get()};

  VerifyTraces(verification);

  return 0;
}
