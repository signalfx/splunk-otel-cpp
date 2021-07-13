#include <splunk/opentelemetry.h>

#include <opentelemetry/baggage/propagation/baggage_propagator.h>
#include <opentelemetry/context/propagation/composite_propagator.h>
#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/propagation/b3_propagator.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>

#include <cctype>
#include <cstdlib>
#include <string>

namespace sdktrace = opentelemetry::sdk::trace;
namespace sdkresource = opentelemetry::sdk::resource;
namespace nostd = opentelemetry::nostd;

namespace splunk {

std::string ToLower(std::string v) {
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::tolower(v[i]);
  }

  return v;
}

std::string Trim(const std::string& v) {
  std::string out;

  for (char c : v) {
    if (!std::isblank(c)) {
      out.append(1, c);
    }
  }

  return out;
}

std::vector<nostd::string_view> Split(nostd::string_view s, char separator) {
  std::vector<nostd::string_view> results;
  size_t tokenStart = 0;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] != separator) {
      continue;
    }

    results.push_back(s.substr(tokenStart, i - tokenStart));
    tokenStart = i + 1;
  }

  results.push_back(s.substr(tokenStart));

  return results;
}

std::string GetEnv(const std::string& key, const std::string& defaultVal = "") {
  const char* envVal = std::getenv(key.c_str());

  if (envVal == nullptr) {
    return defaultVal;
  }

  return ToLower(Trim(std::string(envVal)));
}

std::unique_ptr<sdktrace::SpanExporter> CreateOtlpExporter(const OpenTelemetryOptions& sdkOptions) {
  std::string protocol = GetEnv("OTEL_EXPORTER_OTLP_PROTOCOL", "grpc");

  if (protocol == "http/json") {
    auto endpoint = GetEnv("OTEL_EXPORTER_OTLP_ENDPOINT", "http://localhost:4317/v1/traces");

    opentelemetry::exporter::otlp::OtlpHttpExporterOptions exporterOptions;
    exporterOptions.url = endpoint;
    exporterOptions.content_type = opentelemetry::exporter::otlp::HttpRequestContentType::kJson;

    return std::unique_ptr<sdktrace::SpanExporter>(
      new opentelemetry::exporter::otlp::OtlpHttpExporter(exporterOptions));
  }

  if (protocol == "http/protobuf") {
    auto endpoint = GetEnv("OTEL_EXPORTER_OTLP_ENDPOINT", "http://localhost:4317/v1/traces");

    opentelemetry::exporter::otlp::OtlpHttpExporterOptions exporterOptions;
    exporterOptions.url = endpoint;
    exporterOptions.content_type = opentelemetry::exporter::otlp::HttpRequestContentType::kBinary;

    return std::unique_ptr<sdktrace::SpanExporter>(
      new opentelemetry::exporter::otlp::OtlpHttpExporter(exporterOptions));
  }

  // Default to grpc
  auto endpoint = sdkOptions.otlpEndpoint.empty()
                    ? GetEnv("OTEL_EXPORTER_OTLP_ENDPOINT", "localhost:4317")
                    : sdkOptions.otlpEndpoint;

  opentelemetry::exporter::otlp::OtlpGrpcExporterOptions exporterOptions;
  exporterOptions.endpoint = endpoint;

  return std::unique_ptr<sdktrace::SpanExporter>(
    new opentelemetry::exporter::otlp::OtlpGrpcExporter(exporterOptions));
}

std::unique_ptr<sdktrace::SpanExporter> CreateExporter(const OpenTelemetryOptions& options) {
  auto exporterType = GetEnv("OTEL_TRACES_EXPORTER", "otlp");

  if (exporterType == "otlp") {
    return CreateOtlpExporter(options);
  }

  // TODO: jaeger-thrift-splunk once OpenTelemetry CPP's Jaeger exporter supports HTTP.

  return CreateOtlpExporter(options);
}

sdkresource::Resource GetEnvResource(const sdkresource::ResourceAttributes& options) {
  auto rawAttribs = GetEnv("OTEL_RESOURCE_ATTRIBUTES", "");

  sdkresource::ResourceAttributes attributes;

  // Workaround: creating a resource with empty attributes sets the service.name to unknown_service
  // which will overwrite the service name given in options.
  if (options.count("service.name")) {
    auto serviceName = options.at("service.name");
    if (nostd::holds_alternative<std::string>(serviceName)) {
      attributes.SetAttribute("service.name", nostd::get<std::string>(serviceName));
    }
  } else {
    auto envServiceName = GetEnv("OTEL_SERVICE_NAME", "");

    if (!envServiceName.empty()) {
      attributes.SetAttribute("service.name", envServiceName);
    }
  }

  if (rawAttribs.empty()) {
    return sdkresource::Resource::Create(attributes);
  }

  std::vector<nostd::string_view> tokens = Split(rawAttribs, ',');

  for (nostd::string_view token : tokens) {
    std::vector<nostd::string_view> parts = Split(token, '=');

    if (parts.size() == 2) {
      attributes.SetAttribute(parts[0], parts[1]);
    }
  }

  return sdkresource::Resource::Create(attributes);
}

PropagatorFlags EnvPropagatorFlags() {
  auto envPropagators = GetEnv("OTEL_PROPAGATORS", "tracecontext,baggage");

  std::vector<nostd::string_view> propagators = Split(envPropagators, ',');

  PropagatorFlags flags = Propagator_None;

  for (nostd::string_view propagator : propagators) {
    if (propagator == "tracecontext") {
      flags |= Propagator_TraceContext;
    } else if (propagator == "b3") {
      flags |= Propagator_B3;
    } else if (propagator == "b3multi") {
      flags |= Propagator_B3Multi;
    } else if (propagator == "baggage") {
      flags |= Propagator_Baggage;
    }
  }

  return flags;
}

void SetupPropagators(PropagatorFlags flags) {
  namespace contextprop = opentelemetry::context::propagation;
  namespace traceprop = opentelemetry::trace::propagation;
  namespace baggageprop = opentelemetry::baggage::propagation;

  std::vector<std::unique_ptr<contextprop::TextMapPropagator>> propagators;

  flags = flags | EnvPropagatorFlags();

  if (flags & Propagator_TraceContext) {
    propagators.emplace_back(new traceprop::HttpTraceContext());
  }

  if (flags & Propagator_B3) {
    propagators.emplace_back(new traceprop::B3Propagator());
  }

  if (flags & Propagator_B3Multi) {
    propagators.emplace_back(new traceprop::B3PropagatorMultiHeader());
  }

  if (flags & Propagator_Baggage) {
    propagators.emplace_back(new baggageprop::BaggagePropagator());
  }

  auto composite = nostd::shared_ptr<contextprop::TextMapPropagator>(
    new contextprop::CompositePropagator(std::move(propagators)));

  contextprop::GlobalTextMapPropagator::SetGlobalPropagator(composite);
}

opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>
InitOpentelemetry(const OpenTelemetryOptions& options) {
  auto optres = sdkresource::Resource::Create(options.resourceAttributes);

  auto envResource = GetEnvResource(options.resourceAttributes);

  auto resource = sdkresource::Resource::Create(options.resourceAttributes).Merge(envResource);

  auto exporter = CreateExporter(options);
  sdktrace::BatchSpanProcessorOptions processorOptions;
  auto processor = std::unique_ptr<sdktrace::SpanProcessor>(
    new sdktrace::BatchSpanProcessor(std::move(exporter), processorOptions));

  auto provider = nostd::shared_ptr<opentelemetry::trace::TracerProvider>(
    new sdktrace::TracerProvider(std::move(processor), resource));

  opentelemetry::trace::Provider::SetTracerProvider(provider);

  SetupPropagators(options.propagators);

  return provider;
}

} // namespace splunk
