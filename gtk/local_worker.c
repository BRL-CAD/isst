/*                   L O C A L _ W O R K E R . C
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

/** @file local_worker.c
 *
 * Brief description
 *
 */

#ifdef HAVE_CONFIG_H
# include "isst_config.h"
#endif

#include <pthread.h>

#include <stdint.h>

#include "tie/tie.h"
#include "tie/adrt.h"
#include "tie/adrt_struct.h"
#include "tie/camera.h"

#include "isst.h"

struct tie_s *tie;

void
isst_local_work_frame() {
    isst.update_avail = 1;
    return;
}

gpointer
isst_local_worker (gpointer moocow) {
    int oldmode = -1, meh = 0, origh = -1, origw = -1;
    double mehd = 0;
    struct render_camera_s camera;
    struct camera_tile_s tile;
    struct timeval ts[2];

    render_camera_init(&camera, bu_avail_cpus());

    while(1) {

	/* spinlock. Fix this. */
	if(!isst.update_avail) {
	    usleep(100);
	    continue;
	}

	/* resize event, reset rendering info */
	if(origw != isst.context_width || origh != isst.context_height) {
	    camera.w = isst.context_width;
	    camera.h = isst.context_height;
	    tile.orig_x = 0;
	    tile.orig_y = 0;
	    origw = tile.size_x = isst.context_width;
	    origh = tile.size_y = isst.context_height;
	    tile.format = RENDER_CAMERA_BIT_DEPTH_24;
	}

	isst.update_avail = 0;
	isst.update_idle = 0;

	if( oldmode != isst.mode ) {
	    char buf[BUFSIZ];
	    switch(isst.mode) {
		case ISST_MODE_SHADED:
		case ISST_MODE_SHOTLINE:
		    render_shader_init(&camera.render, "phong", NULL);
		    break;
		case ISST_MODE_NORMAL:
		    render_shader_init(&camera.render, "normal", NULL);
		    break;
		case ISST_MODE_DEPTH:
		    render_shader_init(&camera.render, "depth", NULL);
		    break;
		case ISST_MODE_COMPONENT:
		    render_shader_init(&camera.render, "component", NULL);
		    break;
		case ISST_MODE_CUT:
		    VMOVE(isst.shotline_pos.v, isst.camera_pos.v);
		    VSUB2(isst.shotline_dir.v, isst.camera_foc.v, isst.camera_pos.v);
		    snprintf(buf, BUFSIZ, "#(%f %f %f) #(%f %f %f)", V3ARGS(isst.shotline_pos.v), V3ARGS(isst.shotline_dir.v));
		    render_shader_init(&camera.render, "cut", buf);
		    break;
		case ISST_MODE_FLOS:
		    snprintf(buf, BUFSIZ, "#(%f %f %f)", V3ARGS(isst.shotline_pos.v));
		    render_shader_init(&camera.render, "flos", buf);
		    break;
		default:
		    bu_log("Bad mode: %d\n", isst.mode);
		    bu_bomb("Kapow\n");
	    }
	    oldmode = isst.mode;
	}

	camera.type = isst.camera_type;
	camera.fov  = isst.camera_fov;
	camera.pos  = isst.camera_pos;
	camera.focus= isst.camera_foc;
	camera.gridsize = isst.camera_grid;

	isst.buffer_image.ind = 0;

	render_camera_prep (&camera);

	/* pump tie_work and display the result. */
	gettimeofday(ts, NULL);
	render_camera_render(&camera, tie, &tile, &isst.buffer_image);
#if 0
	gettimeofday(ts+1, NULL);
	++meh;
	mehd += (((double)ts[1].tv_sec+(double)ts[1].tv_usec/(double)1e6) - ((double)ts[0].tv_sec+(double)ts[0].tv_usec/(double)1e6));

	if(mehd>1.0/3.0) {
	    printf("   \r%.2lf FPS", (double)meh / mehd); fflush(stdout);
	    meh = 0;
	    mehd = 0;
	}
#endif

	/* shove results into the gdk canvas */
	paint_context();
	isst.update_idle = 1;
    }
    return;
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
