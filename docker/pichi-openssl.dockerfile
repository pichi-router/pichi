FROM alpine:3.18

ARG http_proxy
ARG https_proxy
ENV SRC_DIR /tmp/src
ENV PICHI_SRC ${SRC_DIR}/pichi
ENV BUILD_DIR /tmp/build
ENV PICHI_BUILD_DIR ${BUILD_DIR}/pichi
ADD CMakeLists.txt ${PICHI_SRC}/CMakeLists.txt
ADD cmake ${PICHI_SRC}/cmake
ADD include ${PICHI_SRC}/include
ADD server ${PICHI_SRC}/server
ADD src ${PICHI_SRC}/src

RUN apk add --no-cache g++ cmake ninja mbedtls-dev mbedtls-static libsodium-dev libsodium-static \
  rapidjson-dev libmaxminddb-dev libmaxminddb-static boost-dev boost-static ca-certificates \
  openssl-dev && \
  cmake -G Ninja -D CMAKE_BUILD_TYPE=MinSizeRel -D CMAKE_INSTALL_PREFIX=/usr/local \
  -D BUILD_TEST=OFF -D TRANSPARENT_IPTABLES=ON -D TLS_FINGERPRINT=OFF \
  -B "${PICHI_BUILD_DIR}" "${PICHI_SRC}" && \
  cmake --build "${PICHI_BUILD_DIR}" -j "$(nproc)" && \
  cmake --install "${PICHI_BUILD_DIR}" --strip && \
  apk del --no-cache g++ cmake ninja mbedtls-dev mbedtls-static libsodium-dev libsodium-static \
  rapidjson-dev libmaxminddb-dev libmaxminddb-static boost-dev boost-static \
  openssl-dev && \
  apk add --no-cache libstdc++ && \
  rm -fr "${BUILD_DIR}" "${SRC_DIR}" "${BSSL_DIR}"
