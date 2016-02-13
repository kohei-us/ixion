#!/usr/bin/env bash

PROGDIR=`dirname $0`
SRCDIR=$PROGDIR/../src

export PATH=$SRCDIR:$SRCDIR/.libs:$PATH

ixion-parser $PROGDIR/*.txt

