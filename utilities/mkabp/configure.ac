AC_INIT([mkabp], [1.1], [adithiya.venkatakrishnan@atomtables.dev])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_FILES([Makefile])
AC_PROG_CC

# Check for NASM assembler
AC_CHECK_PROG([NASM], [nasm], [nasm])
AS_IF([test "x$NASM" = "x"], [AC_MSG_ERROR([nasm is required but not found])])

# Check for xxd utility
AC_CHECK_PROG([XXD], [xxd], [xxd])
AS_IF([test "x$XXD" = "x"], [AC_MSG_ERROR([xxd is required but not found])])

AC_OUTPUT