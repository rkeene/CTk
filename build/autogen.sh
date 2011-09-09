#! /bin/bash

if [ -n "${VERS}" ]; then
	sed "s|@@VERS@@|${VERS}|g" configure.ac > configure.ac.new
	cat configure.ac.new > configure.ac
	rm -f configure.ac.new
fi

cat aclocal/*.m4 > aclocal.m4 || exit 1
autoconf || exit 1
rm -rf autom4te.cache/

exit 0
