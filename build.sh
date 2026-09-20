#!/bin/bash

# Build script for DoomNanoCE port to TI-84+CE

echo "Building DoomNanoCE for TI-84+CE..."

# Clean previous builds
echo "Cleaning previous build artifacts..."
make clean

# Build the project
echo "Building project..."
make

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build completed successfully!"
    echo "Output file: DoomNanoCE.8xp"
else
    echo "Build failed!"
    exit 1
fi

echo "Build process finished."