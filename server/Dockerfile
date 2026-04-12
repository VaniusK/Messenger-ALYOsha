# Multi-stage build for messenger server

# Build stage
FROM drogonframework/drogon AS builder

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    curl zip unzip tar \
    pkg-config perl cpanminus && \
    cpanm IPC::Cmd

RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg
RUN /opt/vcpkg/bootstrap-vcpkg.sh
RUN /opt/vcpkg/vcpkg install minio-cpp

WORKDIR /app
COPY . .



RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
RUN cmake --build build --parallel

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    libc-ares2 \
    libjsoncpp25 \
    libmariadb3 \
    libsqlite3-0 \
    libpq5 \
    libcurl4 \
    libhiredis0.14 \
    uuid-dev \
    openssl libssl3 \
    zlib1g \
    ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy executable
COPY --from=builder /app/build/alesha_server_app .

# Copy logs folder, will be empty in non-release
RUN ["mkdir", "logs"] 

# Copy tests executable
COPY --from=builder /app/build/server_tests .

# Copy config files if needed
COPY --from=builder /app/config.json .

EXPOSE 8080

CMD ["./alesha_server_app"]
