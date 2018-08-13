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

if (NOT TWINE_FOUND)
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
		TWINE_INCLUDE_DIR)

	if(TWINE_FOUND)
		find_library(TWINE_LIBRARY_RELEASE
			NAMES
			twine
			HINTS
			"${TWINE_ROOT_DIR}/lib")
		if (NOT TWINE_LIBRARY_RELEASE)
			set(TWINE_LIBRARY_RELEASE "")
		endif()

		find_library(TWINE_LIBRARY_DEBUG
			NAMES
			twined
			HINTS
			"${TWINE_ROOT_DIR}/lib")
		if (NOT TWINE_LIBRARY_DEBUG)
			set(TWINE_LIBRARY_DEBUG "")
		endif()

		set(TWINE_INCLUDE_DIRS "${TWINE_INCLUDE_DIR}")
		set(TWINE_INCLUDEDIR "${TWINE_INCLUDE_DIR}")
		mark_as_advanced(TWINE_LIBRARY)
	endif()

	mark_as_advanced(TWINE_INCLUDE_DIR)
endif()
