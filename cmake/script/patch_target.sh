#!/bin/sh

set -eu

if ! patchelf --print-rpath "${1}" | grep -Eqs '(^|:)\$ORIGIN($|:)'; then
  patchelf --add-rpath '$ORIGIN' "${1}"
fi
