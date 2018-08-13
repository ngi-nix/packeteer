##############################################################################
# Project metadata
cmake_minimum_required(VERSION 2.8)
project(packeteer)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

set (PACKETEER_PACKAGE_MAJOR 0)
set (PACKETEER_PACKAGE_MINOR 1)
set (PACKETEER_PACKAGE_URL https://github.com/jfinkhaeuser/packeteer)

##############################################################################
# Options
option(PACKETEER_USE_CXX11
    "Forces meta to use C++11 features." ON)

if (PACKETEER_USE_CXX11)
  set (META_CXX_MODE META_CXX_MODE_CXX0X)
else (PACKETEER_USE_CXX11)
  set (META_CXX_MODE META_CXX_MODE_CXX98)
endif (PACKETEER_USE_CXX11)


##############################################################################
# Compiler flags #1
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message("Selecting compiler flags for clang++")

  set (CLANGXX_CXX_FLAGS "-Wall -Wextra -Wabi -Wold-style-cast"
      CACHE STRING "Default flags for clang++")
  if (PACKETEER_USE_CXX11)
    set (CLANGXX_CXX_FLAGS_STD "-std=c++0x -Wc++0x-compat")
  else (PACKETEER_USE_CXX11)
    set (CLANGXX_CXX_FLAGS_STD "-std=c++98")
  endif (PACKETEER_USE_CXX11)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANGXX_CXX_FLAGS} ${CLANGXX_CXX_FLAGS_STD}")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -ggdb -DDEBUG")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -ggdb -DNDEBUG")

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message("Selecting compiler flags for GNU c++")

  set (GNUCXX_CXX_FLAGS "-Wall -Wextra -Wabi -Wold-style-cast -Wstrict-null-sentinel"
      CACHE STRING "Default flags for G++")
  if (PACKETEER_USE_CXX11)
    set (GNUCXX_CXX_FLAGS_STD "-std=c++0x -Wc++0x-compat")
  else (PACKETEER_USE_CXX11)
    set (GNUCXX_CXX_FLAGS_STD "-std=c++98")
  endif (PACKETEER_USE_CXX11)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GNUCXX_CXX_FLAGS} ${GNUCXX_CXX_FLAGS_STD}")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -ggdb -DDEBUG")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -ggdb -DNDEBUG")

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  message("Selecting compiler flags for MSVC")

  set (MSVC_CXX_FLAGS "/analyze /GR /sdl /W4"
      CACHE STRING "Default flags for MSVC")
  if (PACKETEER_USE_CXX11)
    set (MSVC_CXX_FLAGS_STD "/std:c++latest")
  else (PACKETEER_USE_CXX11)
    set (MSVC_CXX_FLAGS_STD "/std:c++98")
  endif (PACKETEER_USE_CXX11)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_CXX_FLAGS} ${MSVC_CXX_FLAGS_STD}")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS}")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /GL /Ox")
endif ()

##############################################################################
# Dependencies
find_package(Threads)
find_package(meta 1.1)
find_package(twine 1.0)
find_package(cppunit 1.12.1)


##############################################################################
# Headers
include(CheckIncludeFileCXX)
check_include_file_cxx(sys/select.h PACKETEER_HAVE_SYS_SELECT_H)
check_include_file_cxx(sys/time.h PACKETEER_HAVE_SYS_TIME_H)
check_include_file_cxx(sys/epoll.h PACKETEER_HAVE_SYS_EPOLL_H)
check_include_file_cxx(poll.h PACKETEER_HAVE_POLL_H)
check_include_file_cxx(arpa/inet.h PACKETEER_HAVE_ARPA_INET_H)
check_include_file_cxx(netinet/in.h PACKETEER_HAVE_NETINET_IN_H)
check_include_file_cxx(linux/un.h PACKETEER_HAVE_LINUX_UN_H)


##############################################################################
# Checks for typedefs, structures, and compiler characteristics.
include(CheckTypeSize)
check_type_size(int32_t INT32_T)
check_type_size(uint32_t UINT32_T)
check_type_size(int64_t INT64_T)
check_type_size(uint64_t UINT64_T)
check_type_size(size_t SIZE_T)
check_type_size(ssize_t SSIZE_T)


##############################################################################
# Symbols
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <sys/epoll.h>

int main(int, char**)
{
  int foo = epoll_create1(EPOLL_CLOEXEC);
}
" PACKETEER_HAVE_EPOLL_CREATE1)

check_cxx_source_compiles("
#include <sys/select.h>
#include <string.h>

int main(int, char**)
{
  int ret = select(0, NULL, NULL, NULL, NULL);
}
" PACKETEER_HAVE_SELECT)

check_cxx_source_compiles("
#include <sys/select.h>
#include <string.h>

int main(int, char**)
{
  int ret = pselect(0, NULL, NULL, NULL, NULL, NULL);
}
" PACKETEER_HAVE_PSELECT)

check_cxx_source_compiles("
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = poll(NULL, 0, 0);
}
" PACKETEER_HAVE_POLL)

check_cxx_source_compiles("
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = ppoll(NULL, 0, 0, NULL);
}
" PACKETEER_HAVE_PPOLL)

check_cxx_source_compiles("
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = POLLRDHUP;
}
" PACKETEER_HAVE_POLLRDHUP)

check_cxx_source_compiles("
#include <poll.h>
#include <string.h>

int main(int, char**)
{
  int foo = POLLHUP;
}
" PACKETEER_HAVE_POLLHUP)

check_cxx_source_compiles("
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int main(int, char **)
{
  int foo = kqueue();
}" PACKETEER_HAVE_KQUEUE)

check_cxx_source_compiles("
#include <unordered_map>

int main(int, char**)
{
  std::unordered_map<int, int> map;
}
" PACKETEER_HAVE_UNORDERED_MAP)

check_cxx_source_compiles("
#include <tr1/unordered_map>

int main(int, char**)
{
  std::tr1::unordered_map<int, int> map;
}
" PACKETEER_HAVE_TR1_UNORDERED_MAP)

check_cxx_source_compiles("
#include <ext/hash_map>

int main(int, char**)
{
  std::hash_map<int, int> map;
}
" PACKETEER_HAVE_EXT_HASHMAP)

check_cxx_source_compiles("
#include <ext/hash_map>

int main(int, char**)
{
  __gnu_cxx::hash_map<int, int> map;
}
" PACKETEER_HAVE_GNUCXX_HASHMAP)

check_cxx_source_compiles("
#include <unordered_set>

int main(int, char**)
{
  std::unordered_set<int> set;
}
" PACKETEER_HAVE_UNORDERED_SET)

check_cxx_source_compiles("
#include <tr1/unordered_set>

int main(int, char**)
{
  std::tr1::unordered_set<int> set;
}
" PACKETEER_HAVE_TR1_UNORDERED_SET)

check_cxx_source_compiles("
#include <ext/hash_set>

int main(int, char**)
{
  __gnu_cxx::hash_set<int> set;
}
" PACKETEER_HAVE_GNUCXX_HASHSET)

check_cxx_source_compiles("
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main(int, char**)
{
  mknod(\"foo\", S_IFIFO, S_IRUSR);
}
" PACKETEER_HAVE_MKNOD)

check_cxx_source_compiles("
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main(int, char**)
{
  mkfifo(\"foo\", S_IRUSR);
}
" PACKETEER_HAVE_MKFIFO)


##############################################################################
# Platform checks
include(TestBigEndian)
test_big_endian(PACKETEER_BIGENDIAN)


##############################################################################
# Write configuration
#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

configure_file(
    "${PROJECT_SOURCE_DIR}/packeteer/packeteer-config.h.in"
    "${PROJECT_BINARY_DIR}/packeteer/packeteer-config.h"
)

configure_file(
    "${PROJECT_SOURCE_DIR}/packeteer.pc.in"
    "${PROJECT_BINARY_DIR}/packeteer.pc"
    @ONLY
)


##############################################################################
# Compiler flags #2

# Add include paths
include_directories(
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}"
    "${META_INCLUDEDIR}"
    "${TWINE_INCLUDEDIR}"
)

# Add library flags
set (CMAKE_CXX_FLAGS          "${TWINE_CFLAGS} ${META_CFLAGS} ${CPPUNIT_CFLAGS} ${CMAKE_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG    "${TWINE_CFLAGS} ${META_CFLAGS} ${CPPUNIT_CFLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
set (CMAKE_CXX_FLAGS_RELEASE  "${TWINE_CFLAGS} ${META_CFLAGS} ${CPPUNIT_CFLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")


##############################################################################
# Build artefacts
set(LIB_SOURCES
    packeteer/version.cpp
    packeteer/error.cpp
    packeteer/packeteer.cpp
    packeteer/scheduler.cpp
    packeteer/connector.cpp
    packeteer/peer_address.cpp
    packeteer/detail/worker.cpp
    packeteer/detail/filedescriptors.cpp
    packeteer/detail/scheduler_impl.cpp
    packeteer/detail/connector.cpp
    packeteer/net/socket_address.cpp
    packeteer/net/network.cpp
    packeteer/net/detail/cidr.cpp
)

if (PACKETEER_HAVE_EPOLL_CREATE1)
  set(LIB_SOURCES ${LIB_SOURCES}
      packeteer/detail/io_epoll.cpp
  )
endif (PACKETEER_HAVE_EPOLL_CREATE1)

if (PACKETEER_HAVE_SELECT)
  set(LIB_SOURCES ${LIB_SOURCES}
      packeteer/detail/io_select.cpp
  )
endif (PACKETEER_HAVE_SELECT)

if (PACKETEER_HAVE_POLL)
  set(LIB_SOURCES ${LIB_SOURCES}
      packeteer/detail/io_poll.cpp
  )
endif (PACKETEER_HAVE_POLL)

if (PACKETEER_HAVE_KQUEUE)
  set(LIB_SOURCES ${LIB_SOURCES}
      packeteer/detail/io_kqueue.cpp
  )
endif (PACKETEER_HAVE_KQUEUE)

if (UNIX)
  set(LIB_SOURCES ${LIB_SOURCES}
      packeteer/detail/posix/connector_socket.cpp
      packeteer/detail/posix/connector_pipe.cpp
      packeteer/detail/posix/connector_tcp.cpp
      packeteer/detail/posix/connector_udp.cpp
      packeteer/detail/posix/connector_anon.cpp
      packeteer/detail/posix/connector_local.cpp
  )
endif (UNIX)

if (WIN32)
  set(LIB_SOURCES ${LIB_SOURCES}
      packeteer/detail/win32/connector_pipe.cpp
      packeteer/detail/win32/connector_tcp.cpp
  )
endif (WIN32)


add_library(packeteer_static STATIC ${LIB_SOURCES})
add_library(packeteer_shared SHARED ${LIB_SOURCES})
set_target_properties(packeteer_static packeteer_shared PROPERTIES
    OUTPUT_NAME packeteer)
set_target_properties(packeteer_shared PROPERTIES
    SOVERSION "${PACKETEER_PACKAGE_MAJOR}"
    VERSION "${PACKETEER_PACKAGE_MAJOR}.${PACKETEER_PACKAGE_MINOR}.0"
)


include(GNUInstallDirs)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/packeteer/packeteer-config.h
    packeteer/packeteer.h
    packeteer/version.h
    packeteer/error.h
    packeteer/macros.h
    packeteer/types.h
    packeteer/handle.h
    packeteer/concurrent_queue.h
    packeteer/callback.h
    packeteer/events.h
    packeteer/scheduler.h
    packeteer/connector.h
    DESTINATION include/packeteer)

install(FILES
    packeteer/detail/operators.h
    DESTINATION include/packeteer/detail)

install(FILES
    packeteer/net/socket_address.h
    packeteer/net/network.h
    DESTINATION include/packeteer/net)

install(FILES
    packeteer/net/detail/sysincludes.h
    DESTINATION include/packeteer/net/detail)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/packeteer.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(TARGETS
    packeteer_static packeteer_shared
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT library)

##############################################################################
# Tests
if (CPPUNIT_FOUND)
  # Tests compatible with all C++ versions
  set(TEST_SOURCES
      test/test_error.cpp
      test/test_cidr.cpp
      test/test_handle.cpp
      test/test_socket_address.cpp
      test/test_network.cpp
      test/test_callback.cpp
      test/test_concurrent_queue.cpp
      test/test_scheduler_impl.cpp
      test/test_scheduler.cpp
      test/test_peer_address.cpp
      test/test_connector.cpp
  )

  add_executable(testsuite
      ${TEST_SOURCES}
      test/testsuite.cpp)

  target_link_libraries(testsuite
      packeteer_static
      ${CMAKE_THREAD_LIBS_INIT}
      ${DEP_LIBRARIES}
      debug
      ${TWINE_LIBRARY_DEBUG}
      ${CPPUNIT_LIBRARY_DEBUG}
      optimized
      ${TWINE_LIBRARY_RELEASE}
      ${CPPUNIT_LIBRARY_RELEASE})

  # Since some time after 2.8.9
  if ("${CMAKE_VERSION}" VERSION_GREATER "2.8.10")
    target_include_directories(testsuite PRIVATE ${CPPUNIT_INCLUDE_DIR})
  else()
    include_directories(testsuite PRIVATE ${CPPUNIT_INCLUDE_DIR})
  endif()

  set_target_properties(testsuite PROPERTIES LINK_FLAGS "${TESTSUITE_LINK_FLAGS}")

  add_test(NAME "Testsuite" COMMAND testsuite)
  set(PROJECT_TEST_NAME testsuite)

  # Code coverage
  if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_BUILD_TYPE STREQUAL "Coverage")
    include(CodeCoverage)
    setup_target_for_coverage(coverage ${PROJECT_TEST_NAME} coverage)
    target_link_libraries(${PROJECT_TEST_NAME} gcov)
  endif()

  # May not work for older CMake versions
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT testsuite)
endif (CPPUNIT_FOUND)

##############################################################################
# CPack Section
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Packeteer provides a simplified, asynchronous, event-based socket API.")
set(CPACK_PACKAGE_VENDOR "finkhaeuser consulting")
set(CPACK_PACKAGE_CONTACT "jens@finkhaeuser.de")
set(CPACK_PACKAGE_VERSION_MAJOR "${PACKETEER_PACKAGE_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PACKETEER_PACKAGE_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "0")

set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
# set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

if(WIN32 AND NOT UNIX)
  set(CPACK_GENERATOR "NSIS;NSIS64")
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}\\\\data\\\\tool-icon.bmp")
  # set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
  set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} packeteer")
  set(CPACK_NSIS_HELP_LINK "http://jfinkhaeuser.github.io/packeteer/")
  set(CPACK_NSIS_URL_INFO_ABOUT "http://jfinkhaeuser.github.io/packeteer/")
  set(CPACK_NSIS_CONTACT "jens@finkhaeuser.de")
  set(CPACK_NSIS_MODIFY_PATH ON)
else(WIN32 AND NOT UNIX)
  set(CPACK_GENERATOR "STGZ;TGZ;TBZ2;DEB")
  # set(CPACK_STRIP_FILES "bin/MyExecutable")
  set(CPACK_SOURCE_STRIP_FILES "")

  set(CPACK_SOURCE_IGNORE_FILES
      "/deps/"
      "/CMakeFiles/"
      "/tmp/"
      "/build/"
      "/_package/"
      "\\\\.git*"
      "\\\\..*\\\\.sw."
      "/\\\\.travis.yml"
  )
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "packeteer-${PACKETEER_PACKAGE_MAJOR}.${PACKETEER_PACKAGE_MINOR}")
endif(WIN32 AND NOT UNIX)

# set(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable")

include(CPack)
