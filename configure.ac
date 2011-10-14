#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
LT_PREREQ([2.4])

AC_INIT([packetflinger], [0.1], [unwesen@users.sourceforge.net])
AC_CONFIG_SRCDIR([packetflinger])
AC_CONFIG_HEADERS([packetflinger/packetflinger-config.h])

LT_INIT
AC_SUBST([LIBTOOL_DEPS])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11.1])

AC_LANG_CPLUSPLUS

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_LIBTOOL

# Checks for libraries.
AX_BOOST_BASE([1.46.0])
AX_BOOST_THREAD
# AX_BOOST_PROGRAM_OPTIONS
PKG_CHECK_MODULES([CPPUNIT], [cppunit >= 1.12])

# Checks for header files.
AC_HEADER_STDC
#AC_CHECK_HEADERS([sys/select.h sys/time.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([iostream])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday])

# Checks for compiler characteristics
AC_C_BIGENDIAN

# Checks for machine characteristics
AX_COUNT_CPUS
AX_CACHE_SIZE

# Optional features
AC_ARG_ENABLE([debug], AC_HELP_STRING([--enable-debug],
      [Set the DEBUG define, and disable compiler optimizations. Passing
       --disable-debug will define NDEBUG and enable compiler optimizations.
       The default is to enable debugging.]),
    [
      case "${enableval}" in
        yes|true|1)
          debug=true
          ;;
        no|false|0)
          debug=false
          ;;
        *)
          AC_MSG_ERROR([bad value ${enableval} for --enable-debug])
          ;;
      esac
    ], [
      debug=true
    ])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])

# Variable substitions
AX_SPLIT_VERSION
AC_DEFINE_UNQUOTED([PACKAGE_MAJOR], ["$AX_MAJOR_VERSION"], [Define to the major
    version of this package.])
AC_DEFINE_UNQUOTED([PACKAGE_MINOR], ["$AX_MINOR_VERSION"], [Define to the minor
    version of this package.])

AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

# Output
AC_CONFIG_SUBDIRS([3rdparty/openpa])

AC_CONFIG_FILES([
  Makefile
  packetflinger/Makefile
  test/Makefile
  packetflinger.pc
])
AC_OUTPUT
