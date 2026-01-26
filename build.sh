#!/bin/bash

# Mini-Snowflake Build Script
echo "Building Mini-Snowflake..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
echo "Building..."
make -j$(nproc)

echo "Build complete! Run with: ./mini_snowflake"