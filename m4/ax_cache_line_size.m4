dnl See if sysconf() returns a cache line size; if not, default to 128 bytes
dnl (modern CPUs tend to have 64 or 128 byte cache lines).
AC_DEFUN([AX_CACHE_LINE_SIZE], [
  AC_CACHE_CHECK([if sysconf() returns the CPU cache line size],
    ax_cv_cache_line_size, [

    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_RUN_IFELSE([
      AC_LANG_PROGRAM([[
          #include <unistd.h>
          #include <iostream>
          #include <fstream>
        ]], [[
          std::ofstream os("conftest.out");
          os << ::sysconf(_SC_LEVEL1_DCACHE_LINESIZE) << std::endl;
          os.close();
        ]]
      )],
      ax_cv_cache_line_size=`cat conftest.out`,
      ax_cv_cache_line_size=128
    )
    AC_LANG_RESTORE
  ])

  AC_DEFINE_UNQUOTED([CACHE_LINE_SIZE], [$ax_cv_cache_line_size],
      [Line size of the processor's data cache; 128 by default.])
])
