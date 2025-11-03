#!/bin/sh
# Run script for T4-S3 simulator
# This script should be run from the simulator directory

set -e

# Get the simulator directory (where this script is located)
SIMULATOR_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "======================================"
echo "  T4-S3 Simulator - Running"
echo "======================================"

cd "$SIMULATOR_DIR"

# Run the project
if [ -f "bin/main" ]; then
    echo "Running project..."
    ./bin/main
elif [ -f "bin/main.exe" ]; then
    echo "Running project..."
    ./bin/main.exe
else
    echo "Executable not found at ./bin/main"
    echo "Please build the project first."
    exit 1
fi

echo "======================================"
echo "  Done"
echo "======================================"

cd "$SIMULATOR_DIR"
