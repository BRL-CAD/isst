
bin_PROGRAMS=isst_sdl
lib_LTLIBRARIES=libmyplugin.la

isst_sdl_SOURCES=main.c event.c

libmyplugin_la_SOURCES=myplugin.c

AM_CFLAGS=${SDL_CFLAGS} @TIE_CFLAGS@ @GL_CFLAGS@
AM_LDFLAGS=${SDL_LIBS} @TIE_LIBS@ @GL_LIBS@

noinst_HEADERS = isst.h
