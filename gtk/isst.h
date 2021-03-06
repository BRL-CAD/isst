/*                          I S S T . C
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

/** @file isst.h
 *
 * Brief description
 *
 */


#ifndef _ISST_H
#define _ISST_H

#define ISST_USE_COMPRESSION	1

#define ISST_CONTEXT_W		640
#define	ISST_CONTEXT_H		(ISST_CONTEXT_W*3/4)
#define ISST_WINDOW_W		(ISST_CONTEXT_W+256)
#define ISST_WINDOW_H		(ISST_CONTEXT_H+128)

#define ISST_VER_DETAIL		"ISST - U.S. Army Research Lab (2005-2009)"

#include <glib.h>

enum
{
    ISST_MODE_SHADED,
    ISST_MODE_COMPONENT,
    ISST_MODE_NORMAL,
    ISST_MODE_DEPTH,
    ISST_MODE_SHOTLINE,
    ISST_MODE_CUT,
    ISST_MODE_FLOS,
    ISST_MODE_BAD,
    ISST_MODES
};

typedef struct isst_s
{
    /* general */
    char username[32];
    char password[32];
    struct bu_vls database;
    struct bu_vls master;
    int32_t uid;
    int32_t pid;
    uint16_t wid;
    uint8_t connected;
    int socket;
    uint16_t endian;
    uint8_t mode;
    uint8_t mode_updated;
    int context_width, context_height;

    /* geometry */
    point_t geom_min;
    point_t geom_max;
    point_t geom_center;
    fastf_t geom_radius;

    /* camera */
    point_t camera_pos;
    point_t camera_foc;
    fastf_t camera_az;
    fastf_t camera_el;
    fastf_t camera_fov;
    fastf_t camera_grid;
    uint8_t camera_type;

    /* shotline */
    point_t shotline_pos;
    vect_t shotline_dir;

    /* buffers */
    tienet_buffer_t buffer;
    tienet_buffer_t buffer_comp;
    tienet_buffer_t buffer_image;

    /* input */
    int16_t mouse_x;
    int16_t mouse_y;
    fastf_t mouse_speed;

    /* miscellaneous */
    uint8_t update_avail;
    uint8_t update_idle;
    pthread_mutex_t update_mut;

    uint8_t notebook_index[ISST_MODES];

    void (*work_frame)(void);

    struct adrt_mesh_s *meshes;	/* a bu_list of meshes */
} isst_t;

extern uint8_t isst_flags;
extern isst_t isst;
extern struct tie_s *tie;

void isst_init (int argc, char **argv);
void isst_azel_to_foc ();
void isst_free();
void isst_azel_to_foc();

void isst_net_work_frame();
gpointer isst_net_worker (gpointer moocow);

void isst_local_work_frame();
gpointer isst_local_worker (gpointer moocow);

void draw_cross_hairs (int16_t x, int16_t y);
void generic_dialog (char *message);

void paint_context ();

#endif

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
