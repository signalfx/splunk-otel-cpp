#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <splunk/opentelemetry.h>

#include "../common/verify.h"

namespace sdktrace = opentelemetry::sdk::trace;

int main(int argc, char** argv) {
  if (argc < 2) {
    return 1;
  }

  auto verification = VerifyBegin(argv[1]);

  splunk::OpenTelemetryOptions otelOptions;
  auto provider = splunk::InitOpentelemetry(otelOptions);
  auto tracer = provider->GetTracer("empty");

  opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span =
    tracer->StartSpan("empty-config-span");

  span->End();

  auto sdkProvider = dynamic_cast<sdktrace::TracerProvider*>(provider.get());
  sdkProvider->ForceFlush(std::chrono::seconds(1));

  verification.resource = &sdkProvider->GetResource();
  verification.spans = {span.get()};

  VerifyTraces(verification);

  return 0;
}
