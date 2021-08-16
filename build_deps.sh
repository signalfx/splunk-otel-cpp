set -ex

mkdir -p third_party

BUILD_TYPE=Release
ROOT_DIR=$(pwd)
THIRD_PARTY_DIR=${ROOT_DIR}/third_party

mkdir -p ${ROOT_DIR}/grpc/build
cd ${ROOT_DIR}/grpc/build
cmake \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DgRPC_BUILD_TESTS=OFF \
  -DgRPC_INSTALL=ON \
  -DgRPC_BUILD_CSHARP_EXT=OFF \
  -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
  "$@" \
  ..
make -j$(nproc)
make install
  
cd ${ROOT_DIR}/grpc
mkdir -p third_party/abseil-cpp/cmake/build
cd third_party/abseil-cpp/cmake/build
cmake \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  "$@" \
  ../..
make -j$(nproc)
make install

mkdir -p ${ROOT_DIR}/thrift/cmake_build
cd ${ROOT_DIR}/thrift/cmake_build
cmake \
  -DBUILD_TESTING=OFF \
  -DWITH_LIBEVENT=OFF \
  -DWITH_QT5=OFF \
  -DWITH_ZLIB=OFF \
  -DBUILD_COMPILER=OFF \
  -DBUILD_AS3=OFF \
  -DBUILD_JAVA=OFF \
  -DBUILD_JAVASCRIPT=OFF \
  -DBUILD_NODEJS=OFF \
  -DBUILD_PYTHON=OFF \
  -DBUILD_HASKELL=OFF \
  "$@" \
   ..
make -j$(nproc)
make install

mkdir -p ${ROOT_DIR}/opentelemetry-cpp/build
cd ${ROOT_DIR}/opentelemetry-cpp/build
cmake \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DCMAKE_PREFIX_PATH=${THIRD_PARTY_DIR} \
  -DWITH_OTLP=ON \
  -DWITH_JAEGER=ON \
  -DWITH_ABSEIL=ON \
  -DBUILD_TESTING=OFF \
  -DWITH_EXAMPLES=OFF \
  "$@" \
  ..
make -j$(nproc)
make install
