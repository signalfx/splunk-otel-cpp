#pragma once

#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/trace/provider.h>

namespace splunk {

enum PropagatorType {
  PropagatorType_None = 0x0,
  PropagatorType_TraceContext = 0x01,
  PropagatorType_B3 = 0x02,
  PropagatorType_B3Multi = 0x04,
  PropagatorType_Baggage = 0x08,
};

enum ExporterType {
  ExporterType_None,
  ExporterType_Otlp,
  ExporterType_JaegerThriftHttp,
};

struct OpenTelemetryOptions {
  opentelemetry::sdk::resource::ResourceAttributes resourceAttributes;
  ExporterType exporterType = ExporterType_None;
  PropagatorType propagators = PropagatorType_None;
  std::string otlpEndpoint;
  std::string otlpProtocol;
  std::string jaegerEndpoint;
  /* Access token is only required when not using Splunk OpenTelemetry Connector */
  std::string accessToken;

  OpenTelemetryOptions& WithServiceName(const std::string& serviceName);
  OpenTelemetryOptions& WithDeploymentEnvironment(const std::string& deploymentEnvironment);
  OpenTelemetryOptions& WithServiceVersion(const std::string& serviceVersion);
  OpenTelemetryOptions& WithExporter(ExporterType type);
  OpenTelemetryOptions& WithOtlpEndpoint(const std::string& endpoint);
  OpenTelemetryOptions& WithJaegerEndpoint(const std::string& endpoint);
  OpenTelemetryOptions& WithPropagators(PropagatorType flags);
};

opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>
InitOpentelemetry(const OpenTelemetryOptions& options = {});

} // namespace splunk
