FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git curl zip unzip pkg-config ninja-build \
    ca-certificates && \
    rm -rf /var/lib/apt/lists/*

ARG VCPKG_ROOT=/opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} && \
    ${VCPKG_ROOT}/bootstrap-vcpkg.sh

ENV VCPKG_ROOT=${VCPKG_ROOT}
ENV PATH="${VCPKG_ROOT}:${PATH}"

WORKDIR /pragmabackend
COPY vcpkg.json /pragmabackend/vcpkg.json

RUN vcpkg install --clean-buildtrees-after-build

COPY . /pragmabackend

RUN cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_FEATURE_FLAGS=manifests,registries \
    -G Ninja && \
    cmake --build build --config Release

RUN mkdir -p /pragmabackend/build/logs

EXPOSE 80 8081 8082

WORKDIR /pragmabackend/build
CMD ["./pragmabackend"]