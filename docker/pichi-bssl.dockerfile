FROM alpine:3.18

ARG http_proxy
ARG https_proxy
ENV SRC_DIR /tmp/src
ENV PICHI_SRC ${SRC_DIR}/pichi
ENV BSSL_SRC ${SRC_DIR}/boringssl
ENV BUILD_DIR /tmp/build
ENV PICHI_BUILD_DIR ${BUILD_DIR}/pichi
ENV BSSL_BUILD_DIR ${BUILD_DIR}/bssl
ENV BSSL_DIR /opt/bssl
ADD CMakeLists.txt ${PICHI_SRC}/CMakeLists.txt
ADD cmake ${PICHI_SRC}/cmake
ADD include ${PICHI_SRC}/include
ADD server ${PICHI_SRC}/server
ADD src ${PICHI_SRC}/src

RUN apk add --no-cache g++ cmake ninja mbedtls-dev mbedtls-static libsodium-dev libsodium-static \
  rapidjson-dev libmaxminddb-dev libmaxminddb-static boost-dev boost-static ca-certificates \
  curl go perl brotli-static brotli-dev && \
  mkdir -p "${BSSL_SRC}" && \
  curl -Ls https://boringssl.googlesource.com/boringssl/+archive/master.tar.gz | \
  tar zxf - -C "${BSSL_SRC}" && \
  cmake -G Ninja -D CMAKE_BUILD_TYPE=MinSizeRel -D CMAKE_INSTALL_PREFIX="${BSSL_DIR}" \
  -D FUZZ=OFF -D RUST_BINDINGS=OFF -D FIPS=OFF -D BUILD_SHARED_LIBS=OFF \
  -B "${BSSL_BUILD_DIR}" "${BSSL_SRC}" && \
  cmake --build "${BSSL_BUILD_DIR}" --target ssl --target bssl -j "$(nproc)" && \
  cmake --install "${BSSL_BUILD_DIR}" --strip && \
  cmake -G Ninja -D CMAKE_BUILD_TYPE=MinSizeRel -D CMAKE_INSTALL_PREFIX=/usr/local \
  -D BUILD_TEST=OFF -D TRANSPARENT_IPTABLES=ON -D OPENSSL_ROOT_DIR="${BSSL_DIR}" \
  -B "${PICHI_BUILD_DIR}" "${PICHI_SRC}" && \
  cmake --build "${PICHI_BUILD_DIR}" -j "$(nproc)" && \
  cmake --install "${PICHI_BUILD_DIR}" --strip && \
  apk del --no-cache g++ cmake ninja mbedtls-dev mbedtls-static libsodium-dev libsodium-static \
  rapidjson-dev libmaxminddb-dev libmaxminddb-static boost-dev boost-static \
  curl go perl brotli-static brotli-dev && \
  apk add --no-cache libstdc++ && \
  rm -fr "${BUILD_DIR}" "${SRC_DIR}" "${BSSL_DIR}"
