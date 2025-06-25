#!/bin/bash

set -e

readonly TOOLCHAIN_AARCH64="/usr/xcc/aarch64-unknown-linux-gnu/Toolchain.cmake"
HARWARE_CONCURRENCY=$(nproc)
readonly HARWARE_CONCURRENCY

PrintUsage() {
    echo ""
    echo " Usage:"
    echo ""
    echo "  ./build.sh cross"
    echo "         Build with $TOOLCHAIN_AARCH64 toolchain"
    echo ""
    echo "  ./build.sh native"
    echo "        Native Debug build"
    echo ""
    echo "  ./build.sh nativ-rel"
    echo "         Native Release build"
    echo ""
}

generator="Unix Makefiles"
build_type=Debug
build_dir=build
extra_args=()

if command -v ninja >&2; then
    generator=Ninja
fi

case "$1" in
cross)
    build_type=Release
    build_dir=build_aarch64
    extra_args+=("-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_AARCH64}")
    ;;
native)
    build_type=Debug
    ;;
native-rel)
    build_type=Release
    ;;
*)
    PrintUsage
    exit 1
    ;;
esac

git config --global --add safe.directory $(pwd)
cmake -B$build_dir -DCMAKE_BUILD_TYPE=$build_type -G "$generator" "${extra_args[@]}" -H.
cmake --build $build_dir -j"${HARWARE_CONCURRENCY}"
exit $?
