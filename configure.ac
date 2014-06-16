##############################################################################
# Setup
AC_PREREQ([2.68])
LT_PREREQ([2.4])

AC_INIT([packeteer], [0.1], [jens@unwesen.co.uk])
AC_CONFIG_SRCDIR([packeteer])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

AC_CONFIG_HEADER([packeteer/_config.h])
AX_PREFIX_CONFIG_H([packeteer/packeteer-config.h])

LT_INIT
AC_SUBST([LIBTOOL_DEPS])
AC_CONFIG_MACRO_DIR([m4])

AC_LANG_CPLUSPLUS

##############################################################################
# Make build target available to automake
target=
case $target_os in
  *linux*|*bsd*)
    target=posix
    ;;
  *win32*)
    target=win32
    ;;
esac
AM_CONDITIONAL([HAVE_POSIX], [test "x$target" == xposix])
AM_CONDITIONAL([HAVE_WIN32], [test "x$target" == xwin32])

##############################################################################
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

##############################################################################
# Checks for compiler characteristics
AC_C_BIGENDIAN

# The AU_ALIAS is needed due to a bug in AX_CXX_HEADER_STDCXX_0X, etc.
AX_CXX_COMPILE_STDCXX_0X
AU_ALIAS([AC_COMPILE_STDCXX_0X], [AX_CXX_COMPILE_STDCXX_0X])

##############################################################################
# Checks for machine characteristics
AX_CACHE_LINE_SIZE
# AX_COUNT_CPUS
# AX_CACHE_SIZE

##############################################################################
# Checks for libraries.
PKG_CHECK_MODULES([CPPUNIT], [cppunit >= 1.12])
PKG_CHECK_MODULES([META], [meta >= 0.1])
PKG_CHECK_MODULES([TWINE], [twine >= 0.1])

##############################################################################
# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/select.h sys/time.h, sys/epoll.h poll.h])

AX_CXX_HEADER_STDCXX_98
AX_CXX_HEADER_STDCXX_TR1
AX_CXX_HEADER_STDCXX_0X

AX_CXX_HEADER_UNORDERED_MAP
AX_CXX_HEADER_TR1_UNORDERED_MAP
AX_CXX_HAVE_EXT_HASH_MAP
AX_CXX_GNUCXX_HASHMAP

AX_CXX_HEADER_UNORDERED_SET
AX_CXX_HEADER_TR1_UNORDERED_SET
AX_CXX_HAVE_EXT_HASH_SET

##############################################################################
# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

##############################################################################
# Checks for library functions.
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
JF_CHECK_FUNC_IN([sys/epoll.h], [epoll_create1],
    [int foo = epoll_create1(EPOLL_CLOEXEC);])
JF_CHECK_FUNC_IN([sys/select.h cstring], [select],
    [int ret = select(0, NULL, NULL, NULL, NULL);])
JF_CHECK_FUNC_IN([sys/select.h cstring], [pselect],
    [int ret = pselect(0, NULL, NULL, NULL, NULL, NULL);])
JF_CHECK_FUNC_IN([poll.h cstring], [poll],
    [int ret = poll(NULL, 0, 0);])
JF_CHECK_FUNC_IN([poll.h cstring], [ppoll],
    [int ret = ppoll(NULL, 0, 0, NULL);])

##############################################################################
# Optional programs

##############################################################################
# Variable substitions
JF_SPLIT_VERSION

AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

##############################################################################
# Output
AC_CONFIG_FILES([
  Makefile
  packeteer/Makefile
  test/Makefile
  packeteer.pc
])
AC_OUTPUT
