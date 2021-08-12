#pragma once

#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/trace/span.h>
#include <stdio.h>
#include <string>

struct TraceVerification {
  FILE* traceFile = nullptr;
  const opentelemetry::sdk::resource::Resource* resource = nullptr;
  std::vector<const opentelemetry::trace::Span*> spans;
};

TraceVerification VerifyBegin(const char* tracesPath);
void VerifyTraces(const TraceVerification& args);
