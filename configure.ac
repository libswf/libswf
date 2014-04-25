AC_INIT(libswf, 0.0.0)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
# Disable C++/Fortran checks
define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])
AC_PROG_LIBTOOL
AC_CONFIG_SRCDIR([libswf/swf.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for standard library functions.
AC_CHECK_FUNCS([strdup malloc])

# Checks for libraries.
AX_CHECK_ZLIB()

# Check for libraries via pkg-config
AC_ARG_ENABLE([test], AS_HELP_STRING([--enable-test],
    [enable test program @<:@default=no@:>@]))

AM_CONDITIONAL([ENABLE_TEST], [test x$enable_test = xyes])

# add libraries/packages to pkg-config for static linking

AC_SUBST([PKG_LIBS_DEFAULT], [$(test x$enable_shared = xno && echo ${pkg_libs})])
AC_SUBST([PKG_REQUIRES_DEFAULT], [$(test x$enable_shared = xno && echo ${pkg_requires})])
AC_SUBST([PKG_LIBS_PRIVATE], [$(test x$enable_shared = xno || echo ${pkg_libs})])
AC_SUBST([PKG_REQUIRES_PRIVATE], [$(test x$enable_shared = xno || echo ${pkg_requires})])

# Setup output beautifier.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_FILES([Makefile libswf/Makefile test/Makefile libswf.pc])
AC_OUTPUT
