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

#include "isst.h"


int
do_loop(struct isst_s *isst)
{
    SDL_Event e;
    struct timeval ts[2];
    int fc = 0;

    gettimeofday(ts, NULL);

    while (1)
    {   
	int i;

	isst->buffer_image.ind = 0;
	if(isst->ogl)
	    paint_ogl(isst);
	else
	    paint_sw(isst);

	/* some FPS stuff */
	fc++;
	if(fc == 10) {
	    gettimeofday(ts+1, NULL);
	    printf("  \r%g FPS", (double)fc/(((double)ts[1].tv_sec+(double)ts[1].tv_usec/(double)1e6) - ((double)ts[0].tv_sec+(double)ts[0].tv_usec/(double)1e6)));      
	    fflush(stdout);
	    fc=0;
	    gettimeofday(ts, NULL);
	}

	while(SDL_PollEvent (&e))
	    switch (e.type)
	    {
		case SDL_VIDEORESIZE:
		    isst->w = e.resize.w;
		    isst->h = e.resize.h;
		    resize_isst(isst);
		    break;
		case SDL_KEYDOWN:
		    switch (tolower (e.key.keysym.sym))
		    {
			case 'f':
			    if(isst->sflags&SDL_FULLSCREEN)
				isst->sflags &= ~SDL_FULLSCREEN;
			    else
				isst->sflags |= SDL_FULLSCREEN;
			    resize_isst(isst);
			    break;
			case 'x':
			case 'q':
			case SDLK_ESCAPE:
			    SDL_Quit ();
			    printf("\n");
			    return EXIT_SUCCESS;
			    break;
			    /* TODO: more keys for nifty things like changing mode or pulling up gui bits or something */
		    }
		case SDL_MOUSEMOTION:
		    switch(e.motion.state) {
			case 1:
			    /* rotate xrel/yrel */
			    break;
			case 4:
			    /* zoom in/out yrel */
			    break;
		    }

	    }
    }
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
