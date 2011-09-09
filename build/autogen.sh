#! /bin/bash

cat aclocal/*.m4 > aclocal.m4 || exit 1
autoconf || exit 1
rm -rf autom4te.cache/

exit 0
