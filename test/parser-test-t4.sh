#!/usr/bin/env bash

PROGDIR=`dirname $0`
source $PROGDIR/parser-test-func.sh

THREAD=`echo $0 | sed -e 's/.*t\([0-9]\)\.sh/\1/g'`

exec_test $THREAD

