FROM alpine:3.8

ENV SRC_DIR /tmp/src
ENV BUILD_DIR /tmp/build
ADD CMakeLists.txt ${SRC_DIR}/CMakeLists.txt
ADD cmake ${SRC_DIR}/cmake
ADD include ${SRC_DIR}/include
ADD server ${SRC_DIR}/server
ADD src ${SRC_DIR}/src
ADD test ${SRC_DIR}/test

RUN sed -i 's/v3\.8/edge/g' /etc/apk/repositories && \
  apk add --no-cache g++ cmake make mbedtls-dev mbedtls-static libsodium-dev rapidjson-dev \
  libmaxminddb-dev boost-dev boost-static && \
  mkdir -p "${BUILD_DIR}" && cd "${BUILD_DIR}" && \
  cmake -D CMAKE_BUILD_TYPE=MiniSizeRel -D CMAKE_INSTALL_PREFIX=/usr "${SRC_DIR}" && \
  cmake --build "${BUILD_DIR}" -j "$(nproc)" && \
  cd "${BUILD_DIR}" && ctest --output-on-failure && \
  cd / && cmake --build "${BUILD_DIR}" --target install/strip && \
  rm -f /usr/lib/libpichi.a && \
  apk del --no-cache g++ cmake make mbedtls-dev mbedtls-static libsodium-dev rapidjson-dev \
  libmaxminddb-dev boost-dev boost-static && \
  apk add --no-cache libstdc++ && \
  rm -fr "${BUILD_DIR}" "${SRC_DIR}"
