/*                    N E T _ W O R K E R . C
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

/** @file net_worker.c
 *
 * Brief description
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <gtk/gtk.h>

#undef CLAMP

#include "tie/tie.h"
#include "tie/camera.h"
#include "tie/adrt.h"
#include "tie/adrt_struct.h"
#include "isst.h"

extern GtkWidget *isst_window;
extern GtkWidget *isst_container;
extern GtkWidget *isst_context;

/* Navigation */
extern GtkWidget *isst_grid_spin;
extern GtkWidget *isst_posx_spin;
extern GtkWidget *isst_posy_spin;
extern GtkWidget *isst_posz_spin;
extern GtkWidget *isst_azim_spin;
extern GtkWidget *isst_elev_spin;
extern GtkWidget *isst_mouse_speed_spin;

/* Shotline */
extern GtkWidget *isst_cellx_spin;
extern GtkWidget *isst_celly_spin;
extern GtkWidget *isst_deltax_spin;
extern GtkWidget *isst_deltay_spin;
extern GtkWidget *isst_name_entry;
extern GtkWidget *isst_inhit_entry;

/* Fragment Line of Sight */
extern GtkWidget *isst_flos_posx_spin;
extern GtkWidget *isst_flos_posy_spin;
extern GtkWidget *isst_flos_posz_spin;

/* Shotline Table */
extern GtkListStore *isst_shotline_store;

/* send out a work request to the network */
void
isst_net_work_frame()
{
    uint32_t size;
    uint8_t ind;
    uint8_t message[256];

    ind = 0;

    /* Send request for next frame */
    message[ind++] = ADRT_NETOP_WORK;

    /* size of message being sent */
    ind += 4;

    /* slave function */
    message[ind++] = ADRT_WORK_FRAME;

    /* workspace id */
    TCOPY(uint16_t, &isst.wid, 0, message, ind);
    ind += 2;

    /* camera type */
    message[ind++] = isst.camera_type;

    /* field of view */ 
    if (isst.camera_type == RENDER_CAMERA_PERSPECTIVE) {
	TCOPY(tfloat, &isst.camera_fov, 0, message, ind);
	ind += sizeof (tfloat);
    } else {
	TCOPY(tfloat, &isst.camera_grid, 0, message, ind);
	ind += sizeof (tfloat);
    }

    /* send camera position and focus */
    TCOPY(TIE_3, &isst.camera_pos, 0, message, ind);
    ind += sizeof (TIE_3);
    TCOPY(TIE_3, &isst.camera_foc, 0, message, ind);
    ind += sizeof (TIE_3);

    message[ind] = 0;
    if (isst.mode_updated) {
	isst.mode_updated = 0;
	ADRT_MESSAGE_MODE_CHANGE(message[ind]);
    }

#define OP(x) message[ind++] += x;
    switch(isst.mode) {
	case ISST_MODE_COMPONENT:
	    OP(RENDER_METHOD_COMPONENT);
	    break;
	case ISST_MODE_NORMAL:	/* surface normal rendering */
	    OP(RENDER_METHOD_NORMAL);
	    break;
	case ISST_MODE_DEPTH:	/* surface normal rendering */
	    OP(RENDER_METHOD_DEPTH);
	    break;
	case ISST_MODE_SHADED:
	case ISST_MODE_SHOTLINE:
	    OP(RENDER_METHOD_PHONG);
	    break;
	case ISST_MODE_CUT:
	    OP(RENDER_METHOD_CUT);

	    VMOVE(isst.shotline_pos.v, isst.camera_pos.v);
	    VSUB2(isst.shotline_dir.v, isst.camera_pos.v, isst.camera_foc.v);

	    /* position and direction */
	    TCOPY(TIE_3, &isst.shotline_pos, 0, message, ind); ind += sizeof (TIE_3);
	    TCOPY(TIE_3, &isst.shotline_dir, 0, message, ind); ind += sizeof (TIE_3);
	    break;

	case ISST_MODE_FLOS:
	    {
		TIE_3 frag_pos;

		OP(RENDER_METHOD_FLOS);

		/* position */
		VSET(frag_pos.v,  gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_flos_posx_spin)),  gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_flos_posy_spin)),  gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_flos_posz_spin)));

		TCOPY(TIE_3, &frag_pos, 0, message, ind);
		ind += sizeof (TIE_3);
	    }
	    break;
    }
#undef OP

    /* size of message being sent */
    size = ind - 4 - 1;
    TCOPY(uint32_t, &size, 0, message, 1);

    tienet_send (isst.socket, message, ind);
}

gpointer
isst_net_worker (gpointer moocow)
{
    struct sockaddr_in my_addr, srv_addr;
    struct hostent srv_hostent;
    tienet_buffer_t *buffer;
    unsigned int addrlen;
    uint8_t op;


    /* create a socket */
    if ((isst.socket = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	generic_dialog ("Unable to create network socket for master.");
	isst.connected = 0;
	return (NULL);
    }

    /* client address */
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons (0);

    if (gethostbyname (bu_vls_addr(&isst.master)))
	srv_hostent = gethostbyname (bu_vls_addr(&isst.master))[0];
    else
    {
	generic_dialog ("Hostname for master doesn't resolve.");
	isst.connected = 0;
	return (NULL);
    }

    srv_addr.sin_family = srv_hostent.h_addrtype;
    memcpy(srv_hostent.h_addr_list[0], (char *) &srv_addr.sin_addr.s_addr, srv_hostent.h_length);
    srv_addr.sin_port = htons (ADRT_PORT);

    if (bind (isst.socket, (struct sockaddr *)&my_addr, sizeof (my_addr)) < 0)
    {
	generic_dialog ("Unable to bind network socket for master.");
	isst.connected = 0;
	return (NULL);
    }

    /* connect to master */
    if (connect (isst.socket, (struct sockaddr *)&srv_addr, sizeof (srv_addr)) < 0)
    {
	generic_dialog ("Connection to master failed.\n1. Check that the master hostname is correct.\n2. Check the master is running on hostname specified.");
	isst.connected = 0;
	return ("NULL");
    }

    /* Now that a connection is established, show the fixed container */
    GTK_WIDGET_UNSET_FLAGS (isst_container, GTK_NO_SHOW_ALL);
    gtk_widget_show_all (isst_window);

    addrlen = sizeof (srv_addr);

    /* send version and get endian info */
    op = ADRT_NETOP_INIT;
    tienet_send (isst.socket, &op, 1);

    /* Get back an endian value from the master to determine endian data will need to be in. */
    tienet_recv (isst.socket, &isst.wid, sizeof (isst.wid));
    isst.endian = isst.endian == 1 ? 0 : 1;


    while (1)
    {
	static short int wid = 0;

	/* get op */
	tienet_recv (isst.socket, &op, 1);
	buffer = (op == ADRT_WORK_FRAME) ? &isst.buffer_image : &isst.buffer;

	/* get workspace id - XXX Why is there no WID coming back? */
	tienet_recv (isst.socket, &wid, 2);

	pthread_mutex_lock (&isst.update_mut);
	isst.update_idle = 0;

	/*
	 * Send the next batch of work out to the master so it's busy
	 * while data is being processed.
	 */

	if (isst.update_avail)
	{
	    isst.work_frame ();
	    isst.update_avail = 0;
	}
	else 
	    isst.update_idle = 1;
	pthread_mutex_unlock (&isst.update_mut);

	tienet_recv (isst.socket, &buffer->ind, 4);
	TIENET_BUFFER_SIZE((*buffer), buffer->ind);

#if ISST_USE_COMPRESSION
	{
	    unsigned long dest_len;

	    TIENET_BUFFER_SIZE(isst.buffer_comp, buffer->ind + 32);
	    tienet_recv (isst.socket, &isst.buffer_comp.ind, sizeof (unsigned int));
	    tienet_recv (isst.socket, isst.buffer_comp.data, isst.buffer_comp.ind);

	    dest_len = buffer->ind + 32;
	    uncompress (buffer->data, &dest_len, isst.buffer_comp.data, (unsigned long)isst.buffer_comp.ind);
	}
#else
	tienet_recv (isst.socket, buffer->data, buffer->ind);
#endif

	switch(op)
	{
	    case ADRT_WORK_FRAME:
		{
		    gdk_threads_enter ();
		    gdk_draw_rgb_image (isst_context->window, isst_context->style->fg_gc[GTK_STATE_NORMAL],
			    0, 0, ISST_CONTEXT_W, ISST_CONTEXT_H, GDK_RGB_DITHER_NONE,
			    isst.buffer_image.data, ISST_CONTEXT_W * 3);
		    gdk_threads_leave ();

		    /* Draw Cross Hairs since they were just overwritten */
		    if (isst.mode == ISST_MODE_SHOTLINE)
			draw_cross_hairs ((int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_cellx_spin)),
				(int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_celly_spin)));
		}
		break;

	    case ADRT_WORK_MINMAX:
		{
		    uint32_t ind;
		    TIE_3 max;

		    ind = 0;

		    TCOPY(TIE_3, buffer->data, ind, &isst.geom_min, 0);
		    ind += sizeof (TIE_3);

		    TCOPY(TIE_3, buffer->data, ind, &isst.geom_max, 0);

		    VADD2(isst.geom_center.v,  isst.geom_min.v,  isst.geom_max.v);
		    VSCALE(isst.geom_center.v,  isst.geom_center.v,  0.5);

		    max.v[0] = fabs (isst.geom_min.v[0]) > fabs (isst.geom_max.v[0]) ? fabs (isst.geom_min.v[0]) : fabs (isst.geom_min.v[0]);
		    max.v[1] = fabs (isst.geom_min.v[1]) > fabs (isst.geom_max.v[1]) ? fabs (isst.geom_min.v[1]) : fabs (isst.geom_min.v[1]);
		    max.v[2] = fabs (isst.geom_min.v[2]) > fabs (isst.geom_max.v[2]) ? fabs (isst.geom_min.v[2]) : fabs (isst.geom_min.v[2]);

		    isst.geom_radius = sqrt (max.v[0]*max.v[0] + max.v[1]*max.v[1] + max.v[2]*max.v[2]);
		}
		break;

	    case ADRT_WORK_SHOTLINE:
		{
		    TIE_3 inhit;
		    uint32_t ind, num, i;
		    char text[256], thickness[16];
		    uint8_t c;
		    GtkTreeIter iter;
		    tfloat t;

		    ind = 0;
		    /* In-Hit */
		    TCOPY(TIE_3, buffer->data, ind, &inhit, 0);
		    ind += sizeof (TIE_3);
		    sprintf (text, "%.3f %.3f %.3f", inhit.v[0], inhit.v[1], inhit.v[2]);
		    gtk_entry_set_text (GTK_ENTRY (isst_inhit_entry), text);

		    TCOPY(uint32_t, buffer->data, ind, &num, 0);
		    ind += 4;

		    /* Update component table */
		    gtk_list_store_clear (isst_shotline_store);
		    for (i = 0; i < num; i++)
		    {
			TCOPY(uint8_t, buffer->data, ind, &c, 0);
			ind += 1;

			bcopy(&buffer->data[ind], text, c);
			ind += c;

			TCOPY(tfloat, buffer->data, ind, &t, 0);
			t *= 1000; /* Convert to milimeters */
			ind += sizeof (tfloat);
			sprintf (thickness, "%.1f", t);

			gtk_list_store_append (isst_shotline_store, &iter);
			gtk_list_store_set (isst_shotline_store, &iter, 0, i+1, 1, text, 2, thickness, -1);
		    }

		    /* Shotline position and direction */
		    TCOPY(TIE_3, buffer->data, ind, &isst.shotline_pos, 0);
		    ind += sizeof (TIE_3);
		    TCOPY(TIE_3, buffer->data, ind, &isst.shotline_dir, 0);
		    ind += sizeof (TIE_3);
		}
		break;

	    default:
		break;
	}
    }

    return NULL;
}

void load_frame_attribute()
{
    /* frame attributes (size and format) */
    uint32_t size;
    uint16_t w, h, format;
    uint8_t ind, message[256], op;

    ind = 0;

    w = ISST_CONTEXT_W;
    h = ISST_CONTEXT_H;
    format = RENDER_CAMERA_BIT_DEPTH_24;

    op = ADRT_NETOP_WORK;
    TCOPY(uint8_t, &op, 0, message, ind);
    ind++;

    size = 9; /* wid + op + w + h + format */
    TCOPY(uint32_t, &size, 0, message, ind);
    ind += 4;

    op = ADRT_WORK_FRAME_ATTR;
    TCOPY(uint8_t, &op, 0, message, ind);
    ind++;

    TCOPY(uint16_t, &isst.wid, 0, message, ind);
    ind += 2;

    TCOPY(uint16_t, &w, 0, message, ind);
    ind += 2;
    TCOPY(uint16_t, &h, 0, message, ind);
    ind += 2;
    TCOPY(uint16_t, &format, 0, message, ind);
    ind += 2;

    tienet_send (isst.socket, message, ind);

    /* Request geometry min and max */
    op = ADRT_NETOP_WORK;
    tienet_send (isst.socket, &op, 1);

    /* size */
    size = 3;
    tienet_send (isst.socket, &size, 4);

    op = ADRT_WORK_MINMAX;
    tienet_send (isst.socket, &op, 1);

    /* workspace id */
    tienet_send (isst.socket, &isst.wid, 2);
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
