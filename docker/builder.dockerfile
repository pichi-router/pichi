FROM alpine:3.9

RUN apk add --no-cache g++ cmake make mbedtls-dev mbedtls-static libsodium-dev rapidjson-dev \
  libmaxminddb-dev boost-context boost-dev boost-filesystem boost-program_options boost-static \
  boost-system boost-unit_test_framework libressl-dev
