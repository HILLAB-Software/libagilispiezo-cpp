#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "agilispiezo::agilispiezo" for configuration ""
set_property(TARGET agilispiezo::agilispiezo APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(agilispiezo::agilispiezo PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libagilispiezo.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS agilispiezo::agilispiezo )
list(APPEND _IMPORT_CHECK_FILES_FOR_agilispiezo::agilispiezo "${_IMPORT_PREFIX}/lib/libagilispiezo.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
