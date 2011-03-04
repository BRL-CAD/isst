
bin_PROGRAMS=isst_sdl
lib_LTLIBRARIES=libmyplugin.la

isst_sdl_SOURCES=main.c event.c
ffu_SOURCES=ffu.c

libmyplugin_la_SOURCES=myplugin.c

AM_CFLAGS=${SDL_CFLAGS} @TIE_CFLAGS@ @GL_CFLAGS@ @AGAR_CFLAGS@ -D_GNU_SOURCE=1 -D_REENTRANT
ffu_CFLAGS=-I/opt/local/include -I/opt/local/include/freetype2 -I/usr/local/include -I/usr/local/include/freetype2
AM_LDFLAGS=${SDL_LIBS} @TIE_LIBS@ @GL_LIBS@ @AGAR_LIBS@ -lSDL -lpthread -lm -ljpeg
ffu_LDADD=-lfreetype

noinst_HEADERS = isst.h
