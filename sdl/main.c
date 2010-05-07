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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <SDL.h>
#ifdef HAVE_OPENGL
# include <SDL_opengl.h>
#endif

#include <tie.h>
#include <adrt.h>
#include <adrt_struct.h>
#include <camera.h>

#include "isst.h"

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
#ifdef HAVE_OPENGL
    if(isst->sflags & SDL_OPENGL) {
	glClearColor (0.0, 0, 0.0, 1);
	glBindTexture (GL_TEXTURE_2D, isst->texid);
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	isst->texdata = malloc(isst->r.w * isst->r.h * 3);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, isst->r.w, isst->r.h, 0, GL_RGB, GL_UNSIGNED_BYTE, isst->texdata);
	glDisable(GL_LIGHTING);
	glViewport(0,0,isst->r.w, isst->r.h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glOrtho(0, isst->r.w, isst->r.h, 0, -1, 1);
	glMatrixMode (GL_MODELVIEW);
    }
#endif
}

struct isst_s *
prep_isst(int argc, const char **argv)
{
    struct isst_s *isst;
    isst = (struct isst_s *)malloc(sizeof(struct isst_s));
    isst->tie = (struct tie_s *)bu_calloc(1,sizeof(struct tie_s), "tie");
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
#ifdef HAVE_OPENGL
    render_camera_prep(&isst->camera);
    render_camera_render(&isst->camera, isst->tie, &isst->tile, &isst->buffer_image);
    glClear(GL_DEPTH_BUFFER_BIT/*|GL_COLOR_BUFFER_BIT*/);
    glLoadIdentity();
    glColor3f(1,1,1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, isst->texid);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, isst->r.w, isst->r.h, GL_RGB, GL_UNSIGNED_BYTE, isst->buffer_image.data);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2d(0,0); glVertex3f(0,0,0);
    glTexCoord2d(0,1); glVertex3f(0,isst->r.h,0);
    glTexCoord2d(1,0); glVertex3f(isst->r.w,0,0);
    glTexCoord2d(1,1); glVertex3f(isst->r.w,isst->r.h,0);
    glEnd();
    SDL_GL_SwapBuffers();
#endif
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
    isst->sflags = sflags;
    isst->ogl = ogl;
    isst->w = w;
    isst->h = h;
    if(render_shader_load_plugin(".libs/libmyplugin.0.dylib")) {
	    printf("Failed loading plugin");
    }
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
