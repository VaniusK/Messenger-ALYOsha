# Multi-stage build for messenger server

# Build stage
FROM drogonframework/drogon AS builder

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    curl zip unzip tar \
    lcov \
    pkg-config perl cpanminus && \
    cpanm IPC::Cmd

RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg
RUN /opt/vcpkg/bootstrap-vcpkg.sh
RUN /opt/vcpkg/vcpkg install minio-cpp

WORKDIR /app
COPY . .

ARG ENABLE_COVERAGE=OFF


ARG CMAKE_RELEASE_TYPE=Release

RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=${CMAKE_RELEASE_TYPE} -DBUILD_TESTS=ON \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake -DENABLE_COVERAGE=${ENABLE_COVERAGE}
RUN cmake --build build --parallel

# Coverage stage
FROM builder AS coverage

WORKDIR /app/build
CMD ["bash", "-c", "./server_tests && lcov --capture --directory . -o coverage.info && lcov --remove coverage.info '/usr/*' '*/build/_deps/*' '*/tests/*' '*/models/*' '*/miniocpp/*' -o filtered.info && genhtml filtered.info -o /output/coverage_report --title 'Alesha Coverage'"]

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

# Copy test data
COPY --from=builder /app/tests/test_data ./tests/test_data

# Copy config files if needed
COPY --from=builder /app/config.json .

EXPOSE 8080

CMD ["./alesha_server_app"]
