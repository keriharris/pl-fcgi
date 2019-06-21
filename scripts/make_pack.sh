#!/bin/bash

########################################################################
#  This file is part of pl-fcgi.
#
#   Copyright (C) 2015 Keri Harris <keri@gentoo.org>
#
#   pl-fcgi is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as
#   published by the Free Software Foundation, either version 2.1
#   of the License, or (at your option) any later version.
#
#   pl-fcgi is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with pl-fcgi.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################

SCRIPT=$(readlink -f "$0")
SCRIPTDIR=$(dirname "$SCRIPT")
TOPDIR=$SCRIPTDIR/..

VERSION=$(cat $TOPDIR/VERSION)
PACKDIR=pack-$VERSION/fcgi

cd $TOPDIR
make distclean

cd $SCRIPTDIR
rm -rf pack-$VERSION

mkdir -p $PACKDIR
echo "name('fcgi')." > $PACKDIR/pack.pl
echo "title('fcgi - Fast Common Gateway Interface (FastCGI) library for Prolog')." >> $PACKDIR/pack.pl
echo "version('$VERSION')." >> $PACKDIR/pack.pl
echo "author('Keri Harris', 'keri@gentoo.org')." >> $PACKDIR/pack.pl
echo "download('http://dev.gentoo.org/~keri/pl-fcgi/fcgi-*.tgz')." >> $PACKDIR/pack.pl

mkdir -p $PACKDIR/prolog
cp $TOPDIR/prolog/fcgi.pl $PACKDIR/prolog/

mkdir -p $PACKDIR/src
cp $TOPDIR/src/fcgi.c $PACKDIR/src/
cp $TOPDIR/src/config.h.in $PACKDIR/src/
cp $TOPDIR/src/Makefile.in $PACKDIR/src/
sed -i -e "s:\(INSTALL_SOLIBDIR\)=.*:\1=../\$(PACKSODIR):" \
       -e "s:\(INSTALL_PLLIBDIR\)=.*:\1=../prolog:" \
       -e "/\$(INSTALL_PROGRAM)/{h;N;N;x}" $PACKDIR/src/Makefile.in

mkdir -p $PACKDIR/examples
cp $TOPDIR/examples/*.fcgi $PACKDIR/examples/
cp $TOPDIR/examples/*.html $PACKDIR/examples/

cp $TOPDIR/README $PACKDIR/
cp $TOPDIR/LICENSE $PACKDIR/
cp $TOPDIR/VERSION $PACKDIR/

cp $TOPDIR/configure.in $PACKDIR/
sed -i -e "/PKG_CHECK_MODULES(SWI/{N;N;d}" $PACKDIR/configure.in
cp $TOPDIR/aclocal.m4 $PACKDIR/
cp $TOPDIR/install-sh $PACKDIR/
cp $TOPDIR/Makefile.in $PACKDIR/

cd pack-$VERSION
tar -cvzf fcgi-$VERSION.tgz fcgi
