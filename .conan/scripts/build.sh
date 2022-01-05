#/usr/bin/env bash

function usage()
{
  echo "Usage:"
  echo "  build.sh [-p platform] [-d] [-o] [-s] [platform specific options] <version>"
  echo "    -p: available platforms are: windows/freebsd/ios/android"
  echo "          windows:"
  echo "            * 'default' profile is used"
  echo "            * always building libressl if "
  echo "            * -s is forbidden"
  echo "            * no platform specific options"
  echo "          freebsd:"
  echo "            * 'freebsd' profile is used"
  echo "            * no platform specific options"
  echo "          ios:"
  echo "            * 'ios' profile is used"
  echo "            * -s is forbidden"
  echo "            * available archs for ios: x86/x86_64/armv7/armv8/armv8.3/armv7s/armv7k"
  echo "          android:"
  echo "            * 'android' profile is used"
  echo "            * -s is forbidden"
  echo "            * available archs are x86/x86_64/armv7/armv7hf/armv8"
  echo "          otherwise or unspecified:"
  echo "            * 'default' profile is used"
  echo "            * no platform specific options"
  echo "    -d: set build_type=Debug if sepecified, otherwise Release"
  echo "    -o: depends on openssl if specified, otherwise libressl"
  echo "    -s: shared=True is set if specified, otherwise False."
  echo "    version: pichi version"
  echo "  platform specific options:"
  echo "    -a arch       : settings.arch, mandatory for ios/android"
  echo "    -v ios_version: os.version, mandatory for ios"
  echo "    -r ndk_root   : specifiy Android NDK root, mandatory for android"
  echo "    -l api_level  : settings.os.api_level, mandatory for android"
  exit 1
}

function generate_default_profile()
{
  if [ -f "${HOME}/.conan/profiles/default" ]; then
    return
  fi
  conan profile new --detect default
  conan profile remove settings.build_type default
  if [ "$(conan profile get settings.compiler default | tr -d '\r')" = 'gcc' ]; then
    conan profile update settings.compiler.libcxx=libstdc++11 default
  fi
}

function copy_profile()
{
  local profile="${platform}"
  local src="${recipes}/../profiles"
  local dst="${HOME}/.conan/profiles"
  if [ ! -f "${dst}/${profile}" ]; then
    cp -f "${src}/${profile}" "${dst}/${profile}"
  fi
}

function validate_arch()
{
  local matched=""
  for i; do
    if [ "${arch}" = "${1}" ]; then
      matched="true"
      break
    fi
    shift
  done
  if [ -z "${matched}" ]; then
    echo "Invalid arch: \"${arch}\""
    false
  fi
}

function disable_shared()
{
  if [ "${shared}" = "True" ]; then
    echo "shared=True is disabled"
    false
  fi
}

function check_mandatory_arg()
{
  if [ -z "$2" ]; then
    echo "$1 is mandatory"
    false
  fi
}

function detect_ndk_compiler_version()
{
  ${ndk}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang --version | \
    sed -n 's/^.* clang version \([0-9][0-9]*\)\..*$/\1/p'
}

function detect_chost()
{
  case "${arch}" in
    x86) echo "i686-linux-android";;
    x86_64) echo "x86_64-linux-android";;
    armv7) echo "armv7a-linux-androideabi";;
    armv7hf) echo "armv7a-linux-androideabi";;
    armv8) echo "aarch64-linux-android";;
    *) false;;
  esac
}

function build()
{
  conan install -b missing \
    -s build_type="${build_type}" \
    -o "*:shared=${shared}" \
    -o "pichi:tls_lib=${tlslib}" \
    "$@" \
    "pichi/${version}@"
}

function build_for_windows()
{
  disable_shared
  trap - EXIT
  generate_default_profile
  local args="-b libressl"
  local compiler="$(conan profile get settings.compiler default | tr -d '\r')"
  if [ "${compiler}" = 'Visual Studio' ]; then
    args="${args} -s compiler.runtime=MT"
    if [ "${build_type}" = "Debug" ]; then
      args="${args}d"
    fi
  fi
  build ${args}
}

function build_for_freebsd()
{
  trap - EXIT
  generate_default_profile
  copy_profile
  build -pr freebsd
}

function build_for_ios()
{
  validate_arch x86 x86_64 armv7 armv7s armv7k armv8 armv8.3
  check_mandatory_arg "-v ios_version" "${ios_ver}"
  disable_shared
  trap - EXIT
  generate_default_profile
  copy_profile
  build -s "os=iOS" -s "arch=${arch}" -s "os.version=${ios_ver}" -pr ios
}

function build_for_android()
{
  validate_arch x86 x86_64 armv7 armv7hf armv8
  check_mandatory_arg "-l api_level" "${api_level}"
  check_mandatory_arg "-r ndk_root" "${ndk}"
  disable_shared
  trap - EXIT
  generate_default_profile
  copy_profile
  local chost="$(detect_chost)"
  local cc="${chost}${api_level}-clang"
  local cxx="${cc}++"
  local as="${chost}-as"
  if [ "${chost}" = "armv7a-linux-androideabi" ]; then
    as="arm-linux-androideabi-as"
  fi
  build -s "compiler.version=$(detect_ndk_compiler_version)" \
    -s "os.api_level=${api_level}" \
    -s "arch=${arch}" \
    -e "PATH=[${ndk}/toolchains/llvm/prebuilt/linux-x86_64/bin]" \
    -e "CONAN_CMAKE_TOOLCHAIN_FILE=${ndk}/build/cmake/android.toolchain.cmake" \
    -e "CC=${cc}" \
    -e "CXX=${cxx}" \
    -e "AS=${as}" \
    -pr android
}

function build_for_default()
{
  trap - EXIT
  generate_default_profile
  build
}

function dispatch_args()
{
  case "${platform}" in
    windows) build_for_windows;;
    freebsd) build_for_freebsd;;
    ios) build_for_ios;;
    android) build_for_android;;
    *) build_for_default;;
  esac
}

set -o errexit

recipes="$(dirname $0)/../recipes"
build_type="Release"
shared="False"
tlslib="libressl"
platform="unspecified"

trap usage EXIT
args=`getopt a:dl:op:r:sv: $*`
set -- $args
for i; do
  case "$i" in
    -a)
      shift
      arch="$1"
      shift
      ;;
    -d)
      shift
      build_type="Debug"
      ;;
    -l)
      shift
      api_level="$1"
      shift
      ;;
    -o)
      shift
      tlslib="openssl"
      ;;
    -p)
      shift
      platform="$1"
      shift
      ;;
    -r)
      shift
      ndk="$1"
      shift
      ;;
    -s)
      shift
      shared="True"
      ;;
    -v)
      shift
      ios_ver="$1"
      shift
      ;;
    --)
      shift
      check_mandatory_arg "version" "$1"
      version="$1"
      dispatch_args
      ;;
  esac
done
