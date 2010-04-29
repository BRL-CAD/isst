AC_INIT
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([gtk/main.c])
AM_CONFIG_HEADER(isst_config.h)
AM_INIT_AUTOMAKE(isst, 1.0)

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_LONG_64_BITS


##
## GTK
HAVE_GTK2=no
GTK_CFLAGS=
GTK_LIBS=
AM_PATH_GTK_2_0(2.6.0,[
		       HAVE_GTK2=yes
		       GTK_CFLAGS="${GTK_CFLAGS} `pkg-config --cflags gthread-2.0`"
		       GTK_LIBS="${GTK_LIBS} `pkg-config --libs gthread-2.0`"
		       ],[AC_MSG_NOTICE([GTK >= 2.0 is required for GTK+ backend])])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

##
## SDL
AC_PATH_XTRA
HAVE_SDL=no
AM_PATH_SDL(1.2.0,HAVE_SDL=yes,)

if test "x$HAVE_GTK2" = xno && "xHAVE_SDL" = xno
then
	AC_MSG_ERROR(["Requires either GTK+-2.0 or SDL 1.2+"])
fi

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

AC_CHECK_FUNCS(strnstr)

# Compile in BRLCAD support if specified by user
BRLCAD_CFLAGS="-I/usr/brlcad/include -I/usr/brlcad/include/brlcad"
BRLCAD_LIBS="-L/usr/brlcad/lib"
AC_ARG_WITH(brlcad, AC_HELP_STRING(--with-brlcad=path,
                [Specify location of BRLCAD Installation]),
        [
                BRLCAD_CFLAGS="-I$withval/include -I$withval/include/brlcad"
                BRLCAD_LIBS="-L$withval/lib"
                AC_DEFINE(HAVE_BRLCAD,1,Checking for BRLCAD Installation)
        ]
)

TIE_CFLAGS="$BRLCAD_CFLAGS -DTIE_PRECISION=0 -DHAX=1"
TIE_LIBS="$BRLCAD_LIBS -ltie -lrender"
AC_SUBST(TIE_CFLAGS)
AC_SUBST(TIE_LIBS)

AC_CONFIG_FILES([Makefile gtk/Makefile sdl/Makefile])
AC_OUTPUT

echo "I can has GTK+? ${HAVE_GTK2}"
echo "I can has SDL? ${HAVE_SDL}"
