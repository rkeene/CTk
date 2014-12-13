#! /bin/bash

if [ "$1" = '--update' ]; then
	(
		mkdir aclocal >/dev/null 2>/dev/null
		cd aclocal || exit 1

		for file in shobj.m4 tcl.m4; do
			rm -f "${file}"

			wget -O "${file}.new" "http://rkeene.org/devel/autoconf/${file}" || continue

			mv "${file}.new" "${file}"
		done
	)
fi

cat aclocal/*.m4 > aclocal.m4 || exit 1
autoconf || exit 1
rm -rf autom4te.cache/

exit 0
