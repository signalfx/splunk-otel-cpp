#include "verify.h"
#include "picojson.h"

#include <chrono>
#include <string>
#include <thread>

namespace {

std::unordered_map<std::string, std::string> ExtractStringAttributes(const picojson::array& array) {
  std::unordered_map<std::string, std::string> attributes;

  for (const auto& v : array) {
    auto obj = v.get<picojson::object>();
    std::string key = obj["key"].get<std::string>();
    auto val = obj["value"].get<picojson::object>();

    if (val.count("stringValue") > 0) {
      std::string value = val["stringValue"].get<std::string>();
      attributes.emplace(std::move(key), std::move(value));
    }
  }

  return attributes;
}

std::string LoadFileContent(FILE* f) {
  if (!f) {
    return "";
  }

  int64_t start = ftell(f);
  fseek(f, 0, SEEK_END);
  int64_t end = ftell(f);
  int64_t size = end - start;
  fseek(f, start, SEEK_SET);

  if (size == 0) {
    return "";
  }

  std::string data(size, 0);
  fread(&data[0], 1, size, f);

  return data;
}

std::string LoadTraces(FILE* file) {
  auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(10);

  std::string content;
  while (content.empty()) {
    if (std::chrono::system_clock::now() > timeout) {
      fprintf(stderr, "Timed out waiting for traces\n");
      abort();
    }

    content = LoadFileContent(file);

    if (!content.empty()) {
      return content;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return "";
}

} // namespace

#define check(v, fmt, ...)                                                                         \
  do {                                                                                             \
    if (!(v)) {                                                                                    \
      fprintf(stderr, "%s (line %d): " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);               \
      abort();                                                                                     \
    }                                                                                              \
  } while (false);

TraceVerification VerifyBegin(const char* tracesPath) {
  TraceVerification verification;
  verification.traceFile = fopen(tracesPath, "rb");

  if (verification.traceFile) {
    fseek(verification.traceFile, 0, SEEK_END);
  }

  return verification;
}

void VerifyTraces(const TraceVerification& verification) {
  std::string content = LoadTraces(verification.traceFile);

  picojson::value json;
  std::string err = picojson::parse(json, content);

  printf("Verifying trace:\n%s\n", content.c_str());

  check(err.empty(), "Unable to parse trace JSON: %s", err.c_str());

  check(json.is<picojson::object>(), "Invalid trace JSON");

  auto trace = json.get<picojson::object>();

  auto resourceSpansMaybe = trace["resourceSpans"];

  check(resourceSpansMaybe.is<picojson::array>(), "Resource spans is not an array");

  auto resourceSpans = resourceSpansMaybe.get<picojson::array>();

  check(resourceSpans.size() == 1, "Too many resource spans");

  auto resourceSpan = resourceSpans[0].get<picojson::object>();
  auto resourceAttributes = ExtractStringAttributes(
    resourceSpan["resource"].get<picojson::object>()["attributes"].get<picojson::array>());


  if (verification.resource) {
    for (auto& attrib : verification.resource->GetAttributes()) {
      const auto& key = attrib.first;
      const auto& value = opentelemetry::nostd::get<std::string>(attrib.second);
      check(resourceAttributes.count(key) > 0, "Missing resource attribute %s", key.c_str());
      const auto& actualValue = resourceAttributes[key];
      check(
        actualValue == value, "Resource attribute %s mismatch. Expected: '%s' actual '%s'",
        key.c_str(), value.c_str(), actualValue.c_str());
    }
  }

  auto ilSpansMaybe = resourceSpan["instrumentationLibrarySpans"].get<picojson::array>();
  check(
    ilSpansMaybe.size() == 1,
    "InstrumentationLibrary span count does not match. Expected 1. Got %zu", ilSpansMaybe.size());

  auto instrumentationLib = ilSpansMaybe[0].get<picojson::object>();

  /*
   * TODO: Re-enable instrumentation library check once Otel CPP attaches it to Jaeger exporter.
  auto instrumentationLibName =
    instrumentationLib["instrumentationLibrary"].get<picojson::object>()["name"].get<std::string>();
  check(
    instrumentationLibName == "sample",
    "InstrumentationLibrary name mismatch. Expected 'sample', got '%s'",
    instrumentationLibName.c_str());
  */

  auto spanObjects = instrumentationLib["spans"].get<picojson::array>();
  check(
    spanObjects.size() == verification.spans.size(),
    "Span count does not match. Expected %zu. Got %zu", verification.spans.size(),
    spanObjects.size());

  std::vector<std::unordered_map<std::string, std::string>> spans;

  for (size_t i = 0; i < spanObjects.size(); i++) {
    auto obj = spanObjects[i].get<picojson::object>();
    const opentelemetry::trace::Span* span = verification.spans[i];

    char traceIdBuf[32];
    span->GetContext().trace_id().ToLowerBase16(traceIdBuf);

    char spanIdBuf[16];
    span->GetContext().span_id().ToLowerBase16(spanIdBuf);

    std::string traceId(traceIdBuf, 32);
    std::string actualTraceId = obj["traceId"].get<std::string>();
    check(
      actualTraceId == traceId, "Trace ID mismatch. Expected '%s', got '%s'", actualTraceId.c_str(),
      traceId.c_str());

    std::string spanId(spanIdBuf, 16);
    std::string actualSpanId = obj["spanId"].get<std::string>();
    check(
      actualSpanId == spanId, "Span ID mismatch. Expected '%s', got '%s'", actualSpanId.c_str(),
      spanId.c_str());
  }
}
