#pragma once

#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/trace/provider.h>

namespace splunk {

enum PropagatorFlags {
  Propagator_None = 0x0,
  Propagator_TraceContext = 0x01,
  Propagator_B3 = 0x02,
  Propagator_B3Multi = 0x04,
  Propagator_Baggage = 0x08,
};

inline PropagatorFlags operator|(PropagatorFlags a, PropagatorFlags b) {
  return static_cast<PropagatorFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline PropagatorFlags& operator|=(PropagatorFlags& a, PropagatorFlags b) {
  a = a | b;
  return a;
}

struct OpenTelemetryOptions {
  opentelemetry::sdk::resource::ResourceAttributes resourceAttributes;
  PropagatorFlags propagators = Propagator_TraceContext | Propagator_Baggage;
  std::string otlpEndpoint = "localhost:4317";

  OpenTelemetryOptions& WithServiceName(const std::string& serviceName) {
    resourceAttributes.SetAttribute("service.name", serviceName);
    return *this;
  }

  OpenTelemetryOptions& WithDeploymentEnvironment(const std::string& deploymentEnvironment) {
    resourceAttributes.SetAttribute("deployment.environment", deploymentEnvironment);
    return *this;
  }

  OpenTelemetryOptions& WithServiceVersion(const std::string& serviceVersion) {
    resourceAttributes.SetAttribute("service.version", serviceVersion);
    return *this;
  }

	OpenTelemetryOptions& WithOtlpEndpoint(const std::string& endpoint) {
    otlpEndpoint = endpoint;
    return *this;
  }

  OpenTelemetryOptions& WithPropagators(PropagatorFlags flags) {
    propagators = flags;
    return *this;
  }
};

opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>
InitOpentelemetry(const OpenTelemetryOptions& options = {});

} // namespace splunk
