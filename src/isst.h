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

/** @file isst.c
 *
 * Brief description
 *
 */


#ifndef _ISST_H
#define _ISST_H

#define ISST_USE_COMPRESSION	1

/*
#define ISST_WINDOW_W		1024
#define ISST_WINDOW_H		768
#define ISST_CONTEXT_W		640
#define	ISST_CONTEXT_H		480
*/
#define ISST_WINDOW_W		(1024+1024-768)
#define ISST_WINDOW_H		(768+768-480)
#define ISST_CONTEXT_W		1024
#define	ISST_CONTEXT_H		768

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
  MYSQL mysql_db;
  int socket;
  uint16_t endian;
  uint8_t mode;
  uint8_t mode_updated;

  /* geometry */
  TIE_3 geom_min;
  TIE_3 geom_max;
  TIE_3 geom_center;
  tfloat geom_radius;

  /* camera */
  TIE_3 camera_pos;
  TIE_3 camera_foc;
  tfloat camera_az;
  tfloat camera_el;
  tfloat camera_fov;
  tfloat camera_grid;
  uint8_t camera_type;

  /* shotline */
  TIE_3 shotline_pos;
  TIE_3 shotline_dir;

  /* buffers */
  tienet_buffer_t buffer;
  tienet_buffer_t buffer_comp;
  tienet_buffer_t buffer_image;

  /* input */
  int16_t mouse_x;
  int16_t mouse_y;
  tfloat mouse_speed;

  /* miscellaneous */
  uint8_t update_avail;
  uint8_t update_idle;
  pthread_mutex_t update_mut;

  uint8_t notebook_index[ISST_MODES];

  void (*work_frame)(void);
  gpointer (*worker)(gpointer trash);
} isst_t;

extern uint8_t isst_flags;
extern isst_t isst;


void isst_init (void);
void isst_azel_to_foc ();
void isst_free();
void isst_azel_to_foc();

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
