#!/bin/bash
set -e

BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "[cachysearch] Configurando build..."
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
elif [ "CMakeLists.txt" -nt "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "[cachysearch] Reconfigurando (CMakeLists.txt modificado)..."
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi

echo "[cachysearch] Compilando..."
cmake --build "$BUILD_DIR" -j"$(nproc)"
echo "[cachysearch] Build concluido."

echo "[cachysearch] Iniciando CachyOS Unified Search GUI..."
exec "$BUILD_DIR/cachysearch_gui"
