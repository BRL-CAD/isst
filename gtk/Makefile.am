bin_PROGRAMS = isst_gtk
isst_gtk_SOURCES = gui.c main.c net_worker.c local_worker.c
noinst_HEADERS = isst.h

AM_CFLAGS = \
	@TIE_CFLAGS@ \
	@GTK_CFLAGS@ \
	-DPANGO_ENABLE_BACKEND \
	-fexceptions

AM_LDFLAGS = \
	@TIE_LIBS@ \
	@GTK_LIBS@
