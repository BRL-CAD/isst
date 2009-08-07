bin_PROGRAMS = isst
isst_SOURCES = gui.c load_g.c main.c
noinst_HEADERS = isst.h

AM_CFLAGS = \
	@TIE_CFLAGS@ \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	@GTHREAD_CFLAGS@ \
	@PANGO_CFLAGS@ \
	@ADRT_CFLAGS@ \
	-DPANGO_ENABLE_BACKEND

AM_LDFLAGS = \
	@TIE_LIBS@ \
	@GTK_LIBS@ \
	@GLIB_LIBS@ \
	@GTHREAD_LIBS@ \
	@PANGO_LIBS@ \
	@ADRT_LIBS@ \
	-lpthread
