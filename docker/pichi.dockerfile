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
  apk add --no-cache g++ cmake make mbedtls-dev libsodium-dev rapidjson-dev libmaxminddb-dev \
  boost-context boost-dev boost-program_options boost-system boost-unit_test_framework && \
  mkdir -p ${BUILD_DIR} && \
  cd ${BUILD_DIR} && \
  cmake -DCMAKE_BUILD_TYPE=MiniSizeRel ${SRC_DIR} && \
  cmake --build ${BUILD_DIR} -j "$(nproc)" && \
  cd ${BUILD_DIR} && ctest --output-on-failure && \
  strip ${BUILD_DIR}/server/pichi && \
  apk del --no-cache g++ cmake make mbedtls-dev libsodium-dev rapidjson-dev libmaxminddb-dev \
  boost-context boost-dev boost-program_options boost-system boost-unit_test_framework && \
  apk add --no-cache libstdc++ mbedtls libsodium libmaxminddb boost-context boost-program_options\
  boost-system && \
  mkdir -p /usr/share/pichi && \
  cp ${SRC_DIR}/test/geo.mmdb /usr/share/pichi/geo.mmdb && \
  cp ${BUILD_DIR}/server/pichi /usr/bin/pichi && \
  rm -fr ${BUILD_DIR} ${SRC_DIR}
