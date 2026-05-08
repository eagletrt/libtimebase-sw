#!/bin/bash

# Get filename input
read -p "Enter filename (without extension): " filename

# Create platformio.ini
cat > platformio_example.ini << EOF
[platformio]
src_dir = examples
build_dir = .pio/build

[env:native]
platform = native

lib_deps =
    Unity
    ./

build_flags =
    -Itest/include

build_src_filter =
    +<${filename}.c>
EOF

echo "Created platformio_example.ini"

# Build
platformio run -e native --project-conf platformio_example.ini 

echo "Build completed"

# Run executable
./.pio/build/native/program

echo "cleaning up..."

rm platformio_example.ini