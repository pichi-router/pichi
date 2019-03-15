FROM alpine:3.9

RUN sed -i 's/v3\.9/edge/g' /etc/apk/repositories && \
  apk add --no-cache g++ cmake make mbedtls-dev mbedtls-static libsodium-dev rapidjson-dev \
  libmaxminddb-dev boost-context boost-dev boost-filesystem boost-program_options boost-static \
  boost-system boost-unit_test_framework libressl-dev
