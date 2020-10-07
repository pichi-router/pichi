FROM alpine:3

ENV SRC_DIR /tmp/src
ENV BUILD_DIR /tmp/build
ADD CMakeLists.txt ${SRC_DIR}/CMakeLists.txt
ADD cmake ${SRC_DIR}/cmake
ADD include ${SRC_DIR}/include
ADD server ${SRC_DIR}/server
ADD src ${SRC_DIR}/src

RUN apk add --no-cache g++ cmake make mbedtls-dev mbedtls-static libsodium-dev libsodium-static \
  rapidjson-dev libmaxminddb-dev boost-dev boost-static libressl-dev ca-certificates && \
  cmake -D CMAKE_BUILD_TYPE=MiniSizeRel -D CMAKE_INSTALL_PREFIX=/usr/local \
  -D BUILD_TEST=OFF -B "${BUILD_DIR}" "${SRC_DIR}" && \
  cmake --build "${BUILD_DIR}" -j "$(nproc)" && \
  cmake --build "${BUILD_DIR}" --target install/strip && \
  apk del --no-cache g++ cmake make mbedtls-dev mbedtls-static libsodium-dev libsodium-static \
  rapidjson-dev libmaxminddb-dev boost-dev boost-static libressl-dev && \
  apk add --no-cache libstdc++ && \
  rm -fr "${BUILD_DIR}" "${SRC_DIR}"
