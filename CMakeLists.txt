cmake_minimum_required(VERSION 3.5.1)

project(SplunkOpenTelemetry)

include(CMakePackageConfigHelpers)

set(PACKAGE_VERSION "0.1.0")

set(CMAKE_CXX_STANDARD 11)

option(SPLUNK_CPP_TESTS "Enable building of tests" OFF)
option(SPLUNK_CPP_EXAMPLES "Enable building of examples" ON)
option(SPLUNK_CPP_WITH_JAEGER_EXPORTER "Enable Jaeger exporter" ON)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(opentelemetry-cpp REQUIRED)
find_package(absl REQUIRED)
find_package(nlohmann_json REQUIRED)
if (SPLUNK_CPP_WITH_JAEGER_EXPORTER)
  find_package(CURL REQUIRED)
  find_package(Thrift REQUIRED)

  set(SPLUNK_CPP_JAEGER_EXPORTER_LIBS
    CURL::libcurl
    thrift::thrift)
endif()

add_library(SplunkOpenTelemetry
  src/opentelemetry.cpp
)

target_link_libraries(SplunkOpenTelemetry
  PUBLIC
  ${OPENTELEMETRY_CPP_LIBRARIES}
  gRPC::grpc++
  protobuf::libprotobuf
  ${SPLUNK_CPP_JAEGER_EXPORTER_LIBS}
)

target_include_directories(SplunkOpenTelemetry
  PUBLIC $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${OPENTELEMETRY_CPP_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
)

if(SPLUNK_CPP_EXAMPLES)
  set(SPLUNK_OPENTELEMETRY_EXAMPLES
    http_client
    http_server
    simple_example
  )

  add_executable(http_client
    examples/http_client.cpp
  )

  add_executable(simple_example
    examples/simple.cpp
  )

  add_executable(http_server
    examples/http_server.cpp
  )

  foreach(example ${SPLUNK_OPENTELEMETRY_EXAMPLES})
    target_link_libraries(${example}
      PRIVATE SplunkOpenTelemetry
    )

    target_include_directories(${example}
      PRIVATE
      ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
    )
  endforeach()
endif()

install(FILES
  include/splunk/opentelemetry.h
  ${PROJECT_BINARY_DIR}/splunk_config.h
  DESTINATION include/splunk)

include(GNUInstallDirs)

if (SPLUNK_CPP_WITH_JAEGER_EXPORTER)
  set(SPLUNK_HAS_JAEGER 1)
else()
  set(SPLUNK_HAS_JAEGER 0)
endif()

configure_file(src/splunk_config.h.in splunk_config.h)

set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}")
configure_package_config_file(
  SplunkOpenTelemetryConfig.cmake.in
  SplunkOpenTelemetryConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SplunkOpenTelemetry
  PATH_VARS PROJECT_NAME INCLUDE_INSTALL_DIR CMAKE_INSTALL_LIBDIR
)

write_basic_package_version_file(
  SplunkOpenTelemetryConfigVersion.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/SplunkOpenTelemetryConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/SplunkOpenTelemetryConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SplunkOpenTelemetry"
)

install(TARGETS SplunkOpenTelemetry
  EXPORT SplunkOpenTelemetryTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

export(EXPORT SplunkOpenTelemetryTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/SplunkOpenTelemetryTargets.cmake"
  NAMESPACE SplunkOpenTelemetry::
)

install(EXPORT SplunkOpenTelemetryTargets
  FILE SplunkOpenTelemetryTargets.cmake
  NAMESPACE SplunkOpenTelemetry::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SplunkOpenTelemetry"
)

if (SPLUNK_CPP_TESTS)
  include(CTest)
  add_subdirectory(test)
endif()
