#!/usr/bin/env bash

# I have to write this wrapper for OSX because ixion.so module is hardcoded to
# find libixion-<version>.dylib to the installed location, and the only way to
# have it use libixion-<version>.dylib in src/libixion/.libs is to physically
# re-write the path in ixion.so.

PROGDIR=$(dirname $0)
source $PROGDIR/env.sh

TESTPYTHONPATH=$PROGDIR/../src/python/.test
PYTESTFILEDIR=$PROGDIR/../test/python

# Copy ixion.so into the special test directory.
mkdir -p $TESTPYTHONPATH
cp $PROGDIR/../src/python/.libs/ixion.so $TESTPYTHONPATH/

echo "library installation directory: $IXION_INSTLIBDIR"
echo "library base name: $IXION_LIBNAME"

# Re-write the path to libixion.dylib in ixion.so.
install_name_tool -change \
    $IXION_INSTLIBDIR/$IXION_LIBNAME.dylib \
    $PROGDIR/../src/libixion/.libs/$IXION_LIBNAME.dylib \
    $TESTPYTHONPATH/ixion.so

# Use that ixion.so module to run the tests.
export PYTHONPATH=$TESTPYTHONPATH

TESTS=$(ls $PYTESTFILEDIR/*.py)

for _file in $TESTS; do
    echo running $_file...
    $_file
done

