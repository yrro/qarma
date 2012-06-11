AC_INIT([qarma], [1], [sam@robots.org.uk])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign silent-rules -Wall])

AC_PROG_CXX
AC_CHECK_TOOL(WINDRES, windres)
if test -z "$WINDRES"; then
	AC_MSG_ERROR(windres not found in PATH)
fi

AC_CONFIG_SRCDIR([qarma.cpp])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
