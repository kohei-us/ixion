#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define([ixion_major_version], [0])
m4_define([ixion_minor_version], [2])
m4_define([ixion_micro_version], [0])
m4_define([ixion_version],
                    [ixion_major_version.ixion_minor_version.ixion_micro_version])
m4_define([ixion_minor_api_version], [m4_eval(ixion_minor_version + ixion_minor_version%2)])
m4_define([ixion_api_version],
                    [ixion_major_version.ixion_minor_api_version])


AC_PREREQ([2.63])
AC_INIT([ixion], [ixion_version])
AM_INIT_AUTOMAKE

IXION_API_VERSION=ixion_api_version
AC_SUBST(IXION_API_VERSION)

BOOST_VERSION=1.34

AC_CONFIG_MACRO_DIR([m4])

m4_pattern_allow([^BOOST_])

for top_builddir in . .. ../.. $ac_auxdir $ac_auxdir/..; do
    test -f $top_builddir/configure && break
done


# Checks for programs.
AC_PROG_CXX
AC_PROG_LIBTOOL

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

BOOST_REQUIRE([$BOOST_VERSION])
BOOST_THREADS
BOOST_PROGRAM_OPTIONS

CXXFLAGS="-D_REENTRANT -std=c++0x"
AC_DEFINE(CXXFLAGS)

AC_CONFIG_FILES([Makefile
                 ixion.pc
                 inc/Makefile
                 src/Makefile
                 src/libixion/Makefile])
AC_OUTPUT
