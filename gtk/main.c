/*                          M A I N . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2009 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
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
 * Brief description
 *
 */

#ifdef HAVE_CONFIG_H
# include "isst_config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <pthread.h>

#include "tie.h"
#include "tie/adrt.h"

#include <gtk/gtk.h>

#include "isst.h"

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#ifdef HAVE_GETOPT_LONG
static struct option longopts[] =
{
    { "help",	no_argument,		NULL, 'h' },
    { "build",	no_argument,		NULL, 'b' },
    { "pluh",	required_argument,	NULL, 'p' },
};
#endif
static char shortopts[] = "hbp:";


static void finish(int sig) {
    printf("Collected signal %d, aborting!\n", sig);
    exit(EXIT_FAILURE);
}


static void help() {
    printf("%s\n", ISST_VER_DETAIL);
    printf("%s\n", "usage: isst [options]\n\
	    -v\t\tdisplay build\n\
	    -h\t\tdisplay help\n");
}


int main(int argc, char **argv) {
    char c = 0;

    signal(SIGINT, finish);

    /* Parse command line options */
    while((c = 
#ifdef HAVE_GETOPT_LONG
		getopt_long(argc, argv, shortopts, longopts, NULL)
#else
		getopt(argc, argv, shortopts)
#endif
	  )!= -1)
    {
	switch(c) {
	    case 'h':
		help();
		return EXIT_SUCCESS;
	    case 'b':
		printf("DIVA %s \n", __DATE__);
		return EXIT_SUCCESS;
	    case 'p':	/* ignored, passed in on mac from launchd/open */
		break;
	    default:
		help();
		return EXIT_FAILURE;
	}
    }

    argc -= optind;
    argv += optind;

    bu_vls_init(&isst.database);
    bu_vls_init(&isst.master);
    if(getenv("ADRT_DB"))
	bu_vls_strcpy(&isst.database, getenv("ADRT_DB"));
    else if(getenv("ADRT_DATABASE"))
	bu_vls_strcpy(&isst.database, getenv("ADRT_DATABASE"));
    if(getenv("ADRT_MASTER"))
	bu_vls_strcpy(&isst.master, getenv("ADRT_MASTER"));

    isst_init (argc, argv);

    return EXIT_SUCCESS;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
