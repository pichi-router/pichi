FROM alpine:3.16

ARG http_proxy
ARG https_proxy
ENV BSSL_SRC /tmp/bssl
ENV BUILD_DIR /tmp/build
ENV STATIC_DIR ${BUILD_DIR}/static
ENV SHARED_DIR ${BUILD_DIR}/shared
ENV BSSL_DIR /opt/bssl

RUN apk add --no-cache boost-context boost-dev boost-filesystem boost-program_options \
  boost-static boost-system boost-unit_test_framework brotli-static brotli-dev ca-certificates \
  cmake g++ git go libmaxminddb-dev libmaxminddb-static libsodium-dev libsodium-static \
  mbedtls-dev mbedtls-static ninja perl rapidjson-dev && \
  git clone https://boringssl.googlesource.com/boringssl "${BSSL_SRC}" && \
  cmake -G Ninja -D CMAKE_BUILD_TYPE=MiniSizeRel -D CMAKE_INSTALL_PREFIX="${BSSL_DIR}" \
  -D FUZZ=OFF -D RUST_BINDINGS=OFF -D FIPS=OFF -D BUILD_SHARED_LIBS=OFF \
  -B "${STATIC_DIR}" "${BSSL_SRC}" && \
  cmake --build "${STATIC_DIR}" --target ssl --target bssl -j "$(nproc)" && \
  cmake --install "${STATIC_DIR}" --strip && \
  cmake -G Ninja -D CMAKE_BUILD_TYPE=MiniSizeRel -D CMAKE_INSTALL_PREFIX="${BSSL_DIR}" \
  -D FUZZ=OFF -D RUST_BINDINGS=OFF -D FIPS=OFF -D BUILD_SHARED_LIBS=ON \
  -B "${SHARED_DIR}" "${BSSL_SRC}" && \
  cmake --build "${SHARED_DIR}" --target ssl --target bssl -j "$(nproc)" && \
  cmake --install "${SHARED_DIR}" --strip && \
  rm -rf "${BSSL_SRC}" "${BUILD_DIR}" "${BSSL_DIR}/bin" && \
  apk del --no-cache go perl
