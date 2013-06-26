#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([libtruepeak], [0.1], [audionuma@gmail.com])
AM_INIT_AUTOMAKE([foreign])
LT_INIT
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
#AM_PROG_CC_C_O

AC_ARG_ENABLE([findtruepeak],
    AS_HELP_STRING([--disable-findtruepeak], [Disable the findtruepeak binary]))


AS_IF([test "x$enable_findtruepeak" != "xno"],
    [PKG_CHECK_MODULES([DEPS], [sndfile >= 1.0],
        [AM_CONDITIONAL(BUILD_FINDTRUEPEAK, true)],
        [AM_CONDITIONAL(BUILD_FINDTRUEPEAK, false)]
        )],
    [AM_CONDITIONAL(BUILD_FINDTRUEPEAK, false)]
)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([
    Makefile
    ])
AC_OUTPUT
