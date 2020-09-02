#!/usr/bin/env bash

PROJ_PREFIX=ixion
PKG_PREFIX=lib$PROJ_PREFIX

# Pick up the version number string from configure.ac.
VER_MAJOR=$(cat ./configure.ac | grep -E "ixion_major_version.*[0-9]" | sed -e "s/.*\[\([0-9][0-9]*\).*/\1/g")
VER_MINOR=$(cat ./configure.ac | grep -E "ixion_minor_version.*[0-9]" | sed -e "s/.*\[\([0-9][0-9]*\).*/\1/g")
VER_MICRO=$(cat ./configure.ac | grep -E "ixion_micro_version.*[0-9]" | sed -e "s/.*\[\([0-9][0-9]*\).*/\1/g")
VER="$VER_MAJOR.$VER_MINOR.$VER_MICRO"

PKGS=$(ls $PKG_PREFIX-$VER.tar.*)

echo "## Release Notes"
echo ""
echo "* add item"
echo ""
echo "## Source packages for distribution"
echo ""

echo "| URL | sha256sum | size |"
echo "|-----|-----------|------|"

for _PKG in $PKGS; do
    _URL="[$_PKG](https://kohei.us/files/$PROJ_PREFIX/src/$_PKG)"
    _HASH=$(sha256sum $_PKG | sed -e "s/^\(.*\)$PKG_PREFIX.*/\1/g" | tr -d "[:space:]")
    _SIZE=$(stat -c "%s" $_PKG)
    echo "| $_URL | $_HASH | $_SIZE |"
done

