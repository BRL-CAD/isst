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

#include <SDL.h>

int main(int argc, char **argv)
{
    SDL_Surface *screen;
    SDL_Event e;

    SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER);
    atexit (SDL_Quit);

    /* can we make this resizable? */
    screen = SDL_SetVideoMode (800, 600, 24, SDL_DOUBLEBUF);

    /* TODO: some stuff to load up geometry and set up the isst buffers */

    /* main event loop */
    while (1)
    {   
	/* TODO: ask libtie/librender to fill the isst buffer */
	/* TODO: copy/blit the isst buffer into the sdl screen */
	SDL_UpdateRect(screen, 0, 0, 0, 0);
	SDL_WaitEvent (&e);
	switch (e.type)
	{
	    case SDL_KEYDOWN:
		switch (tolower (e.key.keysym.sym))
		{
		    case 'x':
		    case 'q':
		    case SDLK_ESCAPE:
			SDL_Quit ();
			return EXIT_SUCCESS;
			break;
			/* TODO: more keys for nifty things like changing mode or pulling up gui bits or something */
		}
		/* TODO: look for mouse events */
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
