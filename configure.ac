AC_INIT([gqlplus], [1.14])
AC_CONFIG_SRCDIR([gqlplus.c])
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_CHECK_LIB(ncurses, tgetnum,,
[
	echo "The required library ncurses not found - aborting."
	exit
],)
AC_CHECK_LIB(readline, tputs,,
[
	echo "The required library readline not found - aborting."
	exit
],)
AC_OUTPUT(Makefile)
