#include <splunk/opentelemetry.h>

#include <thread>

int main(int argc, char** argv) {
  splunk::OpenTelemetryOptions otelOptions = splunk::OpenTelemetryOptions()
                                               .WithServiceName("my-service")
                                               .WithServiceVersion("1.0")
                                               .WithDeploymentEnvironment("test");
  auto provider = splunk::InitOpentelemetry(otelOptions);

  auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("simple");

  opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> parentSpan =
    tracer->StartSpan("parent-op");

  opentelemetry::trace::StartSpanOptions startOptions;
  startOptions.kind = opentelemetry::trace::SpanKind::kServer;
  startOptions.parent = parentSpan->GetContext();

  opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span = tracer->StartSpan(
    "child-op",
    {
      {"my.attribute", "123"},
    },
    startOptions);

  // Simulate work
  std::this_thread::sleep_for(std::chrono::milliseconds(123));

  return 0;
}
