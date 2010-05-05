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

#ifdef HAVE_CONFIG_H
# include "isst_config.h"
#endif

#include <stdio.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include <SDL.h>

#include <tie.h>
#include <adrt.h>
#include <adrt_struct.h>
#include <camera.h>

#include "isst.h"


int
do_loop(struct isst_s *isst)
{
    SDL_Event e;
    struct timeval ts[2];
    int fc = 0;
    double dt = 1.0, dt2 = 0.0;
    double mouse_sensitivity = 0.1;

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
	gettimeofday(ts+1, NULL);
	dt = (((double)ts[1].tv_sec+(double)ts[1].tv_usec/(double)1e6) - ((double)ts[0].tv_sec+(double)ts[0].tv_usec/(double)1e6));
	dt2 += dt;

	gettimeofday(ts, NULL);
	if(dt2 > 0.5) {
	    printf("  \r%g FPS", (double)fc/dt2);
	    fflush(stdout);
	    fc=0;
	    dt2 = 0;
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
			case '1': render_shader_init(&isst->camera.render, "phong", NULL); break;
			case '2': render_shader_init(&isst->camera.render, "normal", NULL); break;
			case '3': render_shader_init(&isst->camera.render, "depth", NULL); break;
			case '4': render_shader_init(&isst->camera.render, "component", NULL); break;

			    /* TODO: more keys for nifty things like changing mode or pulling up gui bits or something */
		    }
		case SDL_MOUSEMOTION:
		    switch(e.motion.state) {
			case 1:
			    /* rotate xrel/yrel */
			    break;
			case 4:
			    /* zoom in/out yrel */
			    {
				/* if you zoom past the focus point, it flips
				 * direction. up is awlays towards the focus. */
				vect_t vec;
				VSUB2(vec, isst->camera.focus.v, isst->camera.pos.v);
				VUNITIZE(vec);
				VSCALE(vec, vec,  - mouse_sensitivity * dt * isst->tie->radius * e.motion.yrel);
				VADD2(isst->camera.pos.v,  isst->camera.pos.v, vec);
			    }
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
