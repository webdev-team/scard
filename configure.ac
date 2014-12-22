dnl configure script for the scard project
dnl Copyright (C) 2014 RTL France
dnl
dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2.1 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl 
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

AC_INIT([scard], [0.0.1], [michel.depeige@rtl.fr])
AC_CONFIG_SRCDIR([src/scard/main.c])
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS(src/include/config.h)

AC_PROG_CC
AM_PROG_CC_C_O

AC_C_INLINE

AC_DEFUN([AC_PROG_CXX], [])
AC_DEFUN([AC_PROG_F77], [])

dnl
dnl flags
dnl Add -Wall and friends if we are using GCC.
dnl

AC_SUBST(AM_CFLAGS)
AC_SUBST(CPPFLAGS)

AC_C_CONST

dnl
dnl types check
dnl

AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T

dnl
dnl headers check
dnl

AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/time.h syslog.h unistd.h])

dnl
dnl fonctions check
dnl

AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_VPRINTF
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([strdup strerror])

dnl
dnl paths
dnl

libexecdir='${exec_prefix}/libexec/scard'

dnl strtok_r specific checks
AC_CHECK_FUNC(strtok_r, [
	AC_DEFINE(HAVE_STRTOK_R, [], ["Define to 1 if you have the `strtok_r' function."])
	ac_found_strtok_r=yes
	], [
	HAVE_STRTOK_R="# "
	])

AC_SUBST(HAVE_STRTOK_R)
if test "$ac_found_strtok_r" = "yes"; then
	AC_MSG_CHECKING(if strtok_r declaration requires -D__EXTENSIONS__)
	AC_EGREP_HEADER(strtok_r, string.h, [
		AC_MSG_RESULT(no)
		], [
		ac_save_cppflags="$CPPFLAGS"
		CPPFLAGS="$ac_save_cppflags -D__EXTENSIONS__"
		AC_EGREP_HEADER(strtok_r, string.h, [
			ac_save_cppflags=
			AC_MSG_RESULT(yes)
		], [
			CPPFLAGS="$ac_save_cppflags"
			AC_MSG_RESULT(still can't find it)
		])
	])
fi

dnl
dnl structures checks
dnl

AC_STRUCT_TM

dnl
dnl enable / disable debug
dnl

AC_ARG_ENABLE(debug,
[  --enable-debug          specify you are building with debugging on],
	AM_CFLAGS="-W -Wall -std=c99 -pedantic -g3" ,
	AM_CFLAGS="-W -Wall -std=c99 -pedantic -DNDEBUG"
)
AC_SUBST(AM_CFLAGS)

dnl solaris needs -lsocket -lnsl
AC_SEARCH_LIBS(socket,socket)
AC_SEARCH_LIBS(getnameinfo,nsl socket)
AC_SEARCH_LIBS(inet_ntoa,nsl socket)

dnl
dnl libevent
dnl

AC_ARG_WITH(libevent,
	[  --with-libevent=PATH    Use libevent in PATH],
	[
		if test -d "$withval"; then
			CPPFLAGS="${CPPFLAGS} -I$withval/include"
			LDFLAGS="${LDFLAGS} -L$withval/lib"
		fi
	]
)

AC_CHECK_HEADER(event.h,
	AC_CHECK_LIB(event, event_init, ,
		AC_MSG_ERROR([*** libevent not found - cannot continue ***])
	)
	AC_CHECK_LIB(event, event_loopbreak, 
		AC_DEFINE(HAVE_EVENT_LOOPBREAK,[],[having event_loopbreak()]))
	AC_CHECK_LIB(event, evbuffer_add_vprintf, ,
		AC_MSG_ERROR([*** libevent >= 1.3 not found - cannot continue ***])
	),
	AC_MSG_ERROR([*** libevent not found - cannot continue ***])
)

dnl
dnl Set PACKAGE_SYSCONF_DIR in config.h.
dnl

if test "x${prefix}" = "xNONE"; then
        PACKAGE_SYSCONF_DIR="${ac_default_prefix}/etc/"
else
        PACKAGE_SYSCONF_DIR="${prefix}/etc/"
fi

AC_DEFINE_UNQUOTED(PACKAGE_SYSCONF_DIR, "$PACKAGE_SYSCONF_DIR",
        [The directory in which the configuration will be stored])

dnl
dnl Create Makefiles
dnl

if test "$enable_quotes" = yes; then
	QUOTES=quotes
else
	QUOTES=
fi
AC_SUBST([QUOTES])

AC_OUTPUT([Makefile\
	etc/Makefile\
	src/include/Makefile\
	src/scard/Makefile\
	src/Makefile]
)
