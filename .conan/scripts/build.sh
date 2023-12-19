#!/bin/sh

usage()
{
  cat <<EOF
Usage:
conan.sh build <-p platform> [-k lockfile] [-d] [-f] [-o] [-s] [platform specific options] <version>
  -p: available platforms are: windows/freebsd/macos/linux/ios/android
        windows:
          * 'windows' profile is used
          * no platform specific options
        freebsd:
          * 'freebsd' profile is used
          * no platform specific options
        macos:
          * 'macos' profile is used
          * no platform specific options
        linux:
          * 'linux' profile is used
          * no platform specific options
        ios:
          * 'ios' profile is used
          * -s is forbidden
          * available archs for ios: x86/x86_64/armv7/armv8/armv8.3/armv7s/armv7k
        android:
          * 'android' profile is used
          * -s is forbidden
          * available archs are x86/x86_64/armv7/armv8
  -d: set build_type=Debug if sepecified, otherwise Release
  -k: pass lockfile to conan if specified, otherwise no lockfile used.
  -o: tls_fingerprint=False is set if specified, otherwise True.
  -s: shared=True is set if specified, otherwise False.
  -f: force to build all dependencies if specified.
  version: pichi version
platform specific options:
  -a arch       : settings.arch, mandatory for ios/android
  -v ios_version: os.version, mandatory for ios
  -r ndk_recipe : specifiy Android NDK recipe, mandatory for android
  -l api_level  : settings.os.api_level, mandatory for android
EOF
  exit 1
}

generate_default_profile()
{
  if ! conan profile path default >/dev/null 2>&1; then
    conan profile detect
  fi
}

copy_if_not_exists()
{
  local src="${recipes}/../profiles"
  local dst="$(conan config home)/profiles"
  local profile="$1"
  mkdir -p "${dst}"
  if [ ! -f "${dst}/${profile}" ]; then
    cp -f "${src}/${profile}" "${dst}/${profile}"
  fi
}

copy_profile()
{
  copy_if_not_exists boost
  copy_if_not_exists "${platform}"
}

verify_arch()
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
    usage
  fi
}

check_mandatory_arg()
{
  if [ -z "$2" ]; then
    echo "$1 is mandatory"
    usage
  fi
}

get_ndk_pid()
{
  conan list -p os=Linux --format=json "${ndk}#latest:*" | \
    jq -r "first(.\"Local Cache\".\"${ndk}\".revisions[]) | .packages | keys[0]" 2>/dev/null | \
    grep -v '^null$'
}

get_ndk_root()
{
  if [ -z "$(get_ndk_pid)" ]; then
    conan download -p os=Linux -r conancenter "${ndk}" >/dev/null
  fi
  echo "$(conan cache path ${ndk}#latest:$(get_ndk_pid))/bin"
}

detect_ndk_compiler_version()
{
  "${1}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" --version | \
    sed -n 's/^.* clang version \([0-9][0-9]*\)\..*$/\1/p'
}

generate_android_profile()
{
  local ndk_root="$(get_ndk_root)"
  local cxxlib="c++"
  if [ "${shared}" = "True" ]; then
    cxxlib="${cxxlib}_shared"
  else
    cxxlib="${cxxlib}_static"
  fi

  cat <<EOF
include(boost)
[settings]
arch=${arch}
os=Android
os.api_level=${api_level}
compiler=clang
compiler.version=$(detect_ndk_compiler_version ${ndk_root})
compiler.libcxx=${cxxlib}
[options]"
pichi/*:build_test=False
pichi/*:build_server=False
[tool_requires]
${ndk}
[conf]
tools.android:ndk_path=${ndk_root}
EOF
}

build()
{
  conan create --version "${version}" \
    ${lockfile} \
    -b "${build_deps}" \
    -s "build_type=${build_type}" \
    -o "*:shared=${shared}" \
    -o "pichi/*:tls_fingerprint=${fingerprint}" \
    "$@" \
    "${code_root}"
}

build_for_windows()
{
  generate_default_profile
  copy_profile
  args="-pr windows -o mbedtls/*:shared=False -s mbedtls/*:compiler.runtime=static"
  local compiler="$(conan profile show -pr default | \
                      sed -n 's/^compiler=\(.*\)$/\1/p' | \
                      head -1 | tr -d '\r')"
  if [ "${compiler}" = 'msvc' ]; then
    if [ "${shared}" = "True" ]; then
      args="${args} -s compiler.runtime=dynamic"
    else
      args="${args} -s compiler.runtime=static"
    fi
    args="${args} -s compiler.runtime_type=${build_type}"
  fi
  build ${args}
}

build_for_ios()
{
  verify_arch x86 x86_64 armv7 armv7s armv7k armv8 armv8.3
  check_mandatory_arg "-v ios_version" "${ios_ver}"
  generate_default_profile
  copy_profile
  local sdk="iphoneos"
  if [ "${arch:0:3}" = "x86" ]; then
    sdk="iphonesimulator"
  fi
  build -s "os=iOS" -s "arch=${arch}" -s "os.sdk=${sdk}" -s "os.version=${ios_ver}" -pr ios
}

build_for_android()
{
  verify_arch x86 x86_64 armv7 armv8
  check_mandatory_arg "-l api_level" "${api_level}"
  check_mandatory_arg "-r ndk_root" "${ndk}"
  copy_if_not_exists boost
  generate_default_profile
  generate_android_profile > "$(conan config home)/profiles/android"

  build -pr android
}

build_for_spec_profile()
{
  generate_default_profile
  copy_profile
  build -pr "${platform}"
}

dispatch_args()
{
  check_mandatory_arg version "${version}"
  case "${platform}" in
    windows) build_for_windows;;
    freebsd) build_for_spec_profile;;
    macos) build_for_spec_profile;;
    linux) build_for_spec_profile;;
    ios) build_for_ios;;
    android) build_for_android;;
    *) usage;;
  esac
}

set -o errexit

code_root="$(dirname $0)/../.."
recipes="$(dirname $0)/../recipes"
build_deps="missing"
build_type="Release"
shared="False"
fingerprint="True"
platform="unspecified"

trap usage EXIT
args=`getopt a:dfk:l:op:r:sv: $*`
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
    -f)
      shift
      build_deps="*"
      ;;
    -k)
      shift
      lockfile="--lockfile-partial -l ${1}"
      shift
      ;;
    -l)
      shift
      api_level="$1"
      shift
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
    -o)
      shift
      fingerprint="False"
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
      version="$1"
      ;;
  esac
done

trap - EXIT
dispatch_args
