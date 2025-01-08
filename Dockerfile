FROM ubuntu:24.04 as builder
RUN DEBIAN_FRONTEND=noninteractive apt update -y \
    && apt install -y wget curl build-essential cmake clang openssl  \
    libssl-dev zlib1g-dev gperf wget git ninja-build libsodium-dev libmicrohttpd-dev liblz4-dev  \
    pkg-config autoconf automake libtool libjemalloc-dev lsb-release software-properties-common gnupg \
    libabsl-dev libbenchmark-dev libboost-context1.83-dev libboost-coroutine1.83-dev libboost-filesystem1.83-dev  \
    libboost-iostreams1.83-dev libboost-locale1.83-dev libboost-program-options1.83-dev libboost-regex1.83-dev  \
    libboost-stacktrace1.83-dev libboost1.83-dev libbson-dev libbz2-dev libc-ares-dev libcctz-dev libcrypto++-dev  \
    libcurl4-openssl-dev libdouble-conversion-dev libev-dev libfmt-dev libgflags-dev libgmock-dev libgtest-dev \
    libhiredis-dev libidn11-dev libjemalloc2 libjemalloc-dev libkrb5-dev libldap2-dev liblz4-dev \
     libnghttp2-dev libpugixml-dev libsnappy-dev libsasl2-dev libssl-dev libxxhash-dev libyaml-cpp0.8  libyaml-cpp-dev \
    libzstd-dev libssh2-1-dev netbase python3-dev python3-jinja2 python3-venv python3-yaml \
    ragel yasm zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++
ENV CCACHE_DISABLE=1

COPY examples/ /app/examples/
COPY py/ /app/py/
COPY external/ /app/external/
COPY tonlib-multiclient/ /app/tonlib-multiclient/
COPY ton-http-api/ /app/ton-http-api/
COPY CMakeLists.txt /app/CMakeLists.txt

WORKDIR /app/build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DPORTABLE=1 .. && make -j$(nproc) && make install
ENTRYPOINT [ "ton-http-api-cpp" ]
