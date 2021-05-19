---

<p align="center">
  <strong>
    <a href="#getting-started">Getting Started</a>
    &nbsp;&nbsp;&bull;&nbsp;&nbsp;
    <a href="CONTRIBUTING.md">Getting Involved</a>
  </strong>
</p>

<p align="center">
  <img alt="Beta" src="https://img.shields.io/badge/status-beta-informational?style=for-the-badge">
  <a href="https://github.com/signalfx/splunk-otel-cpp/releases">
    <img alt="GitHub release (latest SemVer)" src="https://img.shields.io/github/v/release/signalfx/splunk-otel-cpp?include_prereleases&style=for-the-badge">
  </a>
</p>

---

# Splunk Distribution of OpenTelemetry C++

The Splunk Distribution of [OpenTelemetry
C++](https://github.com/open-telemetry/opentelemetry-cpp-contrib) provides
multiple installable packages that automatically instruments your C++
application to capture and report distributed traces to Splunk APM.

This distribution comes with the following defaults:

- [W3C `tracecontext`](https://www.w3.org/TR/trace-context/) context
  propagation; [B3](https://github.com/openzipkin/b3-propagation) can also be
  configured.
- [OTLP
  exporter](https://github.com/open-telemetry/opentelemetry-specification/blob/main/specification/protocol/README.md)
  configured to send spans to a locally running [Splunk OpenTelemetry
  Connector](https://github.com/signalfx/splunk-otel-collector)
  (`http://localhost:4317`); [Jaeger Thrift
  exporter](https://github.com/signalfx/splunk-otel-java/blob/main/docs/advanced-config.md#trace-exporters)
  available for [Smart Agent](https://github.com/signalfx/signalfx-agent)
  (`http://localhost:9080/v1/trace`).

> :construction: This project is currently in **BETA**. It is **officially supported** by Splunk. However, breaking changes **MAY** be introduced.

## Getting Started

Download one of the released packages and follow the directions:

- [Apache httpd](https://github.com/open-telemetry/opentelemetry-cpp-contrib/tree/main/instrumentation/httpd)
- [Nginx](https://github.com/open-telemetry/opentelemetry-cpp-contrib/tree/main/instrumentation/nginx)

# License and versioning

The Splunk Distribution of OpenTelemetry C++ is a distribution
of the [OpenTelemetry C++ Contrib
project](https://github.com/open-telemetry/opentelemetry-cpp-contrib).
It is released under the terms of the Apache Software License version 2.0. See
[the license file](./LICENSE) for more details.
