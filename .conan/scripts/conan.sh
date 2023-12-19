#!/bin/sh

usage()
{
  cat <<EOF
Usage:
conan.sh <sub command> [options]
available sub commands are:
  build : Build pichi with conan
  export: Export recipe inside .conan/recipes
  path  : Show the path to the conan cache for given options
EOF
  exit 1
}

# Main
if [ "$#" -lt 1 ]; then
  usage
fi

scripts="$(dirname ${0})"

case "${1}" in
  build)
    shift
    "${scripts}/build.sh" "$@"
    ;;
  path)
    shift
    "${scripts}/path.sh" "$@"
    ;;
  export)
    shift
    "${scripts}/export.sh" "$@"
    ;;
  *) usage;;
esac
