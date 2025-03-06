# Examples for libagilispiezo

# Basic example
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE agilispiezo)

# Install examples
install(TARGETS basic_example
    RUNTIME DESTINATION bin/examples
)

# Copy example source files to installation
install(FILES
    basic_example.cpp
    DESTINATION share/agilispiezo/examples
)

# README for examples
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/examples_readme.md
"# AgilisPiezo Library Examples

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
")

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/examples_readme.md
    DESTINATION share/agilispiezo/examples
    RENAME README.md
)