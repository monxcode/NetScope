#!/bin/bash
# Build script for NetScope on Linux
# Requirements: CMake 3.20+, GCC 11+ or Clang 14+

set -e

CONFIG="${1:-Release}"

echo "=== NetScope Build Script ==="
echo "Configuration: ${CONFIG}"
echo ""

# Remove old build directory
rm -rf build

# Configure
echo "[1/3] Configuring CMake..."
cmake -B build -DCMAKE_BUILD_TYPE="${CONFIG}" -DCMAKE_CXX_STANDARD=20

# Build
echo "[2/3] Building..."
cmake --build build -j"$(nproc)"

# Run tests
echo "[3/3] Running tests..."
ctest --test-dir build --output-on-failure

echo ""
echo "=== Build complete ==="
echo "Binary: build/bin/netscope"
