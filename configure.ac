#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define([ixion_major_version], [0])
m4_define([ixion_minor_version], [11])
m4_define([ixion_micro_version], [99])

m4_define([ixion_major_api_version], [0])
m4_define([ixion_minor_api_version], [12])

m4_define([ixion_version],
          [ixion_major_version.ixion_minor_version.ixion_micro_version])

m4_define([ixion_api_version],
          [ixion_major_api_version.ixion_minor_api_version])


AC_PREREQ([2.63])
AC_INIT([libixion], [ixion_version])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign dist-bzip2 dist-xz])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AX_CXX_COMPILE_STDCXX_11
CXXFLAGS="$CXXFLAGS -fvisibility=hidden -Wall -pthread"
LDFLAGS="-pthread -Wl,--no-as-needed"

AC_CANONICAL_HOST

case $host_os in
    darwin*)
        _os=Darwin
    ;;
    *)
        _os=
    ;;
esac

AC_ARG_ENABLE([python],
    [AS_HELP_STRING([--disable-python], [Disable python bindings])],
    [enable_python="$enableval"],
    [enable_python=yes]
)

IXION_VERSION=ixion_version
IXION_API_VERSION=ixion_api_version
IXION_MAJOR_VERSION=ixion_major_version
IXION_MINOR_VERSION=ixion_minor_version
IXION_MICRO_VERSION=ixion_micro_version
AC_SUBST(IXION_VERSION)
AC_SUBST(IXION_API_VERSION)
AC_SUBST(IXION_MAJOR_VERSION)
AC_SUBST(IXION_MINOR_VERSION)
AC_SUBST(IXION_MICRO_VERSION)

AC_CONFIG_MACRO_DIR([m4])

m4_pattern_allow([^BOOST_])

for top_builddir in . .. ../.. $ac_auxdir $ac_auxdir/..; do
    test -f $top_builddir/configure && break
done


# Checks for programs.
AC_PROG_CXX
LT_INIT([win32-dll disable-static pic-only])
AC_CANONICAL_HOST

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_STRTOD
AC_CHECK_FUNCS([gettimeofday])

BOOST_REQUIRE([1.36])

# =====
# Debug
# =====
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug], [Build with debug features in mind.])],
	[enable_debug="$enableval"],
	[enable_debug=no]
)
AS_IF([test "x$enable_debug" != "xno"], [
	CXXFLAGS="$CXXFLAGS -g -O0"
], [
	CXXFLAGS="$CXXFLAGS -O2"
	AC_DEFINE([NDEBUG], [], [Disable debugging information])
])

# Check for mdds.
PKG_CHECK_MODULES([MDDS],[mdds-1.2 >= 1.2.0])

# Check for python.
AS_IF([test "x$enable_python" != "xno"], [
    AM_PATH_PYTHON(3)
    PKG_CHECK_MODULES([PYTHON], [python3 >= 3.4])
])
AM_CONDITIONAL([BUILD_PYTHON], [test "x$enable_python" != "xno"])
AM_CONDITIONAL([OSX], [test "x$_os" = "xDarwin"])

BOOST_THREADS
BOOST_PROGRAM_OPTIONS

AC_CONFIG_FILES([Makefile
                 libixion-$IXION_API_VERSION.pc:libixion.pc.in
                 bin/env.sh
                 include/Makefile
                 include/ixion/Makefile
                 include/ixion/interface/Makefile
                 misc/libixion.spec
                 src/Makefile
                 src/libixion/Makefile
                 src/libixion/constants.inl
                 src/python/Makefile])
AC_OUTPUT

AC_MSG_NOTICE([
==============================================================================
Build configuration:
	python:               $enable_python
==============================================================================
])
