#!/bin/sh

usage()
{
  cat <<EOF
Usage:
conan.sh path [-p platform] [-d] [-o] [-s] [platform specific options] <version>
  -d: set build_type=Debug if sepecified, otherwise Release.
  -o: tls_fingerprint=False is set if specified, otherwise True.
  -p: available platforms are: windows/freebsd/macos/linux/ios/android.
  -s: shared=True is set if specified, otherwise False.
  version: pichi version
platform specific options:
  -a arch       : settings.arch
  -v ios_version: os.version
  -l api_level  : settings.os.api_level
EOF
  exit 1
}

check_mandatory_arg()
{
  if [ -z "$2" ]; then
    echo "$1 is mandatory"
    false
  fi
}

find_pkg()
{
  conan list -f json 'pichi:*' | \
    jq -r "\
      first(.\"Local Cache\".\"pichi/${version}\".revisions[]).packages | to_entries | .[] \
      | select(.value.info.settings.build_type == \"${build_type}\") \
      | select(.value.info.options.shared == \"${shared}\") \
      | select(.value.info.options.tls_fingerprint == \"${fingerprint}\") \
      ${selector} \
      | .key"
}

set -o errexit

code_root="$(dirname $0)/../.."
build_type="Release"
shared="False"
fingerprint="True"
selector=""

trap usage EXIT
args=`getopt a:dl:op:r:sv: $*`
set -- $args
for i; do
  case "$i" in
    -a)
      shift
      selector="${selector} | select(.value.info.settings.arch == \"${1}\")"
      shift
      ;;
    -d)
      shift
      build_type="Debug"
      ;;
    -l)
      shift
      selector="${selector} | select(.value.info.settings.\"os.api_level\" == \"${1}\")"
      shift
      ;;
    -p)
      shift
      case "${1}" in
        windows) platform="Windows";;
        freebsd) platform="FreeBSD";;
        macos) platform="Macos";;
        linux) platform="Linux";;
        ios) platform="iOS";;
        android) platform="Android";;
        *) false;;
      esac
      selector="${selector} | select(.value.info.settings.os == \"${platform}\")"
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
      selector="${selector} | select(.value.info.settings.\"os.version\" == \"${1}\")"
      shift
      ;;
    --)
      shift
      check_mandatory_arg "version" "$1"
      version="${1}"
      ;;
  esac
done

trap - EXIT

for pkg in $(find_pkg); do
  conan cache path "pichi/${version}:${pkg}"
done
