# - try to find meta library
#
# Cache Variables: (probably not for direct use in your scripts)
#  META_INCLUDEDIR
#  META_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  META_FOUND
#  META_INCLUDE_DIRS
#  META_LIBRARIES
#
# Requires these CMake modules:
#  SelectLibraryConfigurations (included with CMake >= 2.8.0)
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Based off Findcppunit.cmake

# First, try pkg-config
include(FindPkgConfig)
pkg_check_modules(META "meta>=${meta_FIND_VERSION}")

# Search in this directory if pkg-config found nothing
set(META_ROOT_DIR
	"${META_ROOT_DIR}"
	CACHE
	PATH
	"Directory to search for meta")

if (NOT META_FOUND)
	find_path(META_INCLUDE_DIR
		NAMES
		meta/meta.h
		PATHS
		"${META_ROOT_DIR}"
		PATH_SUFFIXES
		include/)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(meta
		DEFAULT_MSG
		META_INCLUDE_DIR)

	if(META_FOUND)
	    set(META_LIBRARY "")
		set(META_LIBRARIES ${META_LIBRARY} ${CMAKE_DL_LIBS})
		set(META_INCLUDE_DIRS "${META_INCLUDE_DIR}")
		set(META_INCLUDEDIR "${META_INCLUDE_DIR}")
		mark_as_advanced(META_LIBRARY)
	endif()

	mark_as_advanced(META_INCLUDE_DIR)
endif()
