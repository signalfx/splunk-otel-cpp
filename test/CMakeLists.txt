add_library(TraceVerify OBJECT common/verify.cpp)
target_include_directories(TraceVerify
  PUBLIC ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
)

add_executable(test_default_config cases/default_config.cpp)

target_include_directories(test_default_config
  PUBLIC ${CMAKE_SOURCE_DIR}/include
  ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
)
target_link_libraries(test_default_config
  TraceVerify
  SplunkOpenTelemetry
)

add_test(
  NAME test_default_config
  COMMAND $<TARGET_FILE:test_default_config> ${CMAKE_SOURCE_DIR}/test/data/trace.json
)
