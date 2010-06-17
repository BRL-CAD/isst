/*
 * ISST - fast font utility
 *
 * Copyright (c) 2010-2010 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

/** @file ffu.c
 *
 * Generate font image files for test rendering in ISST/SDL
 */

#ifdef HAVE_CONFIG_H
# include "isst_config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

void
paint(char *buf, char *glyph, int rows, int width, int stride)
{
    glyph += (rows-1)*width;
    while(rows--) {
	int i;
	for(i=0;i<width;i++) {
	    buf[i+0] = glyph[i];
	    buf[i+1] = glyph[i];
	    buf[i+2] = glyph[i];
	}
	glyph -= width;
	buf += 3*stride;
    }
}

void
writec(int fd, unsigned char *buf, size_t len)
{
    FILE *f;
    int i = 4;
    f = fdopen(fd, "w");
    fprintf(f, "char font[%d] = {\n    ", len);
    do {
	fprintf(f, "0x%02x, ", (unsigned int)*buf);
	buf++;
	len--;
	i+=6;
	if(i>78) {
	    fprintf(f, "\n    ");
	    i = 4;
	}
    } while(len > 0);
    fprintf(f, "0x%02x\n};\n", *buf);
    return;
}

int
main(int argc, char **argv)
{
    int c, i, fd, s = 512, genc = 0;
    char *buf;
    FT_Library l;
    FT_Face f;

    /* getopt stuff */

    while((c=getopt(argc, argv, "vhs:c")) != -1) {
	switch(c) {
	    case 'v':
		printf("Fast Font Utility 1.0\n");
		return 0;
	    case 'h':
		printf("%s [-h|-v] [-c] [-s <size>] <file.ttf> <output>\n", *argv);
		return 0;
	    case 's':
		s = atoi(optarg);
		break;
	    case 'c':
		genc = 1;
		break;
	    case ':':
		printf("Missing argument\n");
		return -1;
	    case '?':
		printf("Unknown option\n");
		return -1;
	}
    }
    argv += optind;
    argc -= optind;
    if(argc != 2) {
	printf("Missing file names\n");
	return -1;
    }

    if(s&0x7) {
	printf("size must be a multiple of 16.\n");
	return -1;
    }

    buf = (void *)malloc(sizeof(char) * s*s*3);
    memset(buf, 0, s*s*3);

    /* open font */
    i = FT_Init_FreeType(&l);
    if(i) {
	printf("Some freetype init error: %d\n", i);
	return -1;
    }
    i = FT_New_Face(l, *argv, 0, &f);
    if(i) {
	printf("Error loading ttf: %d\n", i);
	return -1;
    }

    printf("%d glyphs %dx%d (%dx%d) from %s, heading to %s\n", f->num_glyphs, s, s, (s>>4), (s>>4), *argv, argv[1]);

    i = FT_Set_Pixel_Sizes(f, 0, s>>4);
    if(i) {
	printf("Error setting pixel size: %d\n", i);
	return -1;
    }

    for(i=0;i<=0xff;i++) 
    {
	if(isprint(i)) {
	    if(FT_Load_Glyph(f,FT_Get_Char_Index(f, i),FT_LOAD_DEFAULT)) {
		printf("Error loading glyph for %c(%d)\n", i, i);
		return -1;
	    }
	    FT_Render_Glyph(f->glyph, FT_RENDER_MODE_NORMAL);
	    paint( buf + 
		    (((i>>4)*s + (i&0xf))
		     * (s>>4) - s * f->glyph->bitmap_top) * 3, f->glyph->bitmap.buffer ,f->glyph->bitmap.rows, f->glyph->bitmap.width, s);
	}
    }

    fd = open(argv[1], O_CREAT|O_TRUNC|O_WRONLY, 0644);
    if(fd==-1) { perror("Open");  return -1; }
    if(genc) {
	writec(fd, buf, s*s*3);
    } else {
	write(fd, buf, s*s*3);
    }
    close(fd);
    free(buf);

    return 0;
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
