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
};

struct isst_s *
prep_isst(int argc, const char **argv, SDL_Surface *screen)
{
    struct isst_s *isst;
    isst = (struct isst_s *)malloc(sizeof(struct isst_s));
    isst->r.w = isst->tile.size_x = isst->camera.w = screen->w;
    isst->r.h = isst->tile.size_y = isst->camera.h = screen->h;
    isst->r.x = isst->r.y = isst->tile.orig_x = isst->tile.orig_y = 0;
    isst->tile.format = RENDER_CAMERA_BIT_DEPTH_24;
    render_camera_init(&isst->camera, bu_avail_cpus());
    isst->camera.type = RENDER_CAMERA_PERSPECTIVE;
    isst->camera.fov = 25;
    VSETALL(isst->camera.pos.v, 1);
    VSETALL(isst->camera.focus.v, 0);
    render_phong_init(&isst->camera.render, NULL);
    isst->tie = (struct tie_s *)bu_malloc(sizeof(struct tie_s), "tie");
    TIENET_BUFFER_SIZE(isst->buffer_image, 3*screen->w*screen->h);
    load_g(isst->tie, argv[0], argc-1, argv+1, &(isst->meshes));
    return isst;
}

int
do_loop(SDL_Surface *screen, struct isst_s *isst)
{
    SDL_Event e;
    struct timeval ts[2];
    int fc = 0;

    gettimeofday(ts, NULL);

    while (1)
    {   
	int i;

	isst->buffer_image.ind = 0;
	render_camera_prep(&isst->camera);
	render_camera_render(&isst->camera, isst->tie, &isst->tile, &isst->buffer_image);
	memcpy(screen->pixels, isst->buffer_image.data, screen->w*screen->h*3);
	SDL_UpdateRect(screen, 0, 0, 0, 0);

	/* some FPS stuff */
	fc++;
	if(fc == 10) {
	    gettimeofday(ts+1, NULL);
	    printf("  \r%g FPS", (double)fc/(((double)ts[1].tv_sec+(double)ts[1].tv_usec/(double)1e6) - ((double)ts[0].tv_sec+(double)ts[0].tv_usec/(double)1e6)));      
	    fflush(stdout);
	    fc=0;
	    gettimeofday(ts, NULL);
	}

	/* we can SDL_PollEvent() for continuous rendering */
	SDL_WaitEvent (&e);
	switch (e.type)
	{
	    case SDL_VIDEORESIZE:
		printf("Resize!\n");
		break;
	    case SDL_KEYDOWN:
		switch (tolower (e.key.keysym.sym))
		{
		    case 'x':
		    case 'q':
		    case SDLK_ESCAPE:
			SDL_Quit ();
			printf("\n");
			return EXIT_SUCCESS;
			break;
			/* TODO: more keys for nifty things like changing mode or pulling up gui bits or something */
		}
		/* TODO: look for mouse events */
	}
    }
}


int
main(int argc, char **argv)
{
    SDL_Surface *screen;
    struct isst_s *isst;
    int w = 800, h = 600, c;
    int ogl = 0;

    const char opts[] = 
	/* or would it be better to */
#ifdef HAVE_OPENGL
	"w:h:g";
#else
	"w:h:";
#endif

    while((c=getopt(argc, argv, opts)) != -1)
	switch(c) {
	    case 'w':
		w = atoi(optarg);
		break;
	    case 'h':
		h = atoi(optarg);
		break;
	    case 'g':
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

    /* can we make this resizable? */
    screen = SDL_SetVideoMode (w, h, 24, SDL_DOUBLEBUF|SDL_HWSURFACE|SDL_RESIZABLE);

    isst = prep_isst(argc, (const char **)argv, screen);

    /* main event loop */
    return do_loop(screen, isst);
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
