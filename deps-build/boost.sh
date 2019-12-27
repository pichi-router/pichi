#!/bin/bash

function unknown_platform()
{
  echo "Unknown platform: ${PLATFORM}"
  echo "Available platforms are:"
  echo "  iOS    : iphoneos, iphonesimulator, appletvos, appletvsimulator."
  echo "  Android: android-16, android-17, android-18, android-19, android-21, android-22,"
  echo "           android-23, android-24, android-26, android-27, android-28."
  exit 1
}

function unknown_address_model()
{
  echo "Unknown address model: ${ADDRESS_MODEL}"
  echo "Available ones are: 32, 64."
  exit 1
}

function build_for_ios()
{
  local xcode_ver="$(xcodebuild -version | sed -En 's/^Xcode ([0-9.]+)$/\1/p')"
  local arm64e=$(echo "${xcode_ver}>10.0" | bc -l)
  local archs=("armv7" "armv7s" "arm64" "arm64e")
  local arch="arm"
  local mac=""
  local os=""
  local abi=""
  local define=""

  : ${IOS_ROOT:=/tmp/iphoneos}
  case "${PLATFORM}" in
    "iphoneos")
      arch="arm"
      mac="iphone-$(xcrun --sdk ${PLATFORM} --show-sdk-version 2>/dev/null)"
      os="iphone"
      abi="aapcs"
      define="-D_LITTLE_ENDIAN"
      case "${ADDRESS_MODEL}" in
        64)
          if (( "${arm64e}" )); then
            archs=("arm64" "arm64e")
          else
            archs=("arm64")
          fi
          ;;
        32)
          archs=("armv7" "armv7s")
          ;;
        *) unknown_address_model;;
      esac
      ;;
    "iphonesimulator")
      arch="x86"
      mac="iphonesim-$(xcrun --sdk ${PLATFORM} --show-sdk-version 2>/dev/null)"
      os="darwin"
      abi="sysv"
      archs=("x86_64")
      ADDRESS_MODEL=64
      ;;
    "appletvos")
      arch="arm"
      mac="appletv-$(xcrun --sdk ${PLATFORM} --show-sdk-version 2>/dev/null)"
      os="appletv"
      abi="aapcs"
      define="-D_LITTLE_ENDIAN"
      archs=("arm64")
      ADDRESS_MODEL=64
      ;;
    "appletvsimulator")
      arch="x86"
      mac="appletvsim-$(xcrun --sdk ${PLATFORM} --show-sdk-version 2>/dev/null)"
      os="darwin"
      abi="sysv"
      archs=("x86_64")
      ADDRESS_MODEL=64
      ;;
  esac
  local sysroot="$(xcrun --sdk ${PLATFORM} --show-sdk-platform-path 2>/dev/null)/Developer"
  local compiler="$(xcrun -sdk ${PLATFORM} -f clang++ 2>/dev/null)"
  local cxxflags="-std=c++17 ${define} $(printf -- ' -arch %s' ${archs[@]})"

  cat > project-config.jam <<EOF
using darwin :
: $compiler $cxxflags
: <root>${sysroot}
;

EOF

  ./b2 -d0 -j "${PARALLEL}" --prefix="${IOS_ROOT}" --with-context --with-system \
    architecture="${arch}" macosx-version="${mac}" address-model="${ADDRESS_MODEL}" \
    target-os="${os}" abi="${abi}" variant=release link=static install
}

function build_for_android()
{
  case "${ARCH}" in
    "x86") ADDRESS_MODEL=32;;
    "x86_64") ADDRESS_MODEL=64; ARCH=x86;;
    "arm") ADDRESS_MODEL=32;;
    "arm64") ADDRESS_MODEL=64; ARCH=arm;;
    *) echo "ARCH is unrecognized"; exit 1;;
  esac

  cat > project-config.jam <<EOF
using clang :
: ${CXX} -std=c++17
;

EOF

  ./b2 -d0 -j "${PARALLEL}" --prefix="${SYSROOT}" --with-context --with-system \
    architecture=${ARCH} address-model="${ADDRESS_MODEL}" \
    target-os=android abi=aapcs variant=release link=static install
}

# Main
set -o errexit

[ $# -ge 1 ] || (echo "Usage: boost.sh <src path>" && exit 1)
: ${PARALLEL:=4}
: ${ADDRESS_MODEL:=64}
: ${CXX:=clang++}
[ "${ADDRESS_MODEL}" -eq 64 ] || [ "${ADDRESS_MODEL}" -eq 32 ] || unknown_address_model

cd "$1"
[ -x "b2" ] || ./bootstrap.sh

case "${PLATFORM}" in
  "iphoneos") build_for_ios;;
  "iphonesimulator") build_for_ios;;
  "appletvos") build_for_ios;;
  "appletvsimulator") build_for_ios;;
  android-*) build_for_android;;
  *) unknown_platform;;
esac

echo "-----------------------------------"
echo "               DONE"
echo "-----------------------------------"
