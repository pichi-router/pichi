#/bin/sh

usage()
{
  echo "Usage:"
  echo "  build.sh <-p platform> [-d] [-o] [-s] [platform specific options] <version>"
  echo "    -p: available platforms are: windows/freebsd/macos/linux/ios/android"
  echo "          windows:"
  echo "            * 'windows' profile is used"
  echo "            * no platform specific options"
  echo "          freebsd:"
  echo "            * 'freebsd' profile is used"
  echo "            * no platform specific options"
  echo "          macos:"
  echo "            * 'macos' profile is used"
  echo "            * no platform specific options"
  echo "          linux:"
  echo "            * 'linux' profile is used"
  echo "            * no platform specific options"
  echo "          ios:"
  echo "            * 'ios' profile is used"
  echo "            * -s is forbidden"
  echo "            * available archs for ios: x86/x86_64/armv7/armv8/armv8.3/armv7s/armv7k"
  echo "          android:"
  echo "            * 'android' profile is used"
  echo "            * -s is forbidden"
  echo "            * available archs are x86/x86_64/armv7/armv7hf/armv8"
  echo "    -d: set build_type=Debug if sepecified, otherwise Release"
  echo "    -o: tls_fingerprint=False is set if specified, otherwise True."
  echo "    -s: shared=True is set if specified, otherwise False."
  echo "    version: pichi version"
  echo "  platform specific options:"
  echo "    -a arch       : settings.arch, mandatory for ios/android"
  echo "    -v ios_version: os.version, mandatory for ios"
  echo "    -r ndk_recipe : specifiy Android NDK recipe, mandatory for android"
  echo "    -l api_level  : settings.os.api_level, mandatory for android"
  exit 1
}

export_deps()
{
  for dep in "$@"; do
    if conan list "${dep}" | grep -sq 'ERROR'; then
      local name=$(echo "${dep}" | cut -d / -f 1)
      local ver=$(echo "${dep}" | cut -d / -f 2)
      conan export --name "${name}" --version "${ver}" "${recipes}/${name}"
    fi
  done
}

download_deps()
{
  for dep in "$@"; do
    if conan list "${dep}" | grep -sq 'ERROR'; then
      conan download --only-recipe -r conancenter "${dep}"
    fi
  done
}

handle_deps()
{
  download_deps ${downloading}
  export_deps ${exporting}
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

validate_arch()
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

check_mandatory_arg()
{
  if [ -z "$2" ]; then
    echo "$1 is mandatory"
    false
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

  echo "include(boost)"
  echo "[settings]"
  echo "arch=${arch}"
  echo "os=Android"
  echo "os.api_level=${api_level}"
  echo "compiler=clang"
  echo "compiler.version=$(detect_ndk_compiler_version ${ndk_root})"
  if [ "${shared}" = "True" ]; then
    echo "compiler.libcxx=c++_shared"
  else
    echo "compiler.libcxx=c++_static"
  fi
  echo "[options]"
  echo "pichi/*:build_test=False"
  echo "pichi/*:build_server=False"
  echo "[tool_requires]"
  echo "${ndk}"
  echo "[conf]"
  echo "tools.android:ndk_path=${ndk_root}"
}

build()
{
  conan create --version "${version}" -b missing \
    -s "build_type=${build_type}" \
    -o "*:shared=${shared}" \
    -o "pichi/*:tls_fingerprint=${fingerprint}" \
    "$@" \
    "${code_root}"
}

build_for_windows()
{
  trap - EXIT
  handle_deps
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
  validate_arch x86 x86_64 armv7 armv7s armv7k armv8 armv8.3
  check_mandatory_arg "-v ios_version" "${ios_ver}"
  trap - EXIT
  generate_default_profile
  copy_profile
  handle_deps
  local sdk="iphoneos"
  if [ "${arch:0:3}" = "x86" ]; then
    sdk="iphonesimulator"
  fi
  build -s "os=iOS" -s "arch=${arch}" -s "os.sdk=${sdk}" -s "os.version=${ios_ver}" -pr ios
}

build_for_android()
{
  validate_arch x86 armv7 armv8
  check_mandatory_arg "-l api_level" "${api_level}"
  check_mandatory_arg "-r ndk_root" "${ndk}"

  trap - EXIT
  handle_deps
  copy_if_not_exists boost
  generate_default_profile
  generate_android_profile > "$(conan config home)/profiles/android"

  build -pr android
}

build_for_spec_profile()
{
  trap - EXIT
  generate_default_profile
  handle_deps
  copy_profile
  build -pr "${platform}"
}

dispatch_args()
{
  case "${platform}" in
    windows) build_for_windows;;
    freebsd) build_for_spec_profile;;
    macos) build_for_spec_profile;;
    linux) build_for_spec_profile;;
    ios) build_for_ios;;
    android) build_for_android;;
    *) false;;
  esac
}

set -o errexit

code_root="$(dirname $0)/../.."
recipes="$(dirname $0)/../recipes"
build_type="Release"
shared="False"
fingerprint="True"
platform="unspecified"
exporting="libmaxminddb/1.8.0"
downloading="boost/1.83.0 libsodium/1.0.18 mbedtls/3.5.0 rapidjson/1.1.0"

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
      check_mandatory_arg "version" "$1"
      version="$1"
      if [ "${fingerprint}" = "True" ]; then
        downloading="${downloading} brotli/1.1.0"
        exporting="${exporting} boringssl/27"
      else
        exporting="${exporting} openssl/3.2.0"
      fi
      dispatch_args
      ;;
  esac
done
