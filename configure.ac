#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ultimate-compress], [1.0], [libnoon@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([ulcompress.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([gmp], [__gmpz_init])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h gmp.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset])

# Print all errors.
error=false
if test "$ac_cv_header_gmp_h" = no || test "$ac_cv_lib_gmp_mpz_init" = no; then
  AC_MSG_ERROR([error: the GNU Multiple Precision library (libgmp) is required])
  error=true
fi

# Fail if there was any error.
if $error; then
  exit 1
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
