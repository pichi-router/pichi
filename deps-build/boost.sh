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

function build_for_ios()
{
  local xcode_ver="$(xcodebuild -version | sed -En 's/^Xcode ([0-9.]+)$/\1/p')"
  local arm64e=$(echo "${xcode_ver}>10.0" | bc -l)
  local archs=("armv7" "armv7s" "arm64" "arm64e")
  local host="iphone"

  case "${PLATFORM}" in
    "iphoneos")
      host="iphone"
      if (( "${arm64e}" )); then
        archs=("armv7" "armv7s" "arm64" "arm64e")
      else
       archs=("armv7" "armv7s" "arm64")
      fi
      ;;
    "iphonesimulator")
      host="iphonesim"
      archs=("x86_64")
      ;;
    "appletvos")
      host="appletv"
      archs=("arm64")
      ;;
    "appletvsimulator")
      host="appletvsim"
      archs=("x86_64")
      ;;
  esac
  local sysroot="$(xcrun --sdk ${PLATFORM} --show-sdk-platform-path 2>/dev/null)/Developer"
  local host_version="${host}-$(xcrun --sdk ${PLATFORM} --show-sdk-version 2>/dev/null)"
  local compiler="$(xcrun -sdk ${PLATFORM} -f clang++ 2>/dev/null)"
  local cxxflags="-std=c++17 -D_LITTLE_ENDIAN $(printf -- ' -arch %s' ${archs[@]})"

  cat > project-config.jam <<EOF
using darwin :
: $compiler $cxxflags
: <root>${sysroot}
;

EOF

  ./b2 -j "${PARALLEL}" --prefix="${PREFIX}" --with-context --with-system \
    variant=release macosx-version="${host_version}" link=static stage
}

function build_for_android()
{
  # NDK_TOOLCHAIN is generated via ${NDK_ROOT}/build/tools/make_standalone_toolchain.py
  [ -d "${NDK_TOOLCHAIN}" ] || (echo "NDK_TOOLCHAIN is unavailable" && exit 1)

  cat > project-config.jam <<EOF
using clang :
: ${NDK_TOOLCHAIN}/bin/clang++ -std=c++17
;

EOF

  ./b2 -j "${PARALLEL}" --prefix="${NDK_TOOLCHAIN}/sysroot" --with-context --with-system \
    variant=release link=static stage
}

# Main
set -o errexit

[ $# -ge 1 ] || (echo "Usage: boost.sh <src path>" && exit 1)
cd "$1"
./bootstrap.sh

: ${PARALLEL:=4}
: ${PREFIX:=/tmp/ios}
case "${PLATFORM}" in
  "iphoneos") build_for_ios;;
  "iphonesimulator") build_for_ios;;
  "appletvos") build_for_ios;;
  "appletvsimulator") build_for_ios;;
  "android-16") build_for_android;;
  "android-17") build_for_android;;
  "android-18") build_for_android;;
  "android-19") build_for_android;;
  "android-21") build_for_android;;
  "android-22") build_for_android;;
  "android-23") build_for_android;;
  "android-24") build_for_android;;
  "android-26") build_for_android;;
  "android-27") build_for_android;;
  "android-28") build_for_android;;
  *) unknown_platform;;
esac

echo "-----------------------------------"
echo "               DONE"
echo "-----------------------------------"
