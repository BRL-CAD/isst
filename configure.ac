AC_INIT
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([gtk/main.c])
AM_CONFIG_HEADER(isst_config.h)
AM_INIT_AUTOMAKE(isst, 1.0)

AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_LONG_64_BITS

AC_CHECK_FUNCS(strnstr getopt)
AC_CHECK_HEADERS([unistd.h sys/time.h])

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

if test "x$HAVE_GTK2" = xno && test "x$HAVE_SDL" = xno
then
	AC_MSG_ERROR(["Requires either GTK+-2.0 or SDL 1.2+"])
fi

AM_PATH_OPENGL(AC_DEFINE(HAVE_OPENGL,,OpenGL exists), AC_MSG_WARN(Cannot find OpenGL, doing without))
AC_SUBST(GL_CFLAGS)
AC_SUBST(GL_LIBS)

AGAR_CFLAGS=""
AGAR_LIBS=""
have_agar=no
AC_ARG_WITH(agar, [  --with-agar=path        Specify location of the AGAR package], [
		   have_agar=yes
		   AGAR_CFLAGS="-I$withval/include/agar"
		   AGAR_LIBS="-L$withval/lib -lag_gui -lag_core -lfreetype"
		   AC_DEFINE(HAVE_AGAR,1,[AGAR])
		   ])
AC_SUBST(AGAR_CFLAGS)
AC_SUBST(AGAR_LIBS)

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

# Compile in BRLCAD support if specified by user
BRLCAD_CFLAGS="`brlcad-config --cflags`"
BRLCAD_LIBS="`brlcad-config --libs`"
AC_ARG_WITH(brlcad, AC_HELP_STRING(--with-brlcad=path,
                [Specify location of BRLCAD Installation]),
        [
                BRLCAD_CFLAGS="`$withval/bin/brlcad-config --cflags` -I$withval/include/tie"
                BRLCAD_LIBS="`$withval/bin/brlcad-config --libs gcv`"
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
echo "I can has AGAR? ${have_agar}"
