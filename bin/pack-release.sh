#!/bin/sh

PROGDIR=`dirname $0`
VERSION=`cat $PROGDIR/../VERSION`
DIR=libixion_$VERSION

#git clone git://gitorious.org/ixion/ixion.git $DIR || exit 1
git clone file:///home/kyoshida/Documents/Workspace/ixion $DIR || exit 1
pushd . > /dev/null
cd $DIR
rm -rf .git
rm -f .gitignore
rm -rf autom4te.cache
rm -rf slickedit

popd > /dev/null

tar jcvf $DIR.tar.bz2 $DIR

