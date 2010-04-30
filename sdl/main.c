/*
 * ISST
 *
 * Copyright (c) 2005-2010 United States Government as represented by
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
/** @file main.c
 *
 * top level for SDL version of ISST
 *
 */

#include <stdio.h>
#include <sys/time.h>

#include <SDL.h>

#include <tie/tie.h>
#include <tie/adrt.h>
#include <tie/adrt_struct.h>
#include <tie/camera.h>


struct isst_s {
    struct tie_s *tie;
    struct render_camera_s camera;
    struct camera_tile_s tile;
    struct adrt_mesh_s *meshes;
    tienet_buffer_t buffer_image;
    struct SDL_Rect r;
    struct SDL_Surface *screen;
    int ogl, sflags, w, h;
};

void
resize_isst(struct isst_s *isst)
{
    isst->r.w = isst->tile.size_x = isst->camera.w = isst->w;
    isst->r.h = isst->tile.size_y = isst->camera.h = isst->h;
    isst->r.x = isst->r.y = isst->tile.orig_x = isst->tile.orig_y = 0;
    isst->tile.format = RENDER_CAMERA_BIT_DEPTH_24;
    TIENET_BUFFER_SIZE(isst->buffer_image, 3 * isst->w * isst->h);
    isst->screen = SDL_SetVideoMode (isst->w, isst->h, 24, isst->sflags);
    if(isst->screen == NULL) {
	printf("Failed to generate display context\n");
	exit(EXIT_FAILURE);
    }
}

struct isst_s *
prep_isst(int argc, const char **argv)
{
    struct isst_s *isst;
    isst = (struct isst_s *)malloc(sizeof(struct isst_s));
    isst->tie = (struct tie_s *)bu_malloc(sizeof(struct tie_s), "tie");
    load_g(isst->tie, argv[0], argc-1, argv+1, &(isst->meshes));
    TIENET_BUFFER_INIT(isst->buffer_image);
    render_camera_init(&isst->camera, bu_avail_cpus());
    isst->camera.type = RENDER_CAMERA_PERSPECTIVE;
    isst->camera.fov = 25;
    VSETALL(isst->camera.pos.v, isst->tie->radius);
    VMOVE(isst->camera.focus.v, isst->tie->mid);
    render_phong_init(&isst->camera.render, NULL);
    return isst;
}

void
paint_ogl(struct isst_s *isst)
{
    render_camera_prep(&isst->camera);
    render_camera_render(&isst->camera, isst->tie, &isst->tile, &isst->buffer_image);
}

void
paint_sw(struct isst_s *isst)
{
    int i;
    render_camera_prep(&isst->camera);
    render_camera_render(&isst->camera, isst->tie, &isst->tile, &isst->buffer_image);
    for(i=0;i<isst->h;i++)
	memcpy(isst->screen->pixels + i * isst->screen->pitch, 
		isst->buffer_image.data + i * isst->w * 3, 
		isst->screen->w*3);
    SDL_UpdateRect(isst->screen, 0, 0, 0, 0);
}

int
main(int argc, char **argv)
{
    struct isst_s *isst;
    int w = 800, h = 600, c, ogl = 0, sflags = SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_RESIZABLE;

    const char opts[] = 
	/* or would it be better to */
#ifdef HAVE_OPENGL
	"fw:h:g";
#else
	"fw:h:";
#endif

    while((c=getopt(argc, argv, opts)) != -1)
	switch(c) {
	    case 'f':
		sflags |= SDL_FULLSCREEN;
		break;
	    case 'w':
		w = atoi(optarg);
		break;
	    case 'h':
		h = atoi(optarg);
		break;
	    case 'g':
		sflags |= SDL_OPENGL;
		ogl = 1;
		break;
	    case ':':
	    case '?':
		printf("Whu?\n");
		return EXIT_FAILURE;
    }
    printf("optind: %d\n", optind);
    argc -= optind;
    argv += optind;

    if(w < 1 || h < 1) {
	printf("Bad screen resolution specified\n");
	return EXIT_FAILURE;
    }
    if(argc < 2) {
	printf("Must give .g file and list of tops\n");
	return EXIT_FAILURE;
    }

    SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER);
    atexit (SDL_Quit);

    isst = prep_isst(argc, (const char **)argv);
    isst->sflags |= sflags;
    isst->ogl = ogl;
    isst->w = w;
    isst->h = h;
    resize_isst(isst);

    /* main event loop */
    return do_loop(isst);
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
