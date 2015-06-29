#!/usr/bin/env bash

PROGDIR=`dirname $0`
IXION_PYTHONPATH="$PROGDIR/../src/python/.libs"

export PYTHONPATH=$IXION_PYTHONPATH:$PYTHONPATH
exec $PWD/"$1"


