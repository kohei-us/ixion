#!/usr/bin/env bash
# Use this script on formula/inc/core_resources.hrc in libreoffice repo.

cat $1 \
    | grep -E "NC_.*FUNCTION_NAMES" \
    | sed -e "s/.*_FUNCTION_NAMES\",\ \"//g" -e "s/\").*//g" \
    | grep -v "#" \
    | grep -v "\." \
    | grep -v "_OOO" | sort | uniq
