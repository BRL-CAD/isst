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

#include <ft2build.h>
#include FT_FREETYPE_H

int
main(int argc, char **argv)
{
    int i, s = 512, ss;
    char c, *buf;
    FT_Library l;
    FT_Face f;

    /* getopt stuff */

    if(s&0x7) {
	printf("size must be a multiple of 16.\n");
	return -1;
    }
    ss = s>>4;

    buf = (void *)malloc(sizeof(char) * s*s*3);

    /* open font */
    i = FT_Init_FreeType(&l);
    if(i) {
	printf("Some freetype init error: %d\n", i);
	return -1;
    }
    i = FT_New_Face(l, argv[1], 0, &f);
    if(i) {
	printf("Error loading ttf: %d\n", i);
	return -1;
    }

    printf("%d glyphs %dx%d (%dx%d)\n", f->num_glyphs, s, s, ss, ss);

    i = FT_Set_Pixel_Sizes(f, 0, s>>4);
    if(i) {
	printf("Error setting pixel size: %d\n", i);
	return -1;
    }
	
    for(i=0;i<=0xff;i++) {
	if(isprint(i)) {
	    int gi;
	    int x;
	    char *bm;

	    gi = FT_Get_Char_Index(f, i);
	    if(FT_Load_Glyph(f,gi,FT_LOAD_DEFAULT)) {
		printf("Error loading glyph index %d for %c(%d)\n", gi, i, i);
		return -1;
	    }
	    FT_Render_Glyph(f->glyph, FT_RENDER_MODE_NORMAL);
	    bm = f->glyph->bitmap.buffer;
	    printf("%c rows:%02d width:%02d pitch:%02d\n", i, f->glyph->bitmap.rows, f->glyph->bitmap.width, f->glyph->bitmap.pitch);
	    /* paint to ss*i&0xf, ss*i&0xf0>>8 */
	}
    }
    /* close font */
    /* open file */
    /* write buf to pix file */
    /* close file */
    free(buf);
    printf("\n");

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
