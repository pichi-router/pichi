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
  echo "    -r ndk_root   : specifiy Android NDK root, mandatory for android"
  echo "    -l api_level  : settings.os.api_level, mandatory for android"
  exit 1
}

generate_default_profile()
{
  if [ -f "${HOME}/.conan/profiles/default" ]; then
    return
  fi
  conan profile new --detect default
  conan profile remove settings.build_type default
}

copy_if_not_exists()
{
  local src="${recipes}/../profiles"
  local dst="${HOME}/.conan/profiles"
  local profile="$1"
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

disable_shared()
{
  if [ "${shared}" = "True" ]; then
    echo "shared=True is disabled"
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

convert_ndk_recipe_to_path()
{
  local recipe="${1}"
  conan install "${recipe}" >/dev/null
  local pkg=$(conan info --paths -n package_folder "${recipe}" | \
    grep package_folder | \
    awk '{print $NF}')
  echo "${pkg}/bin"
}

detect_ndk_compiler_version()
{
  ${ndk}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang --version | \
    sed -n 's/^.* clang version \([0-9][0-9]*\)\..*$/\1/p'
}

detect_chost()
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

generate_vs_runtime()
{
  local recipe="${1}"
  local arg=""
  if [ -z "${recipe}" ]; then
    arg="-s compiler.runtime=M"
  else
    arg="-s ${recipe}:compiler.runtime=M"
  fi
  if [ "${shared}" = "True" ] && [ -z "${recipe}" ]; then
    arg="${arg}D"
  else
    arg="${arg}T"
  fi
  if [ "${build_type}" = "Debug" ]; then
    arg="${arg}d"
  fi
  echo "${arg}"
}

build()
{
  conan install -b missing \
    -s build_type="${build_type}" \
    -o "*:shared=${shared}" \
    -o "pichi:tls_fingerprint=${fingerprint}" \
    "$@" \
    "pichi/${version}@"
}

build_for_windows()
{
  trap - EXIT
  generate_default_profile
  copy_profile
  args="-pr windows"
  local compiler="$(conan profile get settings.compiler default | tr -d '\r')"
  if [ "${compiler}" = 'Visual Studio' ]; then
    args="${args} $(generate_vs_runtime)"
  fi
  build ${args}
}

build_for_ios()
{
  validate_arch x86 x86_64 armv7 armv7s armv7k armv8 armv8.3
  check_mandatory_arg "-v ios_version" "${ios_ver}"
  disable_shared
  trap - EXIT
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
  validate_arch x86 armv7 armv8
  check_mandatory_arg "-l api_level" "${api_level}"
  check_mandatory_arg "-r ndk_root" "${ndk}"
  disable_shared
  trap - EXIT
  generate_default_profile
  copy_profile

  if echo "${ndk}" | grep -sq '@$'; then
    ndk="$(convert_ndk_recipe_to_path ${ndk})"
  fi

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
    -c "tools.android:ndk_path=${ndk}" \
    -e "PATH=[${ndk}/toolchains/llvm/prebuilt/linux-x86_64/bin]" \
    -e "CONAN_CMAKE_TOOLCHAIN_FILE=${ndk}/build/cmake/android.toolchain.cmake" \
    -e "CC=${cc}" \
    -e "CXX=${cxx}" \
    -e "AS=${as}" \
    -pr android
}

build_for_spec_profile()
{
  trap - EXIT
  generate_default_profile
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

recipes="$(dirname $0)/../recipes"
build_type="Release"
shared="False"
fingerprint="True"
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
      dispatch_args
      ;;
  esac
done
