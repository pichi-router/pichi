#!/bin/bash

function unknown_platform()
{
  echo "Unknown platform: ${PLATFORM}"
  echo "Available platforms are"
  echo "  iOS    : iOS, tvOS, watchOS"
  echo "  Android: android-* (depends on the NDK version)"
  exit 1
}

function unknown_arch_for_ios()
{
  echo "Valid ARCHs for iOS platform are"
  echo "  device:    armv7, armv7s, arm64, arm64e"
  echo "  simulator: i386, x86_64"
  exit 1
}

function unknown_arch_for_tvos()
{
  echo "Valid ARCHs for tvOS platform are"
  echo "  device:    arm64"
  echo "  simulator: x86_64"
  exit 1
}

function unknown_arch_for_watchos()
{
  echo "Valid ARCHs for watchOS platform are"
  echo "  device:    arm64_32, armv7k"
  echo "  simulator: i386"
  exit 1
}

function build_for_ios()
{
  local arch=""
  local mac=""
  local os=""
  local abi=""
  local define=""

  if [[ "${ARCH}" = arm* ]]; then
    arch="arm"
    abi="aapcs"
    define="-D_LITTLE_ENDIAN"
  else
    arch="x86"
    abi="sysv"
  fi

  case "${PLATFORM}" in
    "iOS")
      case "${ARCH}" in
        arm*)
          os="iphone"
          sdk="iphoneos"
          mac="iphone-$(xcrun --sdk ${sdk} --show-sdk-version 2>/dev/null)"
          if [ "${ARCH}" = "armv7" ] || [ "${ARCH}" = "armv7s" ]; then
            address_model="32"
          elif [ "${ARCH}" = "arm64" ] || [ "${ARCH}" = "arm64e" ]; then
            address_model="64"
          else
            unknown_arch_for_ios
          fi
          ;;
        *)
          os="darwin"
          sdk="iphonesimulator"
          mac="iphonesim-$(xcrun --sdk ${sdk} --show-sdk-version 2>/dev/null)"
          if [ "${ARCH}" = "i386" ]; then
            address_model="32"
          elif [ "${ARCH}" = "x86_64" ]; then
            address_model="64"
          else
            unknown_arch_for_ios
          fi
          ;;
      esac
      ;;
    "tvOS")
      address_model="64"
      if [ "${ARCH}" = "arm64" ]; then
        os="appletv"
        sdk="appletvos"
        mac="appletv-$(xcrun --sdk ${sdk} --show-sdk-version 2>/dev/null)"
      elif [ "${ARCH}" = "x86_64" ]; then
        os="darwin"
        sdk="appletvsimulator"
        mac="appletvsim-$(xcrun --sdk ${sdk} --show-sdk-version 2>/dev/null)"
      else
        unknown_arch_for_tvos
      fi
      ;;
    *) unknown_platform;;
  esac
  local sysroot="$(xcrun --sdk ${sdk} --show-sdk-platform-path 2>/dev/null)/Developer"
  local compiler="$(xcrun -sdk ${sdk} -f clang++ 2>/dev/null)"
  local cxxflags="-std=c++17 ${define} -arch ${ARCH}"

  cat > project-config.jam <<EOF
using darwin :
: $compiler $cxxflags
: <striper> <root>${sysroot}
;

EOF

  ./b2 -d0 -j "${PARALLEL}" --prefix="${SYSROOT}" --with-context --with-system \
    architecture="${arch}" macosx-version="${mac}" address-model="${address_model}" \
    target-os="${os}" abi="${abi}" variant=release link=static install
}

function build_for_android()
{
  case "${ARCH}" in
    "x86") address_model=32;;
    "x86_64") address_model=64; ARCH=x86;;
    "arm") address_model=32;;
    "arm64") address_model=64; ARCH=arm;;
    *) echo "ARCH is unrecognized"; exit 1;;
  esac

  cat > project-config.jam <<EOF
using clang :
: ${SYSROOT}/../bin/clang++ -std=c++17
;

EOF

  file b2
  ./b2 -d0 -j "${PARALLEL}" --prefix="${SYSROOT}" --with-context --with-system \
    architecture=${ARCH} address-model="${address_model}" \
    target-os=android abi=aapcs variant=release link=static install
}

# Main
set -o errexit

[ $# -ge 1 ] || (echo "Usage: boost.sh <src path>" && exit 1)
: ${PARALLEL:=4}

cd "$1"
[ -x "b2" ] || ./bootstrap.sh

case "${PLATFORM}" in
  "iOS") build_for_ios;;
  "tvOS") build_for_ios;;
  android-*) build_for_android;;
  *) unknown_platform;;
esac

echo "-----------------------------------"
echo "               DONE"
echo "-----------------------------------"
