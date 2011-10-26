#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define([ixion_major_version], [0])
m4_define([ixion_minor_version], [3])
m4_define([ixion_micro_version], [0])
m4_define([ixion_version],
                    [ixion_major_version.ixion_minor_version.ixion_micro_version])
m4_define([ixion_minor_api_version], [m4_eval(ixion_minor_version + ixion_minor_version%2)])
m4_define([ixion_api_version],
                    [ixion_major_version.ixion_minor_api_version])


AC_PREREQ([2.63])
AC_INIT([ixion], [ixion_version])
AM_INIT_AUTOMAKE

AC_ARG_WITH(hash-container-compat,
[
  --with-hash-container-compat use std::hash_foo instead of boost::unordered_foo
],
,with_hash_container_compat=no)

AC_ARG_WITH(mdds-include-path,
    AS_HELP_STRING([--with-mdds-include-path], [specify include path for mdds headers.])
,)

IXION_API_VERSION=ixion_api_version
AC_SUBST(IXION_API_VERSION)

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

CPPFLAGS="-D_REENTRANT -DMDDS_HASH_CONTAINER_BOOST"

AC_MSG_CHECKING([whether to turn on hash container compatibility])
if test "z$with_hash_container_compat" = "zno"; then
    AC_MSG_RESULT([no])
    BOOST_REQUIRE([1.36])
    BOOST_UNORDERED
else
    AC_MSG_RESULT([yes])
    DEFINES="$DEFINES -D_IXION_HASH_CONTAINER_COMPAT"
fi

if test "z$with_mdds_include_path" != "z"; then
    # Custom mdds include path is given.  Use it.
    AC_MSG_CHECKING([mdds include path])
    AC_MSG_RESULT([$with_mdds_include_path])
    CPPFLAGS="$CPPFLAGS -I$with_mdds_include_path"
fi

AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([mdds/rectangle_set.hpp mdds/mixed_type_matrix.hpp], [], [AC_MSG_ERROR([not all required mdds headers found.])])
AC_LANG_POP([C++])

CPPFLAGS="$CPPFLAGS -g -Os"

BOOST_THREADS
BOOST_PROGRAM_OPTIONS

AC_CONFIG_FILES([Makefile
                 libixion.pc
                 VERSION
                 include/Makefile
                 include/ixion/Makefile
                 include/ixion/hash_container/Makefile
                 include/ixion/interface/Makefile
                 misc/libixion.spec
                 src/Makefile
                 src/libixion/Makefile])
AC_OUTPUT
