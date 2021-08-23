add_library(TraceVerify OBJECT common/verify.cpp)
target_include_directories(TraceVerify
  PUBLIC ${OPENTELEMETRY_CPP_INCLUDE_DIRS})

add_executable(test_example_config cases/test_example_config.cpp)
add_executable(test_jaeger_thrift_http cases/test_jaeger_thrift_http.cpp)
add_executable(test_empty_config cases/test_empty_config.cpp)
add_executable(test_env_config cases/test_env_config.cpp)

set(TEST_INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/include
  ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
set(TEST_LINK_LIBRARIES
  TraceVerify
  SplunkOpenTelemetry)
set(TEST_TARGETS
  test_example_config
  test_jaeger_thrift_http
  test_empty_config
  test_env_config)

foreach(TEST_TARGET ${TEST_TARGETS})
  target_include_directories(${TEST_TARGET} PUBLIC ${TEST_INCLUDE_DIRS})
  target_link_libraries(${TEST_TARGET} ${TEST_LINK_LIBRARIES})
  add_test(
    NAME ${TEST_TARGET}
    COMMAND $<TARGET_FILE:${TEST_TARGET}> ${CMAKE_SOURCE_DIR}/test/data/trace.json)
endforeach()

set_tests_properties(test_env_config PROPERTIES
  ENVIRONMENT "OTEL_RESOURCE_ATTRIBUTES=service.name=foo,service.version=1.32;OTEL_SERVICE_NAME=envs")
