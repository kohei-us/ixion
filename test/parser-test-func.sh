#!/usr/bin/env bash

exec_test()
{
    PROGDIR=`dirname $0`
    SRCDIR=$PROGDIR/../src

    export PATH=$SRCDIR:$SRCDIR/.libs:$PATH

    ixion-parser -t $1 $PROGDIR/*.txt
}

