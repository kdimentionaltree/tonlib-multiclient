# TON HTTP API C++

[![Docker - Image Version](https://img.shields.io/docker/v/toncenter/ton-http-api-cpp?label=docker&sort=date)](https://hub.docker.com/repository/docker/toncenter/ton-http-api-cpp)
[![Docker - Image Size](https://img.shields.io/docker/image-size/toncenter/ton-http-api-cpp?label=docker&sort=date)](https://hub.docker.com/repository/docker/toncenter/ton-http-api-cpp)
![Github last commit](https://img.shields.io/github/last-commit/toncenter/ton-http-api-cpp)

The HTTP API C++ for [The Open Network](https://ton.org) is a real-time API to connect to TON blockchain lite servers via ADNL protocol and enable access to lite servers through HTTP requests.

This service is the improved version of original [TON HTTP API](https://github.com/toncenter/ton-http-api), written entirely in C++ to achieve outstanding performance and effecient comnsuming of hardware resources.


## Features

TON HTTP API C++ provides following features:
* Load balances requests to TON nodes
* Checks availability of Lite Servers, disables unavailable and out-of-synced nodes.
* Additionally parses data returned by original TONLib.
* Based on [userver](https://github.com/userver-framework/userver), service provides detailed and Prometheus-compatible usage statistics.


### Key differences from [toncenter/ton-http-api](https://github.com/toncenter/ton-http-api)

* C++ version consumes significantly less CPU and memory resources.
* C++ version uses single instance of TONlib for each Lite server, meanwhile Python version TONLib instances number for each Lite server was equal to number of Gunicorn workers. This significantly decreases the number of healthcheck requests to Lite servers.
* Cache now is integrated in service, no separate Redis is required to enable caching feature.


## Hardware Requirements

- OS: Linux or MacOS (Windows is not supported).
- CPU: 2 vCPU, x86_64 or arm64 architecture.
- RAM: minimum 2GB, 8GB recommended for caching.

## Deploy

There are two main ways to run TON HTTP API:
- __Docker Compose__: flexible configuration, recommended for production environments, works on any x86_64 and arm64 OS with Docker available.
- __Local__ *(experimental)*: only for development purposes.

### Docker Compose

- (First time) Install required tools: `docker`, `docker-compose`, `curl`. 
    - For Ubuntu: run `scripts/setup.sh` from the root of the repo.
    - For MacOS and Windows: install [Docker Desktop](https://www.docker.com/products/docker-desktop/).
    - **Note:** we recommend to use Docker Compose V2.
- Download TON configuration files to private folder:
    ```bash
    mkdir private
    curl -sL https://ton-blockchain.github.io/global.config.json > private/mainnet.json
    curl -sL https://ton-blockchain.github.io/testnet-global.config.json > private/testnet.json
    ```
- Copy service configuration file and adjust params:
    ```bash
    cp config/config_vars.yaml private/config_vars.yaml
    nano private/config_vars.yaml
    ```
    - To run testnet version or connect to your private TON node, please specify:
    ```
    export THACPP_GLOBAL_CONFIG_PATH=private/testnet.json
    # or
    echo "THACPP_GLOBAL_CONFIG_PATH=private/testnet.json" > .env
- Pull or build docker image:
    ```bash
    docker compose pull
    # or 
    docker compose build
    ```
- Run service:
    ```bash
    docker compose up -d
    # or
    docker compose up
    ```
- Additionally check service logs:
    ```bash
    docker compose logs http-api-cpp
    # or 
    docker compose logs -n 100 http-api-cpp
    # or 
    docker compose logs -f -n 100 http-api-cpp
    ```

### Local run

- Install dependencies:
    - For Ubuntu:
    ```bash
    apt update -y
    apt install -y wget curl build-essential cmake clang openssl libssl-dev zlib1g-dev gperf wget git ninja-build libsodium-dev libmicrohttpd-dev liblz4-dev pkg-config autoconf automake libtool libjemalloc-dev lsb-release software-properties-common gnupg libabsl-dev libbenchmark-dev libboost-context1.83-dev libboost-coroutine1.83-dev libboost-filesystem1.83-dev libboost-iostreams1.83-dev libboost-locale1.83-dev libboost-program-options1.83-dev libboost-regex1.83-dev libboost-stacktrace1.83-dev libboost1.83-dev libbson-dev libbz2-dev libc-ares-dev libcctz-dev libcrypto++-dev libcurl4-openssl-dev libdouble-conversion-dev libev-dev libfmt-dev libgflags-dev libgmock-dev libgtest-dev libhiredis-dev libidn11-dev libjemalloc2 libjemalloc-dev libkrb5-dev libldap2-dev liblz4-dev libnghttp2-dev libpugixml-dev libsnappy-dev libsasl2-dev libssl-dev libxxhash-dev libyaml-cpp0.8  libyaml-cpp-dev libzstd-dev libssh2-1-dev netbase python3-dev python3-jinja2 python3-venv python3-yaml ragel yasm zlib1g-dev liblzma-dev libre2-dev clang-format ccache gcc g++ gdb

    # for ubuntu 22.04 check https://userver.tech/de/d9c/md_en_2deps_2ubuntu-22_804.html
    ```

    - For MacOS:
    ```bash
    brew install boost@1.89 c-ares ccache cctz clang-format clickhouse-cpp cmake coreutils cryptopp cyrus-sasl fmt gdb git google-benchmark googletest hiredis icu4c jemalloc krb5 libev librdkafka mariadb mongo-c-driver@1 nghttp2 ninja openldap openssl postgresql@16 pugixml rocksdb unixodbc yaml-cpp zlib sqlite pkg-config automake libtool autoconf texinfo lz4 openssl@3 libsodium zlib libmicrohttpd
    ```
- Build:
    ```bash
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) ton-http-api-cpp
    # optional
    make install
    ```
- Run:
    ```bash
    ./build/ton-http-api/ton-http-api-cpp --config ./config/static_config.yaml --config_vars ./private/config_vars.yaml
    ```

## License

TON HTTP API C++ is licensed under the MIT License.
