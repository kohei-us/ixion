#!/usr/bin/env bash

PROJ_PREFIX=ixion
PKG_PREFIX=lib$PROJ_PREFIX

# Pick up the version number string from configure.ac.
VER_MAJOR=$(grep "m4_define.*\[ixion_version_major\]" ./configure.ac | sed -e "s/.*\[\([0-9]*\)\].*/\1/g")
VER_MINOR=$(grep "m4_define.*\[ixion_version_minor\]" ./configure.ac | sed -e "s/.*\[\([0-9]*\)\].*/\1/g")
VER_MICRO=$(grep "m4_define.*\[ixion_version_micro\]" ./configure.ac | sed -e "s/.*\[\([0-9]*\)\].*/\1/g")
VER="$VER_MAJOR.$VER_MINOR.$VER_MICRO"

PKGS=$(ls $PKG_PREFIX-$VER.tar.*)

echo "## Checksums for Packages"
echo ""

echo "| name | sha256sum | size |"
echo "|-----|-----------|------|"

for _PKG in $PKGS; do
    _HASH=$(sha256sum $_PKG | sed -e "s/^\(.*\)$PKG_PREFIX.*/\1/g" | tr -d "[:space:]")
    _SIZE=$(stat -c "%s" $_PKG)
    echo "| $_PKG | $_HASH | $_SIZE |"
done

