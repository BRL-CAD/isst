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

#if _WIN32
# include "config_win.h"
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

    isst->dirty = 1;
    VSUB2(vec, isst->camera.focus, isst->camera.pos);
    VUNITIZE(vec);
    VSCALE(vec, vec, isst->dt * dist * isst->tie->radius);
    VADD2(isst->camera.pos, isst->camera.pos, vec);
    if(dist < 0) VSCALE(vec, vec, -1);
    VADD2(isst->camera.focus, isst->camera.pos, vec);
}

void 
move_strafe(struct isst_s * isst, double dist)
{
    vect_t vec, dir, up;

    isst->dirty = 1;
    VSET(up, 0, 0, 1);
    VSUB2(dir, isst->camera.focus, isst->camera.pos);
    VCROSS(vec, dir, up);
    VUNITIZE(vec);
    VSCALE(vec, vec, isst->dt * dist * isst->tie->radius);
    VADD2(isst->camera.pos, isst->camera.pos, vec);
    VADD2(isst->camera.focus, isst->camera.pos, dir);
}

void 
move_float(struct isst_s * isst, double dist)
{
    isst->dirty = 1;
    isst->camera.pos[2] += isst->tie->radius*isst->dt*dist;
    isst->camera.focus[2] += isst->tie->radius*isst->dt*dist;
}

void
zero_view(struct isst_s *isst) 
{
    vect_t vec;

    isst->dirty = 1;
    VSUB2(vec, isst->tie->mid, isst->camera.pos);
    VUNITIZE(vec);
    VADD2(isst->camera.focus, isst->camera.pos, vec);
}

void 
look(struct isst_s * isst, double x, double y)
{
    double az, el;
    vect_t vec;

    isst->dirty = 1;
    /* generate az/el (oddly, this generates degrees instead of radians) */
    VSUB2(vec, isst->camera.pos, isst->camera.focus);
    VUNITIZE(vec);
    AZEL_FROM_V3DIR(az, el, vec);
    az = az * -DEG2RAD - x;
    el = el * -DEG2RAD - y;

    /* clamp to sane values */
    while(az > 2*M_PI) az -= 2*M_PI;
    while(az < 0) az += 2*M_PI;
    if(el>M_PI_2) el=M_PI_2 - 0.001;
    if(el<-M_PI_2) el=-M_PI_2 + 0.001;

    /* generate the new lookat point */
    V3DIR_FROM_AZEL(vec, az, el);
    VADD2(isst->camera.focus, isst->camera.pos, vec);
}

void
shader(struct isst_s *isst, const char *mode, const char *buf)
{
    isst->dirty = 1;
    render_shader_init(&isst->camera.render, mode, buf);
}

int
do_loop(struct isst_s *isst)
{
    SDL_Event e;
    double ts[2];
    int fc = 0, showfps = 1;
    double dt2 = 0.0;
    double mouse_sensitivity = 0.002;
    double val = 1;
    int vel[3] = { 0, 0, 0 }, sc=0;
    char buf[BUFSIZ], cmdbuf[BUFSIZ], *cmd;
    vect_t vec;
    struct adrt_mesh_s *mesh;

    isst->dt = 1;
    isst->fps = 1;
    isst->dirty = 1;
    ts[0] = SDL_GetTicks();
    isst->cmdbuf = cmdbuf;
    memset(cmdbuf, 0, BUFSIZ);
    memset(buf, 0, BUFSIZ);

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
	isst->dt = 0.001 * (ts[1] - ts[0]);

#define MAXFPS 30.0
	if(isst->dt < 1.0/MAXFPS) {
	    SDL_Delay(1000 * 1.0/MAXFPS - isst->dt);
	    isst->dt += 1.0/MAXFPS - isst->dt;
	}
#undef MAXFPS

	dt2 += isst->dt;
	ts[0] = SDL_GetTicks();

	if(dt2 > 0.5) {
	    isst->fps = (double)fc/dt2;
	    fc=0;
	    dt2 = 0;
	}
	if(showfps && isst->ui == 0) {
	    printf("  \r%g FPS", isst->fps);
	    fflush(stdout);
	}

	while(SDL_PollEvent (&e)) {
	    if(isst->ui == 0) {
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
			    case SDLK_F11:
				if(isst->sflags&SDL_FULLSCREEN) {
				    isst->sflags &= ~SDL_FULLSCREEN;
				    SDL_WM_GrabInput(SDL_GRAB_OFF);
				} else {
				    isst->sflags |= SDL_FULLSCREEN;
				    SDL_WM_GrabInput(SDL_GRAB_ON);
				}
				resize_isst(isst);
				break;
			    case SDLK_F12:
				SDL_Quit ();
				printf("\n");
				return EXIT_SUCCESS;
				break;
			    case '1': shader(isst, "phong", NULL); break;
			    case '2': shader(isst, "normal", NULL); break;
			    case '3': shader(isst, "depth", NULL); break;
			    case '4': shader(isst, "component", NULL); break;
			    case '5': 
				      /* clear all the hit list */
				      for(BU_LIST_FOR(mesh, adrt_mesh_s, &isst->meshes->l))
					  mesh->flags &= ~ADRT_MESH_HIT;
				      VSUB2(vec, isst->camera.focus, isst->camera.pos);
#if _WIN32
				      sprintf(buf, "#(%f %f %f)  #(%f %f %f)", V3ARGS(isst->camera.pos), V3ARGS(vec));
#else
				      snprintf(buf, BUFSIZ, "#(%f %f %f)  #(%f %f %f)", V3ARGS(isst->camera.pos), V3ARGS(vec));
#endif
				      shader(isst, "cut", buf); 
				      move_strafe(isst, -0.05 / isst->dt);
				      break;
			    case SDLK_ESCAPE:
			    case SDLK_RETURN: VSETALL(vel, 0); cmd = cmdbuf; isst->ui = !isst->ui; printf("\n"); break;
			    case SDLK_DELETE:
			    case '=': 
#if _WIN32
					      sprintf(buf, "%f", val); 
#else
					      snprintf(buf, BUFSIZ, "%f", val);
#endif
					      shader(isst, "myplugin", buf); break;
			    case '[': sc = -1; break;
			    case ']': sc = 1; break;
			    case '-':
				      /* this stuff needs a lot of fixing */
				      printf("\nReloading plugin\n");
				      if(render_shader_unload_plugin(&isst->camera.render, "myplugin")) {
					  printf("Failed unloading plugin");
					  exit(-1);
				      }
#if _WIN32
				      sprintf(buf, "%f", val); 
#else
				      snprintf(buf, BUFSIZ, "%f", val); 
#endif
				      shader(isst, render_shader_load_plugin("libmyplugin.so"), buf);
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
			    case 'f': vel[2] = -1; break;
			    case '0': zero_view(isst); break;
			    case 'z': isst->gs++; if(isst->gs >= 3) isst->gs = 0; resize_isst(isst); break;
			    case 'Z': isst->gs--; if(isst->gs < 0) isst->gs = 2; resize_isst(isst); break;
			    case 'o':
				      isst->camera.type = isst->camera.type == RENDER_CAMERA_ORTHOGRAPHIC ? RENDER_CAMERA_PERSPECTIVE : RENDER_CAMERA_ORTHOGRAPHIC;
				      isst->dirty = 1;
				      break;
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
			    case 'f': vel[2] = 0; break;
			    case '[':
			    case ']': sc = 0; break;
			}
			break;
		    case SDL_MOUSEMOTION:
			switch(e.motion.state) {
			    case 1:
				break;
			    case SDL_BUTTON(SDL_BUTTON_RIGHT):
				look(isst, mouse_sensitivity * e.motion.xrel, mouse_sensitivity * e.motion.yrel);
				isst->dirty = 1;
				break;
			}
			break;

		}
	    } else {	/* control panel mode */
		switch(e.type) {
		    case SDL_KEYDOWN:
			switch(e.key.keysym.sym) {
			    case SDLK_F12:
				SDL_Quit();
				printf("\n");
				return EXIT_SUCCESS;
			    case SDLK_ESCAPE:
				isst->ui = 0;
				break;
			    case SDLK_RETURN:
				*cmd = 0;
				printf("\nExecute command: \"%s\"\n", cmdbuf);
				isst->ui = 0;
				*cmdbuf = 0;
				break;
			    default:
				*cmd++ = e.key.keysym.sym;
				*cmd = 0;
				fflush(stdout);
			}
		}
	    }
	}
	if(vel[0] != 0) move_strafe(isst, (double)vel[0]);
	if(vel[1] != 0) move_walk(isst, (double)vel[1]);
	if(vel[2] != 0) move_float(isst, (double)vel[2]);
	if(sc != 0) { val += isst->dt * sc; 
#ifdef _WIN32
		sprintf(buf, "%f", val); 
#else
		snprintf(buf, BUFSIZ, "%f", val); 
#endif
		shader(isst, "myplugin", buf); }
	if(isst->ui && isst->uic < 0.99999) { isst->uic += 2*isst->dt; if(isst->uic > 1.0) isst->uic = 1; }
	if(!isst->ui && isst->uic > 0.00001) { isst->uic -= 2*isst->dt; if(isst->uic < 0.0) isst->uic = 0; }
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
