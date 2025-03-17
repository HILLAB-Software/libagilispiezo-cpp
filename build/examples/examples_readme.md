# AgilisPiezo Library Examples

This directory contains examples demonstrating how to use the AgilisPiezo library.

## Basic Example

The basic_example demonstrates how to connect to a device, get firmware information,
and perform simple movements.

To compile and run:

```bash
# Compile
mkdir build && cd build
cmake ..
make

# Run
./basic_example /dev/ttyUSB0  # Replace with your device port
```

## Using the Library in Your Own Project

To use the AgilisPiezo library in your own CMake project:

```cmake
find_package(agilispiezo REQUIRED)
add_executable(myapp myapp.cpp)
target_link_libraries(myapp PRIVATE agilispiezo::agilispiezo)
```
