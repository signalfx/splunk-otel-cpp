FROM alpine:3.9

RUN apk update && apk add cmake git make g++ linux-headers openssl-dev

RUN git clone --depth 1 --recurse-submodules -b v1.39.1 https://github.com/grpc/grpc

RUN cd grpc \
  && mkdir -p build \
  && cd build \
  && cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DgRPC_BUILD_TESTS=OFF \
  -DgRPC_INSTALL=ON \
  -DgRPC_BUILD_CSHARP_EXT=OFF \
  -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
  -DgRPC_BACKWARDS_COMPATIBILITY_MODE=ON \
  -DCMAKE_INSTALL_PREFIX=/splunk-cpp-distro \
  -DCMAKE_INSTALL_LIBDIR=lib \
  .. \
  && make -j$(nproc) \
  && make install

RUN cd /grpc/third_party/abseil-cpp \
  && mkdir -p cmake/build \
  && cd cmake/build \
  && cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/splunk-cpp-distro \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_INSTALL_LIBDIR=lib \
  ../.. \
  && make -j$(nproc) \
  && make install

RUN git clone --depth 1 --recurse-submodules -b v1.0.0-rc4 https://github.com/open-telemetry/opentelemetry-cpp

RUN cd /opentelemetry-cpp \
  && mkdir -p build \
  && cd build \
  && cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_OTLP=ON \
    -DWITH_OTLP_HTTP=OFF \
    -DWITH_JAEGER=OFF \
    -DWITH_ABSEIL=ON \
    -DBUILD_TESTING=OFF \
    -DWITH_EXAMPLES=OFF \
    -DCMAKE_PREFIX_PATH=/splunk-cpp-distro \
    -DCMAKE_INSTALL_PREFIX=/splunk-cpp-distro \
    -DCMAKE_INSTALL_LIBDIR=lib \
    .. \
  && make -j$(nproc) \
  && make install


COPY ./build_splunk_distro /usr/bin/build_splunk_distro

CMD sh
