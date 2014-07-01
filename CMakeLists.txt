##############################################################################
# Project metadata
cmake_minimum_required(VERSION 2.8)
project(packeteer)

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
# Dependencies
find_package(Threads)

include(FindPkgConfig)
pkg_check_modules(META meta>=0.1)
pkg_check_modules(TWINE twine>=0.1)
pkg_check_modules(CPPUNIT cppunit>=1.12)


##############################################################################
# Headers
include(CheckIncludeFileCXX)
check_include_file_cxx(sys/select.h PACKETEER_HAVE_SYS_SELECT_H)
check_include_file_cxx(sys/time.h PACKETEER_HAVE_SYS_TIME_H)
check_include_file_cxx(sys/epoll.h PACKETEER_HAVE_SYS_EPOLL_H)
check_include_file_cxx(poll.h PACKETEER_HAVE_POLL_H)
check_include_file_cxx(arpa/inet.h PACKETEER_HAVE_ARPA_INET_H)
check_include_file_cxx(netinet/in.h PACKETEER_HAVE_NETINET_IN_H)


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





##############################################################################
# Platform checks
include(TestBigEndian)
test_big_endian(PACKETEER_BIGENDIAN)


##############################################################################
# Write configuration
get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

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
# Compiler flags

# Set compiler specific flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message("Selecting compiler flags for clang++")

  set (CLANGXX_CXX_FLAGS "-Wall -Wextra -Wabi -Wold-style-cast"
      CACHE STRING "Default flags for clang++")
  if (PACKETEER_USE_CXX11)
    set (CLANGXX_CXX_FLAGS_STD "-std=c++0x -Wc++0x-compat")
  else (PACKETEER_USE_CXX11)
    set (CLANGXX_CXX_FLAGS_STD "-std=c++98")
  endif (PACKETEER_USE_CXX11)
  set (CMAKE_CXX_FLAGS "${CLANGXX_CXX_FLAGS} ${CLANGXX_CXX_FLAGS_STD}")
  set (CMAKE_CXX_FLAGS_DEBUG "${CLANGXX_CXX_FLAGS} ${CLANGXX_CXX_FLAGS_STD} -O0 -ggdb -DDEBUG=1")
  set (CMAKE_CXX_FLAGS_RELEASE "${CLANGXX_CXX_FLAGS} ${CLANGXX_CXX_FLAGS_STD} -O3 -ggdb -DNDEBUG=1")
  set (TESTSUITE_LINK_FLAGS "-static")

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message("Selecting compiler flags for GNU c++")

  set (GNUCXX_CXX_FLAGS "-Wall -Wextra -Wabi -Wold-style-cast -Wstrict-null-sentinel"
      CACHE STRING "Default flags for G++")
  if (PACKETEER_USE_CXX11)
    set (GNUCXX_CXX_FLAGS_STD "-std=c++0x -Wc++0x-compat")
  else (PACKETEER_USE_CXX11)
    set (GNUCXX_CXX_FLAGS_STD "-std=c++98")
  endif (PACKETEER_USE_CXX11)
  set (CMAKE_CXX_FLAGS "${GNUCXX_CXX_FLAGS} ${GNUCXX_CXX_FLAGS_STD}")
  set (CMAKE_CXX_FLAGS_DEBUG "${GNUCXX_CXX_FLAGS} ${GNUCXX_CXX_FLAGS_STD} -O0 -ggdb -DDEBUG=1")
  set (CMAKE_CXX_FLAGS_RELEASE "${GNUCXX_CXX_FLAGS} ${GNUCXX_CXX_FLAGS_STD} -O3 -ggdb -DNDEBUG=1")
  set (TESTSUITE_LINK_FLAGS "-static")
endif ()

# Add include paths
include_directories(
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}"
)

# Add library flags
message(STATUS "current ${CMAKE_CXX_FLAGS}")
message(STATUS "cppunit ${CPPUNIT_CFLAGS}")
message(STATUS "meta ${META_CFLAGS}")
message(STATUS "twine ${TWINE_CFLAGS}")
set (CMAKE_CXX_FLAGS          "${TWINE_CFLAGS} ${META_CFLAGS} ${CPPUNIT_CFLAGS} ${CMAKE_CXX_FLAGS}")
message(STATUS "combined ${CMAKE_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG    "${TWINE_CFLAGS} ${META_CFLAGS} ${CPPUNIT_CFLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
set (CMAKE_CXX_FLAGS_RELEASE  "${TWINE_CFLAGS} ${META_CFLAGS} ${CPPUNIT_CFLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")


##############################################################################
# Build artefacts
set(LIB_SOURCES
    packeteer/version.cpp
    packeteer/error.cpp
    packeteer/
    packeteer/packeteer.cpp
    packeteer/scheduler.cpp
    packeteer/connector.cpp
    packeteer/detail/worker.cpp
    packeteer/detail/scheduler_impl.cpp
    packeteer/detail/connector_pipe.cpp
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

add_library(packeteer_static STATIC ${LIB_SOURCES})
add_library(packeteer_shared SHARED ${LIB_SOURCES})
set_target_properties(packeteer_static packeteer_shared PROPERTIES OUTPUT_NAME packeteer)

set_target_properties(packeteer_shared PROPERTIES
    SOVERSION "${PACKETEER_PACKAGE_MAJOR}"
    VERSION "${PACKETEER_PACKAGE_MAJOR}.${PACKETEER_PACKAGE_MINOR}.0"
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/packeteer/packeteer-config.h
    packeteer/packeteer.h
    packeteer/version.h
    packeteer/error.h
    packeteer/macros.h
    packeteer/types.h
    packeteer/concurrent_queue.h
    packeteer/callback.h
    packeteer/events.h
    packeteer/scheduler.h
    packeteer/connector.h
    DESTINATION include/packeteer)

install(FILES
    packeteer/net/socket_address.h
    packeteer/net/network.h
    DESTINATION include/packeteer/net)

install(FILES
    packeteer/net/detail/sysincludes.h
    DESTINATION include/packeteer/net/detail)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/packeteer.pc
    DESTINATION lib/pkgconfig)

install(TARGETS
    packeteer_static ARCHIVE
    DESTINATION lib)
install(TARGETS
    packeteer_shared LIBRARY
    DESTINATION lib)


##############################################################################
# Tests
if (CPPUNIT_FOUND)
  # Tests compatible with all C++ versions
  set(TEST_SOURCES
      test/test_cidr.cpp
      test/test_socket_address.cpp
      test/test_network.cpp
      test/test_callback.cpp
      test/test_concurrent_queue.cpp
      test/test_scheduler_impl.cpp
      test/test_scheduler.cpp
      test/test_connector.cpp
  )

  add_executable(testsuite
      ${TEST_SOURCES}
      test/testsuite.cpp)
  target_link_libraries(testsuite
      packeteer_static
      ${CMAKE_THREAD_LIBS_INIT}
      ${DEP_LIBRARIES}
      ${TWINE_STATIC_LDFLAGS}
      ${CPPUNIT_LDFLAGS})
  set_target_properties(testsuite PROPERTIES LINK_FLAGS "${TESTSUITE_LINK_FLAGS}")

  add_test(NAME "Testsuite" COMMAND testsuite)
endif (CPPUNIT_FOUND)
