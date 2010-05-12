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

#include <SDL.h>

#include <tie.h>
#include <adrt.h>
#include <adrt_struct.h>
#include <camera.h>

#include "isst.h"

void 
move_walk(struct isst_s * isst, double dist)
{
    vect_t vec;
    VSUB2(vec, isst->camera.focus.v, isst->camera.pos.v);
    VUNITIZE(vec);
    VSCALE(vec, vec, dist * isst->tie->radius);
    VADD2(isst->camera.pos.v, isst->camera.pos.v, vec);
    if(dist < 0) VSCALE(vec, vec, -1);
    VADD2(isst->camera.focus.v, isst->camera.pos.v, vec);
}

void 
move_strafe(struct isst_s * isst, double dist)
{
    vect_t vec, dir, up;
    VSET(up, 0, 0, 1);
    VSUB2(dir, isst->camera.focus.v, isst->camera.pos.v);
    VUNITIZE(dir);
    VCROSS(vec, dir, up);
    VSCALE(vec, vec, dist * isst->tie->radius);
    VADD2(isst->camera.pos.v, isst->camera.pos.v, vec);
    VADD2(isst->camera.focus.v, isst->camera.pos.v, dir);
}

void 
move_float(struct isst_s * isst, double dist)
{
    isst->camera.pos.v[2] += 2*dist;
    isst->camera.focus.v[2] += 2*dist;
}

void
zero_view(struct isst_s *isst) 
{
    vect_t vec;
    VSUB2(vec, isst->tie->mid, isst->camera.pos.v);
    VUNITIZE(vec);
    VADD2(isst->camera.focus.v, isst->camera.pos.v, vec);
}

void 
look(struct isst_s * isst, double x, double y)
{
    double az, el;
    vect_t vec;

    /* generate az/el (oddly, this generates degrees instead of radians) */
    VSUB2(vec, isst->camera.pos.v, isst->camera.focus.v);
    VUNITIZE(vec);
    AZEL_FROM_V3DIR(az, el, vec);
    az *= -DEG2RAD;
    el *= -DEG2RAD;

    az -= 0.5*x;
    el -= 0.5*y;

    /* clamp to sane values */
    while(az > 2*M_PI) az -= 2*M_PI;
    while(az < 0) az += 2*M_PI;
    if(el>M_PI_2) el=M_PI_2;
    if(el<-M_PI_2) el=-M_PI_2;

    /* generate the new lookat point */
    V3DIR_FROM_AZEL(vec, az, el);
    VADD2(isst->camera.focus.v, isst->camera.pos.v, vec);
}

int
do_loop(struct isst_s *isst)
{
    SDL_Event e;
    double ts[2];
    int fc = 0, showfps = 1;
    double dt = 1.0, dt2 = 0.0;
    double mouse_sensitivity = 0.1;
    double val = 1;
    int vel[3] = { 0, 0, 0 };
    char buf[BUFSIZ];

    ts[0] = SDL_GetTicks();

    while (1)
    {   
	int i;

	isst->buffer_image.ind = 0;
#ifdef HAVE_OPENGL
	if(isst->ogl)
	    paint_ogl(isst);
	else
#endif
	    paint_sw(isst);

	/* some FPS stuff */
	fc++;
	ts[1] = SDL_GetTicks();
	dt = 0.001 * (ts[1] - ts[0]);
	dt2 += dt;
	ts[0] = SDL_GetTicks();

	if(showfps && dt2 > 0.5) {
	    printf("  \r%g FPS", (double)fc/dt2);
	    fflush(stdout);
	    fc=0;
	    dt2 = 0;
	}

#define MAXFPS 10.0
	if(dt < 1.0/MAXFPS)
	    SDL_Delay(1000 * 1.0/MAXFPS - dt);
#undef MAXFPS

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
			case '=': snprintf(buf, BUFSIZ, "%f", val); render_shader_init(&isst->camera.render, "myplugin", buf); break;
			case '[': val -= 0.1; snprintf(buf, BUFSIZ, "%f", val); render_shader_init(&isst->camera.render, "myplugin", buf); break;
			case ']': val += 0.1; snprintf(buf, BUFSIZ, "%f", val); render_shader_init(&isst->camera.render, "myplugin", buf); break;
			case SDLK_DELETE:
			case '-':
				  {
				      printf("\nReloading plugin\n");
				      if(render_shader_unload_plugin(&isst->camera.render, "myplugin")) {
					  printf("Failed unloading plugin");
					  exit(-1);
				      }
				      snprintf(buf, BUFSIZ, "%f", val); 
				      render_shader_init(&isst->camera.render, render_shader_load_plugin(".libs/libmyplugin.0.dylib"), buf);
				  }
				  break;
			case SDLK_UP:
			case 'e': vel[1] = 1; break;
			case SDLK_DOWN:
			case 'd': vel[1] = -1; break;
			case SDLK_RIGHT:
			case 'r': vel[0] = 1; break;
			case SDLK_LEFT:
			case 'w': vel[0] = -1; break;
			case ' ': vel[2] = 1; break;
			case 'v': vel[2] = -1; break;
			case '0': zero_view(isst); break;
			case 'z': isst->gs = !isst->gs; resize_isst(isst); break;
				  /* TODO: more keys for nifty things like changing mode or pulling up gui bits or something */
		    }
		    break;
		case SDL_KEYUP:
		    switch (tolower (e.key.keysym.sym))
		    {
			case SDLK_UP:
			case SDLK_DOWN: 
			case 'e':
			case 'd': vel[1] = 0; break;
			case SDLK_RIGHT:
			case SDLK_LEFT:
			case 'r':
			case 'w': vel[0] = 0; break;
			case ' ':
			case 'v': vel[2] = 0; break;
		    }
		    break;
		case SDL_MOUSEMOTION:
		    switch(e.motion.state) {
			case 1:
			    break;
			case 4:
			    look(isst, isst->w / isst->camera.w * mouse_sensitivity * dt * e.motion.xrel, isst->h / isst->camera.h * mouse_sensitivity * dt * e.motion.yrel);
			    break;
		    }
		    break;

	    }
	if(vel[0] != 0) move_strafe(isst, dt*(double)vel[0]);
	if(vel[1] != 0) move_walk(isst, dt*(double)vel[1]);
	if(vel[2] != 0) move_float(isst, dt*(double)vel[2]);
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
