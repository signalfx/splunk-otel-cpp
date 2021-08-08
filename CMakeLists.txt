cmake_minimum_required(VERSION 3.12)

project(SplunkOpenTelemetry)

include(CMakePackageConfigHelpers)

set(PACKAGE_VERSION "0.1.0")

set(CMAKE_CXX_STANDARD 11)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Thrift REQUIRED)
find_package(opentelemetry-cpp REQUIRED)
find_package(CURL REQUIRED)
find_package(absl REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(SplunkOpenTelemetry
  src/opentelemetry.cpp
)

target_link_libraries(SplunkOpenTelemetry
  PUBLIC
  ${OPENTELEMETRY_CPP_LIBRARIES}
  gRPC::grpc++
  protobuf::libprotobuf
  CURL::libcurl
)

target_include_directories(SplunkOpenTelemetry
  PUBLIC $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${OPENTELEMETRY_CPP_INCLUDE_DIRS}>
)

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
    SplunkOpenTelemetry
  )

  target_include_directories(${example}
    PRIVATE
    ${OPENTELEMETRY_CPP_INCLUDE_DIRS}
  )
endforeach()
  
install(TARGETS SplunkOpenTelemetry
  EXPORT SplunkOpenTelemetryTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION include
)

install(FILES include/splunk/opentelemetry.h DESTINATION include/splunk)

install(EXPORT SplunkOpenTelemetryTargets
  FILE SplunkOpenTelemetryTargets.cmake
  NAMESPACE SplunkOpenTelemetry::
  DESTINATION lib/cmake/SplunkOpenTelemetry
)

include(GNUInstallDirs)

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

export(EXPORT SplunkOpenTelemetryTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/SplunkOpenTelemetryTargets.cmake"
  NAMESPACE SplunkOpenTelemetry::
)

export(TARGETS SplunkOpenTelemetry NAMESPACE SplunkOpenTelemetry:: FILE SplunkOpenTelemetryTargets.cmake)
