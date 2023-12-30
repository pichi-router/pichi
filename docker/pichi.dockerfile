FROM alpine:3 AS Builder

ARG FINGERPRINT="false"
ARG VERSION=latest
ENV SRC=/tmp/pichi

RUN apk add --no-cache cmake g++ git go jq linux-headers make perl py3-pip
RUN pip install --break-system-packages conan

RUN --mount="target=${SRC}" <<EOF
arg="-o"
if [ "${FINGERPRINT}" = "true" ]; then
  arg=""
  "${SRC}/.conan/scripts/conan.sh" export -k "${SRC}/.conan/scripts/latest.lock" boringssl
fi
"${SRC}/.conan/scripts/conan.sh" export -k "${SRC}/.conan/scripts/latest.lock" libmaxminddb
"${SRC}/.conan/scripts/conan.sh" build -k "${SRC}/.conan/scripts/latest.lock" \
  -fp linux "${arg}" "${VERSION}"
cp -f $("${SRC}/.conan/scripts/conan.sh" path -p linux "${arg}" "${VERSION}")/bin/pichi /usr/bin
strip -s /usr/bin/pichi
EOF

FROM alpine:3

RUN apk add --no-cache ca-certificates libstdc++
COPY --from=Builder /usr/bin/pichi /usr/bin
ENTRYPOINT [ "/usr/bin/pichi" ]
