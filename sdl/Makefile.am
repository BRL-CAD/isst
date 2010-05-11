
bin_PROGRAMS=isst_sdl
lib_LTLIBRARIES=libmyplugin.la

isst_sdl_SOURCES=main.c event.c

libmyplugin_la_SOURCES=myplugin.c

AM_CFLAGS=${SDL_CFLAGS} @TIE_CFLAGS@ @GL_CFLAGS@ -I/agar-install-path/include/agar -D_GNU_SOURCE=1 -D_REENTRANT
AM_LDFLAGS=${SDL_LIBS} @TIE_LIBS@ @GL_LIBS@ -L/agar-install-path/lib -lag_gui -lag_core -lSDL -lpthread -lm -ljpeg

noinst_HEADERS = isst.h
