#include <splunk/opentelemetry.h>

#include <opentelemetry/baggage/propagation/baggage_propagator.h>
#include <opentelemetry/context/propagation/composite_propagator.h>
#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/exporters/jaeger/jaeger_exporter.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/sdk/trace/exporter.h>
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

namespace {
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

std::unique_ptr<sdktrace::SpanExporter> CreateOtlpExporter(const OpenTelemetryOptions& options) {
  if (options.otlpProtocol == "http/json") {
    opentelemetry::exporter::otlp::OtlpHttpExporterOptions exporterOptions;
    exporterOptions.url = options.otlpEndpoint;
    exporterOptions.content_type = opentelemetry::exporter::otlp::HttpRequestContentType::kJson;

    return std::unique_ptr<sdktrace::SpanExporter>(
      new opentelemetry::exporter::otlp::OtlpHttpExporter(exporterOptions));
  }

  if (options.otlpProtocol == "http/protobuf") {
    opentelemetry::exporter::otlp::OtlpHttpExporterOptions exporterOptions;
    exporterOptions.url = options.otlpEndpoint;
    exporterOptions.content_type = opentelemetry::exporter::otlp::HttpRequestContentType::kBinary;

    return std::unique_ptr<sdktrace::SpanExporter>(
      new opentelemetry::exporter::otlp::OtlpHttpExporter(exporterOptions));
  }

  opentelemetry::exporter::otlp::OtlpGrpcExporterOptions exporterOptions;
  exporterOptions.endpoint = options.otlpEndpoint;

  return std::unique_ptr<sdktrace::SpanExporter>(
    new opentelemetry::exporter::otlp::OtlpGrpcExporter(exporterOptions));
}

std::unique_ptr<sdktrace::SpanExporter> CreateJaegerExporter(const OpenTelemetryOptions& options) {
  opentelemetry::exporter::jaeger::JaegerExporterOptions jaegerOptions;
  jaegerOptions.endpoint = options.jaegerEndpoint;

  if (!options.accessToken.empty()) {
    jaegerOptions.headers = {{"X-SF-TOKEN", options.accessToken}};
  }

  jaegerOptions.transport_format = opentelemetry::exporter::jaeger::TransportFormat::kThriftHttp;
  auto exporter = std::unique_ptr<sdktrace::SpanExporter>(
    new opentelemetry::exporter::jaeger::JaegerExporter(jaegerOptions));

  return exporter;
}

std::unique_ptr<sdktrace::SpanExporter> CreateExporter(const OpenTelemetryOptions& options) {
  switch (options.exporterType) {
    case ExporterType_JaegerThriftHttp:
      return CreateJaegerExporter(options);
    default: {
      return CreateOtlpExporter(options);
    }
  }
}

std::unordered_map<std::string, std::string> GetEnvResourceAttribs() {
  auto rawAttribs = GetEnv("OTEL_RESOURCE_ATTRIBUTES", "");

  std::unordered_map<std::string, std::string> attributes;

  std::vector<nostd::string_view> tokens = Split(rawAttribs, ',');

  for (nostd::string_view token : tokens) {
    std::vector<nostd::string_view> parts = Split(token, '=');

    if (parts.size() == 2) {
      attributes[std::string(parts[0])] = std::string(parts[1]);
    }
  }

  auto envServiceName = GetEnv("OTEL_SERVICE_NAME", "");

  if (!envServiceName.empty()) {
    attributes["service.name"] = envServiceName;
  }

  return attributes;
}

sdkresource::ResourceAttributes MergeEnvAttributes(
  sdkresource::ResourceAttributes target,
  const std::unordered_map<std::string, std::string>& source) {
  for (const auto& attribute : source) {
    if (target.GetAttributes().count(attribute.first) == 0) {
      target.SetAttribute(attribute.first, attribute.second);
    }
  }

  return target;
}

PropagatorType EnvPropagatorFlags() {
  auto envPropagators = GetEnv("OTEL_PROPAGATORS", "tracecontext,baggage");

  std::vector<nostd::string_view> propagators = Split(envPropagators, ',');

  int flags = PropagatorType_None;

  for (nostd::string_view propagator : propagators) {
    if (propagator == "tracecontext") {
      flags |= PropagatorType_TraceContext;
    } else if (propagator == "b3") {
      flags |= PropagatorType_B3;
    } else if (propagator == "b3multi") {
      flags |= PropagatorType_B3Multi;
    } else if (propagator == "baggage") {
      flags |= PropagatorType_Baggage;
    }
  }

  return static_cast<PropagatorType>(flags);
}

void SetupPropagators(PropagatorType flags) {
  namespace contextprop = opentelemetry::context::propagation;
  namespace traceprop = opentelemetry::trace::propagation;
  namespace baggageprop = opentelemetry::baggage::propagation;

  std::vector<std::unique_ptr<contextprop::TextMapPropagator>> propagators;

  if (flags & PropagatorType_TraceContext) {
    propagators.emplace_back(new traceprop::HttpTraceContext());
  }

  if (flags & PropagatorType_B3) {
    propagators.emplace_back(new traceprop::B3Propagator());
  }

  if (flags & PropagatorType_B3Multi) {
    propagators.emplace_back(new traceprop::B3PropagatorMultiHeader());
  }

  if (flags & PropagatorType_Baggage) {
    propagators.emplace_back(new baggageprop::BaggagePropagator());
  }

  auto composite = nostd::shared_ptr<contextprop::TextMapPropagator>(
    new contextprop::CompositePropagator(std::move(propagators)));

  contextprop::GlobalTextMapPropagator::SetGlobalPropagator(composite);
}

bool IsSupportedOtlpProtocol(const std::string& proto) {
  return proto == "grpc" || proto == "http/json" || proto == "http/protobuf";
}

OpenTelemetryOptions ApplyDefaults(OpenTelemetryOptions options) {
  options.resourceAttributes =
    MergeEnvAttributes(options.resourceAttributes, GetEnvResourceAttribs());

  if (options.exporterType == ExporterType_None) {
    auto envExporter = GetEnv("OTEL_TRACES_EXPORTER", "otlp");

    if (envExporter == "jaeger-thrift-splunk") {
      options.exporterType = ExporterType_JaegerThriftHttp;
    } else {
      options.exporterType = ExporterType_Otlp;
    }
  }

  if (options.propagators == PropagatorType_None) {
    options.propagators = EnvPropagatorFlags();

    if (options.propagators == PropagatorType_None) {
      options.propagators =
        static_cast<PropagatorType>(PropagatorType_TraceContext | PropagatorType_Baggage);
    }
  }

  options.otlpEndpoint = options.otlpEndpoint.empty()
                           ? GetEnv("OTEL_EXPORTER_OTLP_ENDPOINT", "localhost:4317")
                           : options.otlpEndpoint;

  options.otlpProtocol = options.otlpProtocol.empty()
                           ? GetEnv("OTEL_EXPORTER_OTLP_PROTOCOL", "grpc")
                           : options.otlpProtocol;

  if (!IsSupportedOtlpProtocol(options.otlpProtocol)) {
    options.otlpProtocol = "grpc";
  }

  options.jaegerEndpoint =
    options.jaegerEndpoint.empty()
      ? GetEnv("OTEL_EXPORTER_JAEGER_ENDPOINT", "http://localhost:9080/v1/trace")
      : options.jaegerEndpoint;

  options.accessToken =
    options.accessToken.empty() ? GetEnv("SPLUNK_ACCESS_TOKEN", "") : options.accessToken;

  return options;
}

} // namespace

opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>
InitOpentelemetry(const OpenTelemetryOptions& userOptions) {
  OpenTelemetryOptions options = ApplyDefaults(userOptions);

  auto resource = sdkresource::Resource::Create(options.resourceAttributes);

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

OpenTelemetryOptions& OpenTelemetryOptions::WithServiceName(const std::string& serviceName) {
  resourceAttributes.SetAttribute("service.name", serviceName);
  return *this;
}

OpenTelemetryOptions&
OpenTelemetryOptions::WithDeploymentEnvironment(const std::string& deploymentEnvironment) {
  resourceAttributes.SetAttribute("deployment.environment", deploymentEnvironment);
  return *this;
}

OpenTelemetryOptions& OpenTelemetryOptions::WithServiceVersion(const std::string& serviceVersion) {
  resourceAttributes.SetAttribute("service.version", serviceVersion);
  return *this;
}

OpenTelemetryOptions& OpenTelemetryOptions::WithExporter(ExporterType type) {
  exporterType = type;
  return *this;
}

OpenTelemetryOptions& OpenTelemetryOptions::WithOtlpEndpoint(const std::string& endpoint) {
  otlpEndpoint = endpoint;
  return *this;
}

OpenTelemetryOptions& OpenTelemetryOptions::WithJaegerEndpoint(const std::string& endpoint) {
  jaegerEndpoint = endpoint;
  return *this;
}

OpenTelemetryOptions& OpenTelemetryOptions::WithPropagators(PropagatorType flags) {
  propagators = flags;
  return *this;
}

} // namespace splunk
