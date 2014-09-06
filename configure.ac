#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([cam], [0.0.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([cam.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Links the math library
AM_CXXFLAGS=-lm

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h math.h ctype.h err.h errno.h fcntl.h signal.h sys/ioctl.h sys/param.h sys/stat.h unistd.h memory.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD

AC_CHECK_FUNCS([memset pow sqrt strtol strlen ceil isdigit warn signal ioctl stat usleep])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

