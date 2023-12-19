#!/bin/sh

usage()
{
  cat <<EOF
Usage:
conan.sh export [-k lockfile] [-v version] [-h] <recipe name>
  -k: specify the recipe version contained within the lockfile
  -v: specify the recipe version
NOTICE:
  * -k/-v must specify one
  * -v would be ignored if -k is specified
EOF
  exit 1
}

# Main
set -o errexit
recipes="$(dirname $0)/../recipes"

trap usage EXIT
args="$(getopt k:v: $*)"
set -- ${args}
for i; do
  case "$i" in
    -k)
      shift
      lockfile="$1"
      shift
      ;;
    -v)
      shift
      version="$1"
      shift
      ;;
    --)
      shift
      recipe="$1"
      [ -n "${recipe}" ]
      [ -n "${version}" ] || [ -n "${lockfile}" ]
  esac
done

trap - EXIT
if [ -n "${lockfile}" ]; then
  version=$(jq -r ".requires[] | select(test(\"^${recipe}/\")) | sub(\"^[^/]*/\"; \"\")" < \
    "${lockfile}")
fi
conan export --version "${version}" "${recipes}/${recipe}"
