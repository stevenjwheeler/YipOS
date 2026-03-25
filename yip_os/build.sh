#!/bin/bash
set -e
cd "$(dirname "$0")"

# CTranslate2 prefix (for NLLB translation support)
# Override with: CT2_PREFIX=/path/to/ct2 ./build.sh configure
# Priority: user-set CT2_PREFIX > CUDA build > CPU-only build > none

if [ -z "$CT2_PREFIX" ]; then
    if [ -d "/tmp/ct2_install/lib64/cmake/ctranslate2" ]; then
        CT2_PREFIX="/tmp/ct2_install"
    elif [ -d "/tmp/ct2_install_cpu/lib64/cmake/ctranslate2" ]; then
        CT2_PREFIX="/tmp/ct2_install_cpu"
    fi
fi

CMAKE_EXTRA=""
if [ -n "$CT2_PREFIX" ] && [ -d "$CT2_PREFIX/lib64/cmake/ctranslate2" ]; then
    CMAKE_EXTRA="-DCMAKE_PREFIX_PATH=$CT2_PREFIX"
    # Detect CUDA vs CPU-only
    if ls "$CT2_PREFIX"/lib64/libcublas* &>/dev/null; then
        echo "CTranslate2 found at $CT2_PREFIX (CUDA)"
    else
        echo "CTranslate2 found at $CT2_PREFIX (CPU-only)"
    fi
else
    echo "CTranslate2 not found — translation will be disabled"
fi

case "${1:-build}" in
    clean)
        rm -rf build
        echo "Cleaned."
        ;;
    configure)
        distrobox enter my-distrobox -- cmake -B build -DCMAKE_BUILD_TYPE=Debug $CMAKE_EXTRA
        ;;
    build)
        distrobox enter my-distrobox -- cmake --build build -j"$(nproc)"
        ;;
    run)
        LD_LIBRARY_PATH="$CT2_PREFIX/lib64:${LD_LIBRARY_PATH:-}" \
            ./build/yip_os
        ;;
    rebuild)
        rm -rf build
        distrobox enter my-distrobox -- cmake -B build -DCMAKE_BUILD_TYPE=Debug $CMAKE_EXTRA
        distrobox enter my-distrobox -- cmake --build build -j"$(nproc)"
        ;;
    *)
        echo "Usage: $0 {clean|configure|build|run|rebuild}"
        exit 1
        ;;
esac
