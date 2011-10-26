#!/bin/sh

VERSION=0.4.0
DIR=libixion_$VERSION

#git clone git://gitorious.org/ixion/ixion.git $DIR || exit 1
git clone file:///home/kyoshida/Documents/Workspace/ixion $DIR || exit 1
pushd . > /dev/null
cd $DIR
rm -rf .git
rm -f .gitignore

touch ChangeLog

if [ ! -e ltmain.sh ]; then
    libtoolize
fi

aclocal -I m4
automake --gnu --add-missing
autoconf
popd > /dev/null

tar jcvf $DIR.tar.bz2 $DIR

