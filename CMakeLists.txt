cmake_minimum_required(VERSION 3.10)
project(libagilispiezo VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define source files
set(SOURCES
    src/agilispiezo.cpp
    src/serial.cpp
)

# Define header files
set(HEADERS
    include/agilispiezo.h
    include/serial.h
)

# Find dependencies
find_package(Threads REQUIRED)

# Find Asio
find_package(Asio QUIET)
if(NOT Asio_FOUND)
  find_path(ASIO_INCLUDE_DIR asio.hpp
    PATHS /usr/include /usr/local/include
  )
  if(NOT ASIO_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find Asio headers. Please install libasio-dev")
  endif()
  include_directories(${ASIO_INCLUDE_DIR})
else()
  include_directories(${Asio_INCLUDE_DIRS})
endif()

# Add include directories
include_directories(include)

# Create the library
add_library(agilispiezo STATIC ${SOURCES} ${HEADERS})
target_include_directories(agilispiezo PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link with necessary libraries
target_link_libraries(agilispiezo PUBLIC Threads::Threads)
target_compile_options(agilispiezo PRIVATE -Wall -Wextra -pedantic)

# Add examples directory
add_subdirectory(examples)

# Installation rules
install(TARGETS agilispiezo
    EXPORT agilispiezoTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/libagilispiezo
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT agilispiezoTargets
    FILE agilispiezoTargets.cmake
    NAMESPACE agilispiezo::
    DESTINATION lib/cmake/agilispiezo
)

# Configure and install package configuration file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/agilispiezoConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/agilispiezoConfig.cmake"
    INSTALL_DESTINATION lib/cmake/agilispiezo
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/agilispiezoConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/agilispiezoConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/agilispiezoConfigVersion.cmake"
    DESTINATION lib/cmake/agilispiezo
)

# Directory structure setup
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/libagilispiezo)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Create a configuration template if it doesn't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/agilispiezoConfig.cmake.in")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/agilispiezoConfig.cmake.in"
        "@PACKAGE_INIT@\n\n"
        "include(\"\${CMAKE_CURRENT_LIST_DIR}/agilispiezoTargets.cmake\")\n"
        "check_required_components(agilispiezo)\n"
    )
endif()