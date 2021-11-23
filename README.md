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

The Splunk Distribution of [OpenTelemetry C++](https://github.com/open-telemetry/opentelemetry-cpp) is a wrapper that
comes with defaults suitable to report distributed traces to Splunk APM.

This distribution comes with the following defaults:

- [W3C `tracecontext`](https://www.w3.org/TR/trace-context/) context
  propagation; [B3](https://github.com/openzipkin/b3-propagation) can also be
  configured.
- [OTLP exporter](https://github.com/open-telemetry/opentelemetry-specification/blob/main/specification/protocol/README.md)
  configured to send spans to a locally running [Splunk OpenTelemetry
  Connector](https://github.com/signalfx/splunk-otel-collector)
  (`http://localhost:4317`);

> :construction: This project is currently in **BETA**. It is **officially supported** by Splunk. However, breaking changes **MAY** be introduced.

## Getting Started

1. Clone the repository along with submodules
```bash
$ git clone --recurse-submodules https://github.com/signalfx/splunk-otel-cpp.git
```

2. Build the dependencies and the project

```bash
$ ./build_deps.sh -DCMAKE_INSTALL_PREFIX=/path/to/splunk-otel-cpp-install
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/path/to/splunk-otel-cpp-install ..
$ make install
```

3. When using CMake, find the Splunk OpenTelemetry package at `/path/to/splunk-otel-cpp-install`

```CMake
find_package(SplunkOpenTelemetry REQUIRED)
add_program(example example.cpp)
target_link_libraries(example ${SplunkOpenTelemetry_LIBRARIES})
target_include_directories(example ${SplunkOpenTelemetry_INCLUDE_DIRS})
```

### (Optional) Using Conan package manager with CMake

* Update your `conanfile.txt`:
  ```
  [requires]
  splunk-opentelemetry-cpp/0.4.0

  [generators]
  cmake_find_package
  cmake_paths
  ```
* Add `include(${CMAKE_BINARY_DIR}/conan_paths.cmake)` to your CMakeLists.txt
* Add `find_package(SplunkOpenTelemetry REQUIRED)` lines described earlier.
* `mkdir build && cd build && conan install .. && cmake .. && make`

4. Use it

```c++
#include <splunk/opentelemetry.h>

int main(int argc, char** argv) {
  splunk::OpenTelemetryOptions options =
    splunk::OpenTelemetryOptions()
      .WithServiceName("my-service");

  splunk::InitOpenTelemetry(options);

  // Use the usual OpenTelemetry API
  auto tracer = opentelemetry::trace::Provider::GetTraceProvider()->GetTracer("my-tracer");
  auto span = tracer->StartSpan("operation-name");

  // Do something useful
  span->End();

  return 0;
}

```

For more examples, see the `examples` directory.

## Configuration options


### Via environment variables

Note: options passed via `splunk::OpenTelemetryOptions` take preference over environment variables.

| Environment variable                 | Default value                 | Notes
| -----------------------------        | ----------------------------- | ------------------------------------ |
| OTEL_SERVICE_NAME                    | `unknown_service`             | Service name of the application      |
| OTEL_RESOURCE_ATTRIBUTES             | none                          | Comma separated list of [Resource](https://github.com/open-telemetry/opentelemetry-specification/blob/main/specification/resource/sdk.md#resource-sdk) attributes. For example `OTEL_RESOURCE_ATTRIBUTES=service.name=foo,deployment.environment=production` |
| OTEL_PROPAGATORS                     | `tracecontext,baggage`        | Comma separated list of propagators to use. Possible values: `tracecontext`, `b3`, `b3multi`, `baggage` |
| OTEL_TRACES_EXPORTER                 | `otlp`                        | Trace exporter to use. Possible values: `otlp`, `jaeger-thrift-splunk`. |
| OTEL_EXPORTER_OTLP_PROTOCOL          | `grpc`                        | OTLP transport to use. Possible values: `grpc`. |
| OTEL_EXPORTER_OTLP_ENDPOINT          | `localhost:4317` (gRPC) or `http://localhost:4317/v1/traces` |
| OTEL_EXPORTER_JAEGER_ENDPOINT        | `http://localhost:9080/v1/trace` | Needs to be compiled with Jaeger support
| SPLUNK_ACCESS_TOKEN                  | none                          | Only required when Splunk OpenTelemetry Connector is not used. |

## Requirements

* C++11 capable compiler
* Linux (Windows support coming)

# License and versioning

The Splunk Distribution of OpenTelemetry C++ is a distribution
of the [OpenTelemetry C++ project](https://github.com/open-telemetry/opentelemetry-cpp).
It is released under the terms of the Apache Software License version 2.0. See [the license file](./LICENSE) for more details.
