#!/usr/bin/env bash

PROGDIR=`dirname $0`
ORCUS_PYTHONPATH="$PROGDIR/../src/python/.libs"

export PYTHONPATH=$ORCUS_PYTHONPATH:$PYTHONPATH
exec "$1"


