FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential git curl zip unzip pkg-config ninja-build ca-certificates \
    linux-libc-dev python3 \
    autoconf autoconf-archive automake libtool m4 perl gpg wget \
 && rm -rf /var/lib/apt/lists/*

RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc \
  | gpg --dearmor -o /etc/apt/trusted.gpg.d/kitware.gpg \
 && echo "deb [signed-by=/etc/apt/trusted.gpg.d/kitware.gpg] https://apt.kitware.com/ubuntu/ jammy main" \
    > /etc/apt/sources.list.d/kitware.list \
 && apt-get update && apt-get install -y --no-install-recommends cmake \
 && cmake --version

ARG VCPKG_ROOT=/opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
 && ${VCPKG_ROOT}/bootstrap-vcpkg.sh
ENV VCPKG_ROOT=${VCPKG_ROOT}
ENV PATH="${VCPKG_ROOT}:${PATH}"
ENV VCPKG_BUILD_TYPE=release
ENV VCPKG_DEFAULT_TRIPLET=x64-linux

WORKDIR /app

COPY vcpkg.json /app/vcpkg.json
RUN vcpkg install --clean-buildtrees-after-build

COPY . /app

RUN cmake -B build -S . \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DVCPKG_BUILD_TYPE=release \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_FEATURE_FLAGS=manifests,registries \
    -DREQUIRE_RELEASE_BUILD=ON
RUN cmake --build build -j

RUN mkdir -p /app/build/logs
EXPOSE 80 8081 8082
WORKDIR /app/build
CMD ["./pragmabackend"]