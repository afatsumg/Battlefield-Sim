FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /workspace

# Help CMake find locally-installed packages (protobuf, gRPC) when building inside the container
ENV CMAKE_PREFIX_PATH=/usr/local
ENV PATH=/usr/local/bin:$PATH

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git wget unzip zip tar autoconf libtool pkg-config \
    curl ca-certificates python3 python3-pip \
    zlib1g-dev libssl-dev ninja-build \
    libyaml-cpp-dev \
 && rm -rf /var/lib/apt/lists/*

# ============================================
# Install Protocol Buffers (protoc and libprotobuf)
# ============================================
RUN git clone https://github.com/protocolbuffers/protobuf.git /tmp/protobuf \
    && cd /tmp/protobuf \
    && git checkout v33.2 \
    && mkdir -p cmake/build && cd cmake/build \
    && cmake ../.. -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF \
    && make -j$(nproc) \
    && make install \
    && ldconfig \
    && cd / \
    && rm -rf /tmp/protobuf

# ============================================
# gRPC C++ plugin
# ============================================
# ============================================
# Abseil (absl) — required by gRPC C++ targets
# Install abseil so `absl::log` and other CMake targets are exported
# ============================================
RUN git clone --depth 1 https://github.com/abseil/abseil-cpp.git /tmp/abseil \
    && cd /tmp/abseil \
    && mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr/local \
    && make -j$(nproc) \
    && make install \
    && ldconfig \
    && cd / \
    && rm -rf /tmp/abseil

# install gRPC
RUN git clone --recursive -b v1.48.0 https://github.com/grpc/grpc /tmp/grpc \
    && mkdir -p /tmp/grpc/cmake/build \
    && cd /tmp/grpc/cmake/build \
    && cmake ../.. -DgRPC_BUILD_TESTS=OFF -DgRPC_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/usr/local \
    && make -j$(nproc) \
    && make install \
    && ldconfig \
    && cd / \
    && rm -rf /tmp/grpc


COPY . /workspace/

RUN echo "Compiling the project..." \
    && mkdir -p build \
    && cd build \
    && cmake .. \
    && cmake --build . -j$(nproc)
