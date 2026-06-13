FROM alpine:3.24 AS builder

ARG FINGERPRINT="True"
ARG VERSION=latest
ENV SRC=/tmp/pichi
ENV JUST="just -f ${SRC}/.justfile"

RUN apk add --no-cache cmake g++ git go jq just linux-headers make perl py3-pip
RUN pip install --break-system-packages conan

RUN --mount="target=${SRC}" <<EOF
set -eu
if [ "${FINGERPRINT}" = "false" ]; then
  FINGERPRINT=False
fi
${JUST} detect-host
conan download -r conancenter --only-recipe b2/5.4.2
conan create --version 5.4.2 -b '*' "$(conan cache path b2/5.4.2)"
${JUST} build ${VERSION} MinSizeRel
cp -f "$(find ${HOME}/.conan2/p/b -name pichi -type f | grep 'bin/pichi$')" /usr/bin
strip -s /usr/bin/pichi
EOF

FROM alpine:3.24

RUN apk add --no-cache ca-certificates libstdc++
COPY --from=builder /usr/bin/pichi /usr/bin
ENTRYPOINT [ "/usr/bin/pichi" ]
