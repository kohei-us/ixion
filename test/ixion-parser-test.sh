#!/usr/bin/env bash

PROGDIR=`dirname $0`
PARSER=$PROGDIR/../bin/ixion-parser.sh
$PARSER $PROGDIR/*.txt

