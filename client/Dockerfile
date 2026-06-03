FROM ubuntu:24.04 AS base

RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    cmake \
    clang \
    qt6-base-dev \
    qt6-declarative-dev \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-templates \
    libgl1-mesa-dev \
    libxkbcommon-dev \
    libvulkan-dev \
    qt6-websockets-dev \
    qt6-svg-dev \
    qt6-wayland \
    libqt6multimedia6 \
    qt6-multimedia-dev \
    qml6-module-qtmultimedia \
    gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    ca-certificates \
    openssl \
    && rm -rf /var/lib/apt/lists/*
    
WORKDIR /usr/src/client
COPY . .
ENV QT_QPA_PLATFORM=offscreen
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
RUN cmake --build build --parallel
CMD ["cp", "/usr/src/client/build/client", "/output/client"]

FROM base AS test-build
WORKDIR /usr/src/client
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
RUN cmake --build build --parallel --target client_tests

FROM base AS test-runner
COPY --from=test-build /usr/src/client/build/client_tests /usr/local/bin/
CMD ["client_tests"]
