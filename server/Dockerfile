# Multi-stage build for messenger server

# Build stage
FROM drogonframework/drogon AS builder

WORKDIR /app
COPY . .

RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
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

EXPOSE 5555

CMD ["./alesha_server_app"]
