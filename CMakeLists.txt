cmake_minimum_required(VERSION 3.10)
project(agilispiezo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/agilispiezo.cpp
    src/serial.cpp
)

set(HEADERS
    include/${PROJECT_NAME}/agilispiezo.h
    include/${PROJECT_NAME}/serial.h
)

find_package(Threads REQUIRED)

find_package(asio CONFIG QUIET)
if(NOT asio_FOUND)
  find_path(ASIO_INCLUDE_DIR asio.hpp
    PATHS
      /usr/include
      /usr/local/include
      $ENV{ASIO_ROOT}/include
  )
  if(NOT ASIO_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find Asio headers. Install via vcpkg (vcpkg install asio) or set ASIO_ROOT.")
  endif()
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES
    DEBUG_POSTFIX "d"
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}
)

if(asio_FOUND)
  target_link_libraries(${PROJECT_NAME} PUBLIC asio::asio)
else()
  target_include_directories(${PROJECT_NAME} PUBLIC ${ASIO_INCLUDE_DIR})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

if(NOT MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

option(AGILISPIEZO_BUILD_EXAMPLES "Build libagilispiezo examples" OFF)
if(AGILISPIEZO_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${INSTALL_CONFIGDIR}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)
