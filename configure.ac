#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define([ixion_major_version], [0])
m4_define([ixion_minor_version], [14])
m4_define([ixion_micro_version], [99])

m4_define([ixion_major_api_version], [0])
m4_define([ixion_minor_api_version], [15])

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
CXXFLAGS="$CXXFLAGS -fvisibility=hidden -Wall -Wshadow -pthread -DBOOST_LOG_DYN_LINK"

AC_CANONICAL_HOST

case $host_os in
    darwin*)
        _os=Darwin
        LDFLAGS="$LDFLAGS -L/usr/local/lib"
    ;;
    *)
        _os=
        LDFLAGS="$LDFLAGS -Wl,--no-as-needed -pthread -ldl"
    ;;
esac

AC_ARG_ENABLE([python],
    [AS_HELP_STRING([--disable-python], [Disable python bindings])],
    [enable_python="$enableval"],
    [enable_python=yes]
)

AC_ARG_ENABLE([threads],
    [AS_HELP_STRING([--disable-threads], [Disable threaded calculations])],
    [enable_threads="$enableval"],
    [enable_threads=yes]
)

AC_ARG_ENABLE([cuda],
    [AS_HELP_STRING([--enable-cuda], [Enable CUDA compute engine])],
    [enable_cuda="$enableval"],
    [enable_cuda=no]
)

IXION_VERSION=ixion_version
IXION_API_VERSION=ixion_api_version
IXION_MAJOR_VERSION=ixion_major_version
IXION_MINOR_VERSION=ixion_minor_version
IXION_MICRO_VERSION=ixion_micro_version
IXION_MAJOR_API_VERSION=ixion_major_api_version
IXION_MINOR_API_VERSION=ixion_minor_api_version
AC_SUBST(IXION_VERSION)
AC_SUBST(IXION_API_VERSION)
AC_SUBST(IXION_MAJOR_VERSION)
AC_SUBST(IXION_MINOR_VERSION)
AC_SUBST(IXION_MICRO_VERSION)
AC_SUBST(IXION_MAJOR_API_VERSION)
AC_SUBST(IXION_MINOR_API_VERSION)

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

BOOST_REQUIRE([1.36])
BOOST_FILESYSTEM

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

PKG_CHECK_MODULES([MDDS],[mdds-1.4 >= 1.4.0])
PKG_CHECK_MODULES([SPDLOG], [spdlog >= 0.16.0])

AC_ARG_ENABLE(log-debug,
	AS_HELP_STRING([--enable-log-debug], [Enable debug outputs.])],
	[enable_log_debug="$enableval"],
	[enable_log_debug=no    ]
)

AS_IF([test "x$enable_log_debug" != "xno"], [
        CXXFLAGS="$CXXFLAGS -DSPDLOG_DEBUG_ON"
],)

AC_ARG_ENABLE(log-trace,
	AS_HELP_STRING([--enable-log-trace], [Enable trace outputs.])],
	[enable_log_trace="$enableval"],
	[enable_log_trace=no]
)

AS_IF([test "x$enable_log_trace" != "xno"], [
        CXXFLAGS="$CXXFLAGS -DSPDLOG_TRACE_ON"
],)

# Check for python.
AS_IF([test "x$enable_python" != "xno"], [
    AM_PATH_PYTHON(3)
    PKG_CHECK_MODULES([PYTHON], [python3 >= 3.4])
])

AS_IF([test "x$enable_threads" != "xno"], [
        CXXFLAGS="$CXXFLAGS -DIXION_THREADS=1"
], [
        CXXFLAGS="$CXXFLAGS -DIXION_THREADS=0"
])

AM_CONDITIONAL([BUILD_PYTHON], [test "x$enable_python" != "xno"])
AM_CONDITIONAL([IXION_THREADS], [test "x$enable_threads" != "xno"])
AM_CONDITIONAL([BUILD_CUDA], [test "x$enable_cuda" != "xno"])
AM_CONDITIONAL([OSX], [test "x$_os" = "xDarwin"])

BOOST_PROGRAM_OPTIONS

AC_CONFIG_FILES([Makefile
                 libixion-$IXION_API_VERSION.pc:libixion.pc.in
                 bin/env.sh
                 include/Makefile
                 include/ixion/Makefile
                 include/ixion/interface/Makefile
                 misc/libixion.spec
                 src/Makefile
                 src/include/Makefile
                 src/libixion/Makefile
                 src/libixion/constants.inl
                 src/python/Makefile])
AC_OUTPUT

AC_MSG_NOTICE([
==============================================================================
Build configuration:
        host os:              $host_os
        python:               $enable_python
        threads:              $enable_threads
        cuda:                 $enable_cuda
        log (debug)           $enable_log_debug
        log (trace)           $enable_log_trace
==============================================================================
])
