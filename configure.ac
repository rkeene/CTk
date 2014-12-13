dnl Define ourselves
AC_INIT(ctk, @@VERS@@) 

dnl Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_GNU_SOURCE

dnl Determine system information
DC_CHK_OS_INFO

dnl Determine how to make shared objects
DC_GET_SHOBJFLAGS

dnl Perform Tcl Extension required stuff
TCLEXT_INIT

dnl Determine how to link to NCurses
AC_SEARCH_LIBS(getch, ncurses curses tinfo,, [
	AC_MSG_ERROR([Unable to determine how to link to curses])
])

AC_SEARCH_LIBS(curses_version, ncurses curses tinfo)

dnl Look for appropriate headers
AC_HEADER_STDC
AC_CHECK_HEADERS(ctype.h curses.h errno.h fcntl.h limits.h math.h ncurses.h pwd.h signal.h stddef.h stdio.h stdlib.h string.h sys/file.h sys/select.h sys/stat.h sys/time.h sys/times.h sys/types.h tcl.h unistd.h)

dnl Determine what type of targets to build
TARGETS="libctk.${SHOBJEXT}"
CFLAGS="${CFLAGS} ${SHOBJFLAGS}"
AC_SUBST(TARGETS)

dnl Produce output
AC_OUTPUT(Makefile pkgIndex.tcl)
