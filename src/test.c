/*                          T E S T . C
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

/** @file test.c
 *
 * Brief description
 *
 */

#include <stdio.h>
#include "tie/adrt.h"

int isst_gui() { return 0; }
int isst_setup() { return 0; }

int main(int argc, char **argv)
{
	int i;

	if(argc!=2)
		return 1;
	i = atoi (argv[1]);

	printf("%d %s\n", i, ADRT_MESSAGE_NAME(i));
	
	return 0;
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
