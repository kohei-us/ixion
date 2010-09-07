#!/bin/sh

touch ChangeLog

if [ ! -e ltmain.sh ]; then
    libtoolize
fi

aclocal
automake --gnu --add-missing
autoconf
./configure $@
