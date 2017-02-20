# - try to find twine library
#
# Cache Variables: (probably not for direct use in your scripts)
#  TWINE_INCLUDEDIR
#  TWINE_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  TWINE_FOUND
#  TWINE_INCLUDE_DIRS
#  TWINE_LIBRARIES
#
# Requires these CMake modules:
#  SelectLibraryConfigurations (included with CMake >= 2.8.0)
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Based off Findcppunit.cmake

# First, try pkg-config
include(FindPkgConfig)
pkg_check_modules(TWINE "twine>=${twine_FIND_VERSION}")

# Search in this directory if pkg-config found nothing
set(TWINE_ROOT_DIR
  "${TWINE_ROOT_DIR}"
  CACHE
  PATH
  "Directory to search for twine")

if (TWINE_FOUND)
  # We need to emulate find_library's _RELEASE and _DEBUG libraries, even on
  # other platforms.
  set(TWINE_LIBRARY_DEBUG "${TWINE_LIBRARIES}")
  set(TWINE_LIBRARY_RELEASE "${TWINE_LIBRARIES}")
else()
  find_library(TWINE_LIBRARY_RELEASE
    NAMES
    twine
    HINTS
    "${TWINE_ROOT_DIR}/lib")

  find_library(TWINE_LIBRARY_DEBUG
    NAMES
    twined
    HINTS
    "${TWINE_ROOT_DIR}/lib")

  include(SelectLibraryConfigurations)
  select_library_configurations(TWINE)

  # Might want to look close to the library first for the includes.
  get_filename_component(_libdir "${TWINE_LIBRARY_RELEASE}" PATH)

  find_path(TWINE_INCLUDE_DIR
    NAMES
    twine/twine.h
    PATHS
    "${TWINE_ROOT_DIR}"
    PATH_SUFFIXES
    include/)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(twine
    DEFAULT_MSG
    TWINE_LIBRARY
    TWINE_INCLUDE_DIR)

  if(TWINE_FOUND)
    set(TWINE_INCLUDE_DIRS "${TWINE_INCLUDE_DIR}")
  endif()

  mark_as_advanced(TWINE_INCLUDE_DIR
    TWINE_LIBRARY_RELEASE
    TWINE_LIBRARY_DEBUG)
endif()
message("${TWINE_INCLUDEDIR}")
message("${TWINE_LIBRARY}")
message("${TWINE_LIBRARY_DEBUG}")
message("${TWINE_LIBRARY_RELEASE}")
message("${TWINE_FOUND}")
message("${TWINE_INCLUDE_DIRS}")
message("${TWINE_LIBRARIES}")
