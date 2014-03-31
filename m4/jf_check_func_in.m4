# SYNOPSIS
#
#   JF_CHECK_FUNC_IN(HEADER, NAME, FUNCTION-BODY [,ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   Checking for library functions in a given header file, providing a function
#   body.
#
# LICENSE
#
#   Author(s): Jens Finkhaeuser <jens@unwesen.co.uk>
#
#   Copyright (c) 2014 Unwesen Ltd.
#
#   This software is licensed under the terms of the GNU GPLv3 for personal,
#   educational and non-profit use. For all other uses, alternative license
#   options are available. Please contact the copyright holder for additional
#   information, stating your intended usage.
#
#   You can find the full text of the GPLv3 in the COPYING file in this code
#   distribution.
#
#   This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
#   without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
#   PARTICULAR PURPOSE.

#serial 1

dnl JF_CHECK_FUNC_IN(HEADER, NAME, FUNCTION-BODY, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AU_ALIAS([AC_CHECK_FUNC_IN], [JF_CHECK_FUNC_IN])
AC_DEFUN([JF_CHECK_FUNC_IN],
[AC_MSG_CHECKING([for $2 in $1])
AC_CACHE_VAL(ac_cv_func_$2,
jf_check_func_in_headers=
for jf_check_func_in_header in $1 ; do
  jf_check_func_in_headers="$jf_check_func_in_headers
#include <$jf_check_func_in_header>"
done
[AC_TRY_LINK(
dnl Don't include <ctype.h> because on OSF/1 3.0 it includes <sys/types.h>
dnl which includes <sys/select.h> which contains a prototype for
dnl select.  Similarly for bzero.
[
#include <assert.h>
$jf_check_func_in_headers
], [
$3
], eval "ac_cv_func_$2=yes", eval "ac_cv_func_$2=no")])
unset jf_check_func_in_header
unset jf_check_func_in_headers

if eval "test \"`echo '$ac_cv_func_'$2`\" = yes"; then
  AC_DEFINE_UNQUOTED(translit([HAVE_$2], [a-z], [A-Z]), [1], [Found the $2 function in $1.])
  AC_MSG_RESULT(yes)
  ifelse([$4], , :, [$4])
else
  AC_MSG_RESULT(no)
ifelse([$5], , , [$5
])dnl
fi
])
