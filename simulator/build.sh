#!/bin/sh
# Build script for T4-S3 simulator
# This script should be run from the simulator directory

set -e

# Get the simulator directory (where this script is located)
SIMULATOR_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "======================================"
echo "  T4-S3 Simulator - Building"
echo "======================================"

cd "$SIMULATOR_DIR"

# Detect number of CPU cores (Windows-compatible)
if [ -n "$NUMBER_OF_PROCESSORS" ]; then
    CORES=$NUMBER_OF_PROCESSORS
else
    CORES=$(nproc 2>/dev/null || echo 4)
fi

# Step 0: Delete old binary if it exists
if [ -f "bin/main" ]; then
    echo "[0/5] Removing old executable bin/main..."
    rm -f bin/main
fi

# Step 1: Clean old build if requested OR if cross-platform conflict detected
NEED_CLEAN=false
if [ "$1" = "clean" ]; then
    NEED_CLEAN=true
    echo "[1/5] Cleaning old build (requested)..."
elif [ -f "build/CMakeCache.txt" ]; then
    # Check if build was created on a different platform
    CURRENT_DIR=$(pwd)
    CACHE_DIR=$(grep "CMAKE_HOME_DIRECTORY:INTERNAL=" build/CMakeCache.txt 2>/dev/null | cut -d= -f2)

    # Normalize paths for comparison
    # Convert both to lowercase and handle both Windows (C:/) and MSYS (/c/) formats
    CURRENT_NORMALIZED=$(echo "$CURRENT_DIR" | tr '\\' '/' | tr 'A-Z' 'a-z' | sed 's|^/\([a-z]\)/|\1:/|')
    CACHE_NORMALIZED=$(echo "$CACHE_DIR" | tr '\\' '/' | tr 'A-Z' 'a-z' | sed 's|^/\([a-z]\)/|\1:/|')

    if [ -n "$CACHE_DIR" ] && [ "$CURRENT_NORMALIZED" != "$CACHE_NORMALIZED" ]; then
        NEED_CLEAN=true
        echo "[1/5] Cleaning old build (cross-platform conflict detected)..."
        echo "  Old path: $CACHE_DIR"
        echo "  New path: $CURRENT_DIR"
    fi
fi

if [ "$NEED_CLEAN" = true ]; then
    rm -rf build bin
fi

# Step 2: Create build directory
echo "[2/5] Creating build directory..."
mkdir -p build
cd build

# Step 3: Configure with CMake (Release mode)
echo "[3/5] Configuring CMake (Release mode with optimizations)..."
cmake -DCMAKE_BUILD_TYPE=Release ..

# Step 4: Build using all CPU cores
echo "[4/5] Building with -j$CORES (using all cores)..."
cmake --build . --config Release --parallel $CORES

echo ""
echo "======================================"
echo "  Build Complete!"
echo "======================================"

cd "$SIMULATOR_DIR"
