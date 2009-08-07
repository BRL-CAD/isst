AC_INIT
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([src/main.c])
AM_CONFIG_HEADER(isst_config.h)
AM_INIT_AUTOMAKE(isst, 1.0)

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_LONG_64_BITS

##
## GTK
##
AM_PATH_GTK_2_0(2.6.0,,AC_MSG_ERROR([GTK >= 2.0 is required]))
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

##
## GLIB
##
AC_MSG_CHECKING(for GLIB)
if pkg-config --exists glib-2.0; then
	GLIB_CFLAGS="`pkg-config --cflags glib-2.0`"
	GLIB_LIBS="`pkg-config --libs glib-2.0`"
	GLIB_VER="`pkg-config --modversion glib-2.0`"
	AC_MSG_RESULT(yes (version $GLIB_VER))
else
	AC_MSG_RESULT(no)
	AC_MSG_ERROR([GLIB >= 2.0 is required])
fi
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

##
## GTHREAD
##
AC_MSG_CHECKING(for GTHREAD)
if pkg-config --exists gthread-2.0; then
	GTHREAD_CFLAGS="`pkg-config --cflags gthread-2.0`"
	GTHREAD_LIBS="`pkg-config --libs gthread-2.0`"
	GTHREAD_VER="`pkg-config --modversion gthread-2.0`"
	AC_MSG_RESULT(yes (version $GTHREAD_VER))
else
	AC_MSG_RESULT(no)
	AC_MSG_ERROR([GTHREAD >= 2.0 is required])
fi
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

##
## Pango
##
AC_MSG_CHECKING(for PANGO)
if pkg-config --exists pango; then
	PANGO_CFLAGS="`pkg-config --cflags pango`"
	PANGO_LIBS="`pkg-config --libs pango`"
	PANGO_VER="`pkg-config --modversion pango`"
	AC_MSG_RESULT(yes (version $PANGO_VER))
else
	AC_MSG_RESULT(no)
	AC_MSG_ERROR([PANGO is required])
fi
AC_SUBST(PANGO_CFLAGS)
AC_SUBST(PANGO_LIBS)

### ##
### ## MYSQL
### ##
### AC_PATH_PROG(MYSQL_CONFIG, mysql_config, , $PATH)
### AC_MSG_CHECKING(for MYSQL)
### if test "x$MYSQL_CONFIG" = "x" ; then
### 	AC_MSG_RESULT(no)
### 	AC_MSG_ERROR([MYSQL is required])
### else
### 	MYSQL_CFLAGS="`$MYSQL_CONFIG --cflags`"
### 	MYSQL_LIBS="`$MYSQL_CONFIG --libs`"
### 	MYSQL_VER="`$MYSQL_CONFIG --version`"
### 	AC_MSG_RESULT(yes (version $MYSQL_VER))
### fi
### AC_SUBST(MYSQL_CFLAGS)
### AC_SUBST(MYSQL_LIBS)


AC_MSG_CHECKING(whether to compile for debugging)
want_debug=no
AC_ARG_ENABLE(debug,
	AC_HELP_STRING(--enable-debug, [Build with debugging [default=no]]),
	[
		if test "x$enableval" = "xyes"
		then
			want_debug=yes
		fi
	]
)


## Check for ADRTInstallation
## "/usr/adrt" by default

# Compile in ADRT support if specified by user
with_adrtpfx=no
AC_ARG_WITH(adrt, AC_HELP_STRING(--with-adrt=path,
                [Specify location of ADRT Installation]),
        [
                ADRT_CFLAGS="-I$withval/include"
                ADRT_LIBS="$withval/lib/librender.la"
                AC_DEFINE(HAVE_ADRT,1,Checking for ADRT Installation)
                with_adrtpfx=yes
        ]
)
AC_SUBST(ADRT_CFLAGS)
AC_SUBST(ADRT_LIBS)
AM_CONDITIONAL(HAVE_ADRT, [test "x$with_adrtpfx" != "xno"])

TIE_CFLAGS="-I/usr/brlcad/include -I/usr/brlcad/include/brlcad -DTIE_PRECISION=0 -DHAX=1"
TIE_LIBS="-L/usr/brlcad/lib -ltie -lrender -lrt -lgcv"
AC_SUBST(TIE_CFLAGS)
AC_SUBST(TIE_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile utils/Makefile])
AC_OUTPUT
