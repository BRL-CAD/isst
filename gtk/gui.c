/*                          G U I . C
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

/** @file gui.c
 *
 * Brief description
 *
 */

#ifdef HAVE_CONFIG_H
# include "isst_config.h"
#endif

#include <gtk/gtk.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef CLAMP

/* Networking Includes */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pango/pango.h>
#if ISST_USE_COMPRESSION
#include <zlib.h>
#endif

#include <stdint.h>
#include "tie.h"
#include "tie/camera.h"

#include "tie/adrt.h"
#include "tie/adrt_struct.h"

#include "isst.h"

#include "icv.h"	/* for image saving */


#ifndef HAVE_STRNSTR
char *strnstr(const char *s1, const char *s2, size_t n) { return strstr(s1, s2); }
#endif


#define TABLE_BORDER_WIDTH	0

static const char shotfilefmt[] = "S %s POS<%g %g %g> AZEL<%g %g> XY<%g %g>";

/* GUI */
GtkWidget *isst_window = NULL;
GtkUIManager *isst_ui_manager;
GtkWidget *isst_notebook;
GtkWidget *isst_context;


/* Navigation */
GtkWidget *isst_grid_spin;
GtkWidget *isst_posx_spin;
GtkWidget *isst_posy_spin;
GtkWidget *isst_posz_spin;
GtkWidget *isst_azim_spin;
GtkWidget *isst_elev_spin;
GtkWidget *isst_mouse_speed_spin;

/* Shotline */
GtkWidget *isst_cellx_spin;
GtkWidget *isst_celly_spin;
GtkWidget *isst_deltax_spin;
GtkWidget *isst_deltay_spin;
GtkWidget *isst_name_entry;
GtkWidget *isst_inhit_entry;

/* Fragment Line of Sight */
GtkWidget *isst_flos_posx_spin;
GtkWidget *isst_flos_posy_spin;
GtkWidget *isst_flos_posz_spin;

/* Shotline Table */
GtkListStore *isst_shotline_store;
GtkListStore *isst_saved_shotline_store;

GtkWidget **gwlist;

uint8_t isst_flags;
isst_t isst;

char filename[BUFSIZ];	/* the .g we loaded up */

/* Update the camera data based on the current azimuth and elevation */
#define AZEL_TO_FOC() { \
    V3DIR_FROM_AZEL( isst.camera_foc, isst.camera_az * DEG2RAD, isst.camera_el * DEG2RAD ); \
	VSUB2( isst.camera_foc, isst.camera_pos, isst.camera_foc); }

static void update_camera_widgets ();
static void update_view_callback (GtkWidget *widget, gpointer ptr);

void
generic_dialog (char *message)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (GTK_WINDOW (isst_window),
	    GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_INFO,
	    GTK_BUTTONS_OK,
	    message);

    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    g_signal_connect_swapped (dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);

    gtk_widget_show (dialog);
}

void
paint_context() 
{
    gdk_threads_enter ();
    gdk_window_freeze_updates ( GDK_WINDOW(isst_context->window) );
    /* probably manipulate the image buffer to add the crosshairs? then do away
     * with draw_cross_hairs ? */
    gdk_draw_rgb_image (isst_context->window, isst_context->style->fg_gc[GTK_STATE_NORMAL],
	    0, 0, isst.context_width, isst.context_height, GDK_RGB_DITHER_NONE,
	    isst.buffer_image.data + sizeof(camera_tile_t), isst.context_width * 3);
    if (isst.mode == ISST_MODE_SHOTLINE)
	draw_cross_hairs (isst.mouse_x, isst.mouse_y);
    gdk_window_thaw_updates ( GDK_WINDOW(isst_context->window) );
    gdk_threads_leave ();
}

static void
isst_project_widgets ()
{
    GtkWidget *widget;
    uint8_t mode = isst.connected ? TRUE : FALSE;

#define SWIDG(s,m) gtk_widget_set_sensitive (gtk_ui_manager_get_widget (isst_ui_manager, s), m);
    /* Widgets to display when connected */
    SWIDG("/MainMenu/ModeMenu",mode);
    SWIDG("/MainMenu/ViewMenu",mode);
    SWIDG("/MainMenu/MiscMenu",mode);
#undef SWIDG
}

void
draw_cross_hairs (int16_t x, int16_t y)
{
    uint8_t line[3*isst.context_width];
    int i;
    uint8_t *p;

    /*
    gdk_threads_enter ();
    */
    gdk_window_freeze_updates ( GDK_WINDOW(isst_context->window) );

    /* Update cellx and celly spin buttons */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_cellx_spin), x);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_celly_spin), y);

    /* Restore previous scanlines */

    /* Horizontal */
    p = &((uint8_t *) isst.buffer_image.data)[3 * (isst.mouse_y * isst.context_width) + 0];
    for (i = 0; i < isst.context_width; i++)
    {
	line[3*i+0] = *(p++);
	line[3*i+1] = *(p++);
	line[3*i+2] = *(p++);
    }
    gdk_draw_rgb_image (isst_context->window, isst_context->style->fg_gc[GTK_STATE_NORMAL],
	    0, isst.mouse_y, isst.context_width, 1, GDK_RGB_DITHER_NONE, line, 3 * isst.context_width);

    /* Vertical */
    p = &((uint8_t *)isst.buffer_image.data)[3 * isst.mouse_x];
    for (i = 0; i < isst.context_height; i++)
    {
	line[3*i+0] = *(p+0);
	line[3*i+1] = *(p+1);
	line[3*i+2] = *(p+2);
	p += 3*isst.context_width;
    }
    gdk_draw_rgb_image (isst_context->window, isst_context->style->fg_gc[GTK_STATE_NORMAL],
	    isst.mouse_x, 0, 1, isst.context_height, GDK_RGB_DITHER_NONE, line, 3);


    for (i = 0; i < isst.context_width; i++)
    {
	line[3*i+0] = 0;
	line[3*i+1] = 255;
	line[3*i+2] = 0;
    }

    /* Horizontal Line */
    gdk_draw_rgb_image (isst_context->window, isst_context->style->fg_gc[GTK_STATE_NORMAL], 
	    0, y, isst.context_width, 1, GDK_RGB_DITHER_NONE, line, 3 * isst.context_width);

    /* Vertical Line */
    gdk_draw_rgb_image (isst_context->window, isst_context->style->fg_gc[GTK_STATE_NORMAL], 
	    x, 0, 1, isst.context_height, GDK_RGB_DITHER_NONE, line, 3);

    gdk_window_thaw_updates ( GDK_WINDOW(isst_context->window) );
    /*
    gdk_threads_leave ();
    */
}

static void
destroy (GtkWidget *widget, gpointer data)
{
    isst_free ();
    gtk_main_quit ();

    /* Instruct the master to shut down */

}

static int
attach_master(struct bu_vls *hostname)
{
    GError *error = NULL;

    /* Initiate networking */
    if(strlen(bu_vls_addr(&isst.master)) == 0 || strncmp(bu_vls_addr(&isst.master), "local", 5) == 0) {
	isst.work_frame = isst_local_work_frame;
	g_thread_create (isst_local_worker, 0, FALSE, &error);
    } else {
	isst.work_frame = isst_net_work_frame;
	g_thread_create (isst_net_worker, 0, FALSE, &error);
    }

    isst.connected = 1;
    isst_project_widgets ();

    return 0;
}


static void
menuitem_exit_callback ()
{
    uint8_t op;

    isst.connected = 0;

    if(isst.work_frame == isst_net_work_frame) {
#if 0
	op = ADRT_NETOP_SHUTDOWN;
	tienet_send (isst.socket, &op, 1);
#else
	bu_exit(-1, "Distributed stuff disabled\n");
#endif
    }

    exit (0);
}


static void
isst_update_gui ()
{
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_grid_spin), isst.camera_grid);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_posx_spin), isst.camera_pos[0]);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_posy_spin), isst.camera_pos[1]);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_posz_spin), isst.camera_pos[2]);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_azim_spin), isst.camera_az);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_elev_spin), isst.camera_el);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_cellx_spin), isst.mouse_x);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_celly_spin), isst.mouse_y);
}


#define VIEW(n, p, s, a, e) \
static void menuitem_view_##n##_callback () { \
    VMOVE(isst.camera_pos, isst.geom_center); \
	isst.camera_pos[p] s##= isst.geom_radius; \
	isst.camera_az = a; \
	isst.camera_el = e; \
	isst_update_gui (); \
	AZEL_TO_FOC (); \
	isst.update_avail = 1; \
	isst.work_frame (); \
}

VIEW(front, 0, +,   0, 0);
VIEW(left,  1, +,  90, 0);
VIEW(back,  0, -, 180, 0);
VIEW(right, 1, -, 270, 0);
static void menuitem_view_aiieee_callback () {
	vect_t vec;
	VSUB2(vec, isst.geom_center, isst.camera_pos);
	VUNITIZE(vec);
	printf("%g %g %g\n", V3ARGS(vec));
	AZEL_FROM_V3DIR(isst.camera_az, isst.camera_el, vec);
	isst.camera_az *= -1;
	isst.camera_el *= -1;
	isst_update_gui ();
	AZEL_TO_FOC();
	isst.update_avail = 1;
	isst.work_frame ();
}

static void
view_hit_point_callback (GtkWidget *widget, gpointer ptr)
{
    GtkWidget **wlist;
    float posx, posy, posz, azim, elev;
    float celev;

    wlist = (GtkWidget **) ptr;
    posx = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[1]));
    posy = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[2]));
    posz = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[3]));
    azim = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[4]));
    elev = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[5]));

    /* Update Azimuth and Elevation */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_azim_spin), azim);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_azim_spin), elev);

    /* Update Position */
    AZEL_TO_FOC();

    /* Destroy the window along with all of its widgets */
    gtk_widget_destroy (wlist[0]);

    /* Request a shaded frame */
    isst.work_frame ();

    free (wlist);
}

static void
menuitem_view_hit_point_callback ()
{
    GtkWidget **wlist;
    GtkWidget *window;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *posx_spin, *posy_spin, *posz_spin;
    GtkWidget *azim_spin, *elev_spin;
    GtkWidget *set_view_button;

    /* allocate some ptr memory for widget to pass to callback */
    wlist = (GtkWidget **) malloc(6 * sizeof (GtkWidget *));

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_modal (GTK_WINDOW (window), TRUE);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
    gtk_window_set_title (GTK_WINDOW (window), "View Hit Point");
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

    /* Create a table for the window */
    table = gtk_table_new (6, 2, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (table), TABLE_BORDER_WIDTH+2);
    gtk_container_add (GTK_CONTAINER (window), table);

    /* Position X */
    label = gtk_label_new ("Position(X)");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
    posx_spin = gtk_spin_button_new_with_range (-100, 100, 0.1);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (posx_spin), 3);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (posx_spin), 0.0);
    gtk_table_attach_defaults (GTK_TABLE (table), posx_spin, 1, 2, 0, 1);

    /* Position Y */
    label = gtk_label_new ("Position(Y)");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
    posy_spin = gtk_spin_button_new_with_range (-100, 100, 0.1);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (posy_spin), 3);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (posy_spin), 0.0);
    gtk_table_attach_defaults (GTK_TABLE (table), posy_spin, 1, 2, 1, 2);

    /* Position Z */
    label = gtk_label_new ("Position(Z)");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
    posz_spin = gtk_spin_button_new_with_range (-100, 100, 0.1);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (posz_spin), 3);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (posz_spin), 0.0);
    gtk_table_attach_defaults (GTK_TABLE (table), posz_spin, 1, 2, 2, 3);

    /* Azimuth */
    label = gtk_label_new ("Azimuth");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
    azim_spin = gtk_spin_button_new_with_range (0, 360, 0.1);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (azim_spin), 2);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (azim_spin), 0.0);
    gtk_table_attach_defaults (GTK_TABLE (table), azim_spin, 1, 2, 3, 4);

    /* Elevation */
    label = gtk_label_new ("Elevation");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);
    elev_spin = gtk_spin_button_new_with_range (-90, 90, 0.1);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (elev_spin), 2);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (elev_spin), 0.0);
    gtk_table_attach_defaults (GTK_TABLE (table), elev_spin, 1, 2, 4, 5);

    /* Set View Button */
    set_view_button = gtk_button_new_with_label ("Set View");
    gtk_table_attach_defaults (GTK_TABLE (table), set_view_button, 0, 2, 5, 6);

    wlist[0] = window;
    wlist[1] = posx_spin;
    wlist[2] = posy_spin;
    wlist[3] = posz_spin;
    wlist[4] = azim_spin;
    wlist[5] = elev_spin;
    g_signal_connect (G_OBJECT (set_view_button), "clicked", G_CALLBACK (view_hit_point_callback), wlist);

    gtk_widget_show_all (window);
}


static void
save_screenshot_callback (GtkWidget *widget, gpointer ptr)
{
    const gchar *selected_filename;
    GdkPixmap *pixmap;
    GdkImage *image;
    GdkGC *gc;
    GdkColor colorb, colorf;
    PangoContext *context;
    PangoLayout *layout;
    PangoLanguage *language;
    PangoFontDescription *font_desc;
    int32_t i, width, height, x, y, px, py;
    uint32_t pixel;
    char string[80];
    char overlay[256];
    char ltime[26];
    time_t timer;
    unsigned char *buf;

    context = gdk_pango_context_get();
    language = gtk_get_default_language();
    pango_context_set_language (context, language);
    pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);

    font_desc = pango_font_description_from_string ("terminal 10");
    pango_context_set_font_description (context, font_desc);

    layout = pango_layout_new (context);
    pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);

    sprintf (string, "shotline: %s\n", gtk_entry_get_text (GTK_ENTRY (isst_name_entry)));
    strcpy (overlay, string);

    timer = time (NULL);
    strcpy (ltime, asctime (localtime (&timer)));
    ltime[24] = 0;
    sprintf (string, "date: %s\n", ltime);
    strcat (overlay, string);

    sprintf (string, "position: %.3f %.3f %.3f (m)\n", isst.camera_pos[0], isst.camera_pos[1], isst.camera_pos[2]);
    strcat (overlay, string);

    sprintf (string, "az: %.3f, el: %.3f (deg)\n", isst.camera_az, isst.camera_el);
    strcat (overlay, string);

    sprintf (string, "grid: %.2fm x %.2fm\n", isst.camera_grid, ((float) isst.context_height) / ((float) isst.context_width) * isst.camera_grid);
    strcat (overlay, string);

    sprintf (string, "in-hit: %s (m)", gtk_entry_get_text (GTK_ENTRY (isst_inhit_entry)));
    strcat (overlay, string);

    pango_layout_set_text (layout, overlay, -1);
    pango_layout_get_pixel_size (layout, &width, &height);
    width += 2; /* padding */
    height += 1; /* padding */

    pixmap = gdk_pixmap_new (NULL, width, height, 24);
    gc = gdk_gc_new (pixmap);
    colorf.pixel = 0x00ffffff;
    colorb.pixel = 0x00404040;

    gdk_gc_set_foreground (gc, &colorb);
    gdk_gc_set_background (gc, &colorb);
    gdk_draw_rectangle (pixmap, gc, TRUE, 0, 0, width, height);
    gdk_gc_set_foreground (gc, &colorf);
    gdk_gc_set_background (gc, &colorb);

    gdk_draw_layout (pixmap, gc, 2, 0, layout);

    image = gdk_drawable_get_image (pixmap, 0, 0, width, height);

    selected_filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (widget));

    /* Put text onto the image */
    for (y = 0; y < height; y++)
	for (x = 0; x < width; x++)
	{
	    pixel = gdk_image_get_pixel (image, x, y);
	    if (x == 0 || y == 0 || x == width-1 || y == height-1)
		pixel = 0xffffffff;
	    isst.buffer_image.data[3 * (y * isst.context_width + x) + 0] = (pixel & 0x000000ff)>>0;
	    isst.buffer_image.data[3 * (y * isst.context_width + x) + 1] = (pixel & 0x0000ff00)>>8;
	    isst.buffer_image.data[3 * (y * isst.context_width + x) + 2] = (pixel & 0x00ff0000)>>16;
	}

    px = (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_cellx_spin));
    py = (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_celly_spin));

    /* Draw a dot where the hairs cross */
    for (y = py - 20; y <= py + 20; y++)
	for (x = px - 20; x <= px + 20; x++)
	{
	    if (x >= 0 && x < isst.context_width && y >= 0 && y < isst.context_height)
	    {
		if (y == py || x == px)
		{
		    isst.buffer_image.data[3 * (y * isst.context_width + x) + 0] = 0;
		    isst.buffer_image.data[3 * (y * isst.context_width + x) + 1] = 255;
		    isst.buffer_image.data[3 * (y * isst.context_width + x) + 2] = 0;
		}
		if ((x-px)*(x-px) + (y-py)*(y-py) <= 9)
		{
		    isst.buffer_image.data[3 * (y * isst.context_width + x) + 0] = 255;
		    isst.buffer_image.data[3 * (y * isst.context_width + x) + 1] = 0;
		    isst.buffer_image.data[3 * (y * isst.context_width + x) + 2] = 162;
		}
	    }
	}

    {   
	icv_image_t *bif = icv_create(isst.context_width, isst.context_height, ICV_COLOR_SPACE_RGB);
	/* flip image */
	for(i=0;i<isst.context_height;i++) {
	    icv_writeline(bif, i, (unsigned char *)isst.buffer_image.data + (isst.context_height-i)*isst.context_width*3, ICV_DATA_UCHAR);
	}
	/* Save Image */
	icv_save( bif, (char *)selected_filename, ICV_IMAGE_AUTO);
	icv_free(bif);
    }

    isst.work_frame ();
    gtk_widget_destroy (widget);
}


static void
menuitem_misc_screenshot_callback ()
{
    GtkWidget *file_selector;

    file_selector = gtk_file_selection_new ("Save Screenshot");
    gtk_window_set_modal (GTK_WINDOW (file_selector), TRUE);

    g_signal_connect_swapped (GTK_FILE_SELECTION (file_selector)->ok_button, "clicked", G_CALLBACK (save_screenshot_callback), file_selector);
    g_signal_connect_swapped (GTK_FILE_SELECTION (file_selector)->cancel_button, "clicked", G_CALLBACK (gtk_widget_destroy), file_selector);
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_selector), "screenshot.png");

    gtk_widget_show_all (file_selector);
}

#define VIEW_CALLBACK(n, m, e) \
static void \
    menuitem_##n##_callback () \
{ \
    isst.mode = ISST_MODE_##m; \
	isst.mode_updated = 1; \
	gtk_notebook_set_current_page (GTK_NOTEBOOK (isst_notebook), isst.notebook_index[isst.mode]); \
	e; \
	isst.work_frame (); \
}
#define VIEW_CB(n, m) VIEW_CALLBACK(n, m, isst.camera_type=RENDER_CAMERA_PERSPECTIVE);

VIEW_CB(view_shaded, SHADED);
VIEW_CB(view_normal, NORMAL);
VIEW_CB(view_depth, DEPTH);
VIEW_CB(view_component, COMPONENT);
VIEW_CALLBACK(view_cut, CUT, isst.camera_type=RENDER_CAMERA_PERSPECTIVE);
VIEW_CALLBACK(flos, FLOS, isst.camera_type=RENDER_CAMERA_PERSPECTIVE;
    VSET(isst.shotline_pos,  gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_flos_posx_spin)),  gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_flos_posy_spin)),  gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_flos_posz_spin))));
VIEW_CALLBACK(shotline, SHOTLINE, 
	isst.camera_type = RENDER_CAMERA_ORTHOGRAPHIC;
	isst.mouse_x = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_cellx_spin));
	isst.mouse_y = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_celly_spin)));

static int
load_g_project_callback (const char *file, const char **region)
{
    uint8_t op;
    char buf[BUFSIZ];
    int size, i, rval, nreg = 0;

    const char **r = region;
    while(*r++) ++nreg;

    attach_master(&isst.master);

    snprintf(filename, BUFSIZ, "%s", file);
    if(isst.work_frame == isst_local_work_frame) {
	    struct bu_vls times;
	    vect_t mid;

	    gtk_widget_show_all (isst_window);
	    
	    bu_vls_init (&times);
	    /* init/load/prep the tie engine */
	    rt_prep_timer();
	    rval = load_g(tie, file, nreg, region, &(isst.meshes));
	    rt_get_timer(&times, NULL);

	    printf("\nTime to load: %s\n\n", bu_vls_addr(&times));
	    bu_vls_free(&times);

	    isst.update_avail = 1;
	    isst.pid = 0;	/* no project id's here, but not -1 */

	    sleep(0);	/* O.o get nan's without this. */
	    VMOVE(isst.geom_min, tie->min);
	    VMOVE(isst.geom_max, tie->max);
	    VMOVE(isst.geom_center, tie->mid);
	    VMOVE(isst.camera_foc,  isst.geom_center);
	    isst.camera_grid = tie->radius*2;

	    VSUB2SCALE(mid, isst.geom_max, isst.geom_min, 0.5);

	    isst.geom_radius = tie->radius;

	    V3DIR_FROM_AZEL(mid, isst.camera_az, isst.camera_el);
	    VSCALE(mid, mid, -isst.geom_radius);
	    VADD2(isst.camera_pos, isst.camera_foc, mid);
	    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_mouse_speed_spin), isst.geom_radius * 0.002);
	    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_grid_spin), isst.geom_radius);
	    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_grid_spin), isst.camera_grid);
	    update_camera_widgets();
	    update_view_callback(NULL, gwlist);
    } else {
	/*
	   op = ADRT_NETOP_REQWID;
	   tienet_send (isst.socket, &op, 1);
	   tienet_recv (isst.socket, &isst.wid, 2);
	   */
#if 0
	op = ADRT_NETOP_LOAD;
	tienet_send (isst.socket, &op, 1);

	size = sizeof(op) + sizeof(isst.wid) + 1 + strlen("*/tmp/ktank.g:tank");

	/* send size */
	tienet_send (isst.socket, &size, 4);

	snprintf(buf, BUFSIZ, "%c  %c/tmp/ktank.g:tank", ADRT_WORK_INIT, ADRT_LOAD_FORMAT_G);
	*(uint16_t *)(buf+1) = isst.wid;

	tienet_send (isst.socket, buf, size);
	load_frame_attribute();
#else
	bu_exit(-1, "Distributed stuff disabled\n");
#endif
    }

    isst.work_frame ();
    return rval;
}

void
shotline_load_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer usr)
{
    char shot[BUFSIZ], hn[BUFSIZ], filename[BUFSIZ];
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(isst_saved_shotline_store, &iter, path))
    {
	gchar *str;
	float w, h;
	int i;
	gtk_tree_model_get(isst_saved_shotline_store, &iter, 1, &str, -1);
	i = sscanf(str, shotfilefmt,
			&shot,
			&isst.camera_pos[0], &isst.camera_pos[1], &isst.camera_pos[2],
			&isst.camera_az, &isst.camera_el, &w, &h);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_cellx_spin), w * (fastf_t) isst.context_width);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_celly_spin), h * (fastf_t) isst.context_height);
	g_free(str);
	gtk_widget_destroy(GTK_WIDGET(usr));
    }
    isst_update_gui ();
    AZEL_TO_FOC ();
    isst.update_avail = 1;
    isst.work_frame ();
}

static void
menuitem_load_shotline_callback()
{
    GtkWidget *window, *view;
    GtkCellRenderer *renderer;
    GtkTreeModel *model;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    view = gtk_tree_view_new ();
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), -1, "Name", renderer, "text", 0, NULL);
    gtk_tree_view_set_model (GTK_TREE_VIEW (view), isst_saved_shotline_store);
    g_signal_connect(view, "row-activated", (GCallback)shotline_load_activated, window);
    gtk_container_add (GTK_CONTAINER (window), view);
    gtk_widget_show_all (window);
}

static void
menuitem_load_g_callback ()
{
    GtkWidget *dialog, *table, *file, *regions, *label;
    gint res;

    dialog = gtk_dialog_new_with_buttons (
	    "Load BRL-CAD Geometry", 
	    GTK_WINDOW(isst_window), 
	    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
	    GTK_STOCK_OK, GTK_RESPONSE_OK, 
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
	    NULL);

    table = gtk_table_new (2, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);

    label = gtk_label_new_with_mnemonic ("_File");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
    file = gtk_entry_new ();
    gtk_table_attach_defaults (GTK_TABLE (table), file, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), file);

    label = gtk_label_new_with_mnemonic ("_Region");
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
    regions = gtk_entry_new ();
    gtk_table_attach_defaults (GTK_TABLE (table), regions, 1, 2, 1, 2);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), regions);

    gtk_widget_show_all (table);
    gtk_widget_show (dialog);

AGAIN:
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
	const char *regs[2] = {NULL, NULL};	/* only handling one region from dialog */
	regs[0] = gtk_entry_get_text(GTK_ENTRY(regions));
	if(load_g_project_callback(gtk_entry_get_text(GTK_ENTRY(file)), regs)) {
	    GtkWidget *heh;
	    heh = gtk_message_dialog_new (GTK_WINDOW (isst_window),
		    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		    "Must enter a valid .g filename");
	    gtk_dialog_run (GTK_DIALOG (heh));
	    gtk_widget_destroy (heh);
	    goto AGAIN;
	}
    }
    gtk_widget_destroy(dialog);
}

static void
menuitem_about_callback ()
{
    char message[128];

    sprintf (message, "ISST Version 1.0\nBUILD DATE: %s\nBUILD TIME: %s", __DATE__, __TIME__);
    generic_dialog (message);
}


static void
update_view_callback (GtkWidget *widget, gpointer ptr)
{
    GtkWidget **wlist = (GtkWidget **)ptr;

    /* Update camera position */
    VSET(isst.camera_pos,  gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[0])),  gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[1])),  gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[2])));

    /* Update camera azimuth and elevation */
    isst.camera_az = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[3]));
    isst.camera_el = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[4]));

    /* Field of View */
    isst.camera_type = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wlist[5])) ? RENDER_CAMERA_PERSPECTIVE : RENDER_CAMERA_ORTHOGRAPHIC;
    isst.camera_fov = gtk_spin_button_get_value (GTK_SPIN_BUTTON (wlist[6]));
    isst.camera_grid = gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_grid_spin));

    /* Adjust Mouse Speed */
    isst.mouse_speed = gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_mouse_speed_spin));

    AZEL_TO_FOC ();

    isst.work_frame ();
}


static void
update_cellx_callback (GtkWidget *widget, gpointer ptr)
{
    int16_t x, y;

    x = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_cellx_spin));
    y = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_celly_spin));

    draw_cross_hairs (x, y);
    isst.mouse_x = x;
}


static void
update_celly_callback (GtkWidget *widget, gpointer ptr)
{
    int16_t x, y;

    x = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_cellx_spin));
    y = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_celly_spin));

    draw_cross_hairs (x, y);
    isst.mouse_y = y;
}


static void
apply_delta_callback (GtkWidget *widget, gpointer ptr)
{
    int16_t cellx, celly, deltax, deltay;

    deltax = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_deltax_spin));
    deltay = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_deltay_spin));

    cellx = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_cellx_spin));
    celly = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_celly_spin));

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_cellx_spin), cellx + (((fastf_t) deltax) / ((fastf_t) 1000*isst.camera_grid)) * (fastf_t) isst.context_width);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_celly_spin), celly - (((fastf_t) deltay) / ((fastf_t) 1000*isst.camera_grid)) * (fastf_t) isst.context_height);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_deltax_spin), 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_deltay_spin), 0);	

    cellx = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_cellx_spin));
    celly = (int16_t) gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_celly_spin));

    gdk_window_freeze_updates ( GDK_WINDOW(isst_context->window) );
    draw_cross_hairs (cellx, celly);
    gdk_window_thaw_updates ( GDK_WINDOW(isst_context->window) );

    isst.mouse_x = cellx;
    isst.mouse_y = celly;
}

static gboolean model_fwrite(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    gchar *str;

    gtk_tree_model_get(model, iter, 1, &str);
    fprintf((FILE *)data, "%s\n", str);
    return FALSE;
}

static void
save_shotline_callback (GtkWidget *widget, gpointer ptr)
{
    FILE *out;
    char slf[BUFSIZ], buf[BUFSIZ], hn[BUFSIZ], ss[BUFSIZ];
    const char *shotname;
    GtkTreeIter iter;

    shotname = gtk_entry_get_text (GTK_ENTRY (isst_name_entry));
    if (strlen(shotname) == 0) { 
	generic_dialog ("Must provide name for shotline.");
	return;
    }

    /* update the table */
    if(!gethostname(hn, BUFSIZ))
	snprintf(hn, BUFSIZ, "unknown");

    snprintf (ss, BUFSIZ, shotfilefmt,
	    shotname, V3ARGS(isst.camera_pos), isst.camera_az, isst.camera_el,
	    gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_cellx_spin)) / (fastf_t) isst.context_width,
	    gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_celly_spin)) / (fastf_t) isst.context_height);

    gtk_list_store_append(isst_saved_shotline_store, &iter);
    gtk_list_store_set(isst_saved_shotline_store, &iter, 0, shotname, 1, ss, -1);

    /* write the table to the file */
    snprintf (slf, BUFSIZ, "%s/.isst", getenv("HOME"));

    if ((out = fopen(slf, "w")) == NULL) {
	snprintf(buf, BUFSIZ, "Unable to open %s for writing", slf);
	generic_dialog (buf);
	return;
    }
    gtk_tree_model_foreach(GTK_TREE_MODEL(isst_saved_shotline_store), model_fwrite, out);
    fclose(out);

    snprintf(buf, BUFSIZ, "Shotline %s saved to %s", shotname, slf);
    generic_dialog(buf);
    return;
}

static void
component_select_callback (GtkWidget *widget, gpointer ptr)
{
    GtkWidget *component_entry;
    uint32_t size, num;
    uint8_t op, c;
    char *str;

    str = (char *)gtk_entry_get_text (GTK_ENTRY ((GtkWidget *)ptr) );

    if(str[0] == 0)	/* no empty strings. */
	return;

    if(isst.work_frame == isst_local_work_frame) {
	struct adrt_mesh_s *m;
	for(BU_LIST_FOR(m, adrt_mesh_s, &isst.meshes->l))
	    /* should this search be case insensitive? regex? */
	    m->flags |= strnstr(m->name, str, 255) ? ADRT_MESH_SELECT : 0;
    } else {
#if 0
	c = (uint8_t)(strlen (str) + 1);

	/* Send request for next frame */
	op = ADRT_NETOP_WORK;
	tienet_send (isst.socket, &op, 1);

	/* size */
	size = 1 + 1 + 4 + 1 + c;
	tienet_send (isst.socket, &size, 4);

	/* slave function */
	op = ADRT_WORK_SELECT;
	tienet_send (isst.socket, &op, 1);

	/* reset flag off */
	op = 0;
	tienet_send (isst.socket, &op, 1);

	/* number of strings to select */
	num = 1;
	tienet_send (isst.socket, &num, 4);

	/* string length and string */
	tienet_send (isst.socket, &c, 1);
	tienet_send (isst.socket, str, c);
#else
	bu_exit(-1, "Distributed stuff disabled");
#endif
    }

    /* Update the component view */
    isst.work_frame();
}


static void
component_deselect_all_callback (GtkWidget *widget, gpointer ptr)
{
    uint32_t size, num;
    uint8_t op;


    if(isst.work_frame == isst_local_work_frame) {
	struct adrt_mesh_s *m;
	for(BU_LIST_FOR(m, adrt_mesh_s, &isst.meshes->l))
	    m->flags = 0;
    } else {
	/* Send request for next frame */
#if 0
	op = ADRT_NETOP_WORK;
	tienet_send (isst.socket, &op, 1);

	/* size */
	size = 1 + 1 + 4;
	tienet_send (isst.socket, &size, 4);

	/* slave function */
	op = ADRT_WORK_SELECT;
	tienet_send (isst.socket, &op, 1);

	/* reset flag off */
	op = 1;
	tienet_send (isst.socket, &op, 1);

	/* number of strings to select */
	num = 0;
	tienet_send (isst.socket, &num, 4);
#else
	bu_exit(-1, "Distributed stuff disabled");
#endif
    }

    /* Update the component view */
    isst.work_frame();
}


static void
flos_use_current_position_callback (GtkWidget *widget, gpointer ptr)
{
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_flos_posx_spin), isst.camera_pos[0]);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_flos_posy_spin), isst.camera_pos[1]);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_flos_posz_spin), isst.camera_pos[2]);
}


static void
fire_shotline_callback (GtkWidget *widget, gpointer ptr)
{
    uint32_t size;
    uint8_t op;
    render_camera_t camera;
    struct tie_ray_s ray;
    point_t v1, v2;

    /* Calculate Position and Direction of ray for shotline based on Cell coordinates */
    render_camera_init (&camera, 1);
    camera.w = isst.context_width;
    camera.h = isst.context_height;
    camera.type = RENDER_CAMERA_ORTHOGRAPHIC;
    VMOVE(camera.pos, isst.camera_pos);
    VMOVE(camera.focus, isst.camera_foc);
    camera.gridsize = isst.camera_grid;
    render_camera_prep (&camera);

    VMOVE(ray.pos, camera.view_list[0].pos);
    VMOVE(ray.dir, camera.view_list[0].top_l);

    VSCALE(v1,  camera.view_list[0].step_x,  gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_cellx_spin)));
    VSCALE(v2,  camera.view_list[0].step_y,  gtk_spin_button_get_value (GTK_SPIN_BUTTON (isst_celly_spin)));
    VADD2(ray.pos,  ray.pos,  v1);
    VADD2(ray.pos,  ray.pos,  v2);

    if(isst.work_frame == isst_local_work_frame) {
	char *msg = NULL;
	int dlen;
	point_t inhit;
	uint32_t ind, num, i;
	char text[256], thickness[16];
	uint8_t c;
	GtkTreeIter iter;
	fastf_t t;

	ray.depth = 0;
	render_util_shotline_list(tie, &ray, (void **)&msg, &dlen);

	ind = 0;
	/* In-Hit */
	memcpy(&inhit, msg+ind, sizeof(point_t));
	/*
	TCOPY(point_t, msg, ind, &inhit, 0);
	*/
	ind += sizeof (point_t);
	sprintf (text, "%.3f %.3f %.3f", inhit[0], inhit[1], inhit[2]);
	gtk_entry_set_text (GTK_ENTRY (isst_inhit_entry), text);

	TCOPY(uint32_t, msg, ind, &num, 0);
	ind += 4;

	/* Update component table */
	gtk_list_store_clear (isst_shotline_store);
	for (i = 0; i < num; i++)
	{
	    TCOPY(uint8_t, msg, ind, &c, 0);
	    ind += 1;

	    bcopy(&msg[ind], text, c);
	    ind += c;

	    TCOPY(fastf_t, msg, ind, &t, 0);
	    t *= 1000; /* Convert to milimeters */
	    ind += sizeof (fastf_t);
	    sprintf (thickness, "%.1f", t);

	    gtk_list_store_append (isst_shotline_store, &iter);
	    gtk_list_store_set (isst_shotline_store, &iter, 0, i+1, 1, text, 2, thickness, -1);
	}

	/* Shotline position and direction */
	memcpy( &isst.shotline_pos, msg+ind, sizeof(point_t));
	ind += sizeof (point_t);
	memcpy( &isst.shotline_dir, msg+ind, sizeof(point_t));
	ind += sizeof (point_t);
    } else {
	/* Send request for next frame */
#if 0
	op = ADRT_NETOP_WORK;
	tienet_send (isst.socket, &op, 1);

	/* size */
	size = 1 + 2 + 2 * sizeof (point_t);
	tienet_send (isst.socket, &size, 4);

	/* slave function */
	op = ADRT_WORK_SHOTLINE;
	tienet_send (isst.socket, &op, 1);

	/* workspace id */
	tienet_send (isst.socket, &isst.wid, 2);

	/* Send Position and Direction */
	tienet_send (isst.socket, ray.pos, sizeof (point_t));
	tienet_send (isst.socket, ray.dir, sizeof (point_t));
#else
	bu_exit(-1, "Distributed stuff disabled\n");
#endif
    }
}

/* Create a new backing pixmap of the appropriate size */
static gboolean
context_configure_event( GtkWidget *widget, GdkEventConfigure *event )
{
    return TRUE;
}

static void
update_camera_widgets ()
{
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_posx_spin), isst.camera_pos[0]);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_posy_spin), isst.camera_pos[1]);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_posz_spin), isst.camera_pos[2]);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_azim_spin), isst.camera_az);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_elev_spin), isst.camera_el);
}


static gboolean
context_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
    if(widget->allocation.width != isst.context_width || widget->allocation.height != isst.context_height) {
	isst.context_width = widget->allocation.width;
	isst.context_height = widget->allocation.height;
	TIENET_BUFFER_SIZE(isst.buffer_image, 3 * isst.context_width * isst.context_height);
	isst.update_avail = 1;
    }
    gdk_draw_rgb_image (widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
	    0, 0, isst.context_width, isst.context_height, GDK_RGB_DITHER_NONE,
	    isst.buffer_image.data, isst.context_width * 3);

    /* Draw cross hairs if in shotline mode */
    if (isst.mode == ISST_MODE_SHOTLINE)
	draw_cross_hairs (isst.mouse_x, isst.mouse_y);

    return FALSE;
}


static gboolean
context_button_event (GtkWidget *widget, GdkEventButton *event)
{
    /* Do nothing if a project has not yet been loaded */
    if (isst.pid < 0)
	return 0;

    if (isst.mode == ISST_MODE_SHOTLINE)
	draw_cross_hairs (event->x, event->y);

    isst.mouse_x = (short)event->x;
    isst.mouse_y = (short)event->y;

    return TRUE;
}


static gboolean
context_scroll_event (GtkWidget *widget, GdkEventScroll *event)
{
    if (event->direction == GDK_SCROLL_UP)
	isst.mouse_speed *= 1.1;
    else if (event->direction == GDK_SCROLL_DOWN)
	isst.mouse_speed *= 1.0/1.1;

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_mouse_speed_spin), isst.mouse_speed);
    return TRUE;
}


static gboolean
context_motion_event (GtkWidget *widget, GdkEventMotion *event)
{
    int16_t dx;
    int16_t dy;
    uint8_t update = 1;

    /* Do nothing if a project has not yet been loaded */
    if (isst.pid < 0)
	return 0;

    dx = (int16_t) (event->x - isst.mouse_x);
    dy = (int16_t) (event->y - isst.mouse_y);

    /* Do Not Generate new Frames */
    if (isst.mode == ISST_MODE_SHOTLINE)
	update = 0;

    pthread_mutex_lock (&isst.update_mut);
    if (event->state & GDK_BUTTON1_MASK)
    {
	if (isst.mode == ISST_MODE_SHOTLINE)
	{
	    CLAMP(event->x, 0, isst.context_width - 1);
	    CLAMP(event->y, 0, isst.context_height - 1);
	    draw_cross_hairs (event->x, event->y);
	}
	else
	{
	    point_t vec;

	    /* backward and forward movement */
	    VSUB2(vec,  isst.camera_foc,  isst.camera_pos);
	    VSCALE(vec,  vec,  (isst.mouse_speed*-dy));
	    VADD2(isst.camera_pos,  isst.camera_pos,  vec);

	    update_camera_widgets ();
	}
    }

    if (event->state & GDK_BUTTON2_MASK)
    {
	point_t up, side, look;

	/* relative up/down left/right movement */

	VSET(up,  0,  0,  1);
	VSUB2(look,  isst.camera_foc,  isst.camera_pos);
	VUNITIZE(look);
	/* side vector */
	VCROSS(side,  look,  up);
	/* up vector */
	VCROSS(up,  look,  side);

	/* left/right */
	VSCALE(side,  side,  isst.mouse_speed*dx);
	VADD2(isst.camera_pos,  isst.camera_pos,  side);

	/* up/down */
	VSCALE(up,  up,  isst.mouse_speed*dy);
	VADD2(isst.camera_pos,  isst.camera_pos,  up);

	update_camera_widgets ();
    }

    if (event->state & GDK_BUTTON3_MASK)
    {
	/* look around / rotate */
	isst.camera_az -= (.4/3)*dx;
	isst.camera_el += (.4/3)*dy;

	while(isst.camera_az < 0.0) isst.camera_az += 360.0;
	while(isst.camera_az > 360.0) isst.camera_az -= 360.0;
	CLAMP(isst.camera_el, -90.0,  90.0);

	update_camera_widgets ();
    }

    if (event->state & GDK_BUTTON1_MASK ||
	    event->state & GDK_BUTTON2_MASK ||
	    event->state & GDK_BUTTON3_MASK)
    {
	isst.mouse_x = (int16_t) event->x;
	isst.mouse_y = (int16_t) event->y;
    }

    AZEL_TO_FOC ();
    isst.update_avail = 1;

    if (isst.update_idle && update)
    {
	isst.update_idle = 0;
	isst.work_frame ();
    }

    pthread_mutex_unlock (&isst.update_mut);

    return FALSE;
}


static GtkActionEntry entries[] = {
    { "ISSTMenu",		NULL,			"_ISST" },
    { "Load G",		GTK_STOCK_OPEN,		"Load _G",		"<control>G",	"Load G",		menuitem_load_g_callback },
    { "Load Shotline",		GTK_STOCK_OPEN,		"Load _Shotline",		"<control>S",	"Load S",		menuitem_load_shotline_callback },
    { "Quit",		GTK_STOCK_QUIT,		"_Quit",		"<control>Q",	"Quit",			menuitem_exit_callback },
    { "ModeMenu",		NULL,			"_Mode" },
    { "Shaded View",	NULL,			"Shaded View",		NULL,		"Shaded View",		menuitem_view_shaded_callback },
    { "Normal View",	NULL,			"Normal View",		NULL,		"Normal View",		menuitem_view_normal_callback },
    { "Depth View",		NULL,			"Depth View",		NULL,		"Depth View",		menuitem_view_depth_callback },
    { "Component View",	NULL,			"Component View",	NULL,		"Component View",	menuitem_view_component_callback },
    { "Cut View",		NULL,			"Cut View",		NULL,		"Cut View",		menuitem_view_cut_callback },
    { "Shotline",		NULL,			"Shotline",		NULL,		"Shotline",		menuitem_shotline_callback },
#if 0
    { "FLOS",		NULL,			"FLOS",			NULL,		"FLOS",			menuitem_flos_callback },
    { "BAD",		NULL,			"BAD",			NULL,		"BAD",			menuitem_exit_callback },
#endif
    { "ViewMenu",		NULL,			"_View" },
    { "Front",		NULL,			"Front",		NULL,		"Front",		menuitem_view_front_callback },
    { "Back",		NULL,			"Back",			NULL,		"Back",			menuitem_view_back_callback },
    { "Left",		NULL,			"Left",			NULL,		"Left",			menuitem_view_left_callback },
    { "Right",		NULL,			"Right",		NULL,		"Right",		menuitem_view_right_callback },
    { "Aiieee",		NULL,			"Aiieee",		NULL,		"Aiieee",		menuitem_view_aiieee_callback },
    { "Hit Point",		NULL,			"Hit Point",		NULL,		"Hit Point",		menuitem_view_hit_point_callback },
    { "MiscMenu",		NULL,			"_Misc" },
    { "Screen Capture",	NULL,			"Screen Capture",	NULL,		"Screen Capture",	menuitem_misc_screenshot_callback },
    { "HelpMenu",		NULL,			"_Help" },
    { "About ISST",		GTK_STOCK_ABOUT,	"_About ISST",		"<control>A",	"About ISST",		menuitem_about_callback },
};


static const char *ui_description =
"<ui>"
"	<menubar name='MainMenu'>"
"		<menu action='ISSTMenu'>"
"			<menuitem action='Load G'/>"
"			<menuitem action='Load Shotline'/>"
"			<menuitem action='Quit'/>"
"		</menu>"
"		<menu action='ModeMenu'>"
"			<menuitem action='Shaded View'/>"
"			<menuitem action='Normal View'/>"
"			<menuitem action='Depth View'/>"
"			<menuitem action='Component View'/>"
"			<menuitem action='Cut View'/>"
"			<menuitem action='Shotline'/>"
#if 0
"			<menuitem action='FLOS'/>"
"			<menuitem action='BAD'/>"
#endif
"		</menu>"
"		<menu action='ViewMenu'>"
"			<menuitem action='Front'/>"
"			<menuitem action='Back'/>"
"			<menuitem action='Left'/>"
"			<menuitem action='Right'/>"
"			<menuitem action='Aiieee'/>"
"			<separator/>"
"			<menuitem action='Hit Point'/>"
"		</menu>"
"		<menu action='MiscMenu'>"
"			<menuitem action='Screen Capture'/>"
"		</menu>"
"		<menu action='HelpMenu'>"
"			<menuitem action='About ISST'/>"
"		</menu>"
"	</menubar>"
"</ui>";


/* Returns a menubar widget made from the above menu */
static GtkWidget*
build_menubar (GtkWidget *window)
{
    /* Create menu bar */
    GtkActionGroup *action_group;
    GtkAccelGroup *accel_group;
    GtkWidget *menubar;
    GError *error;

    action_group = gtk_action_group_new ("MenuActions");
    gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), isst_window);
    isst_ui_manager = gtk_ui_manager_new ();
    gtk_ui_manager_insert_action_group (isst_ui_manager, action_group, 0);

    accel_group = gtk_ui_manager_get_accel_group (isst_ui_manager);
    gtk_window_add_accel_group (GTK_WINDOW (isst_window), accel_group);

    error = NULL;
    if (!gtk_ui_manager_add_ui_from_string (isst_ui_manager, ui_description, -1, &error))
    {
	g_message ("building menus failed: %s", error->message);
	g_error_free (error);
	exit (EXIT_FAILURE);
    }

    menubar = gtk_ui_manager_get_widget (isst_ui_manager, "/MainMenu");

    return (menubar);
}


void
isst_gui ()
{
    GtkWidget *main_vbox, *menu_bar, *hp, *vp;

    /* Create the window */
    isst_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (GTK_WIDGET (isst_window), ISST_WINDOW_W, ISST_WINDOW_H);
    gtk_window_set_title (GTK_WINDOW (isst_window), "ISST-isst-GTK");
    g_signal_connect (G_OBJECT (isst_window), "destroy", G_CALLBACK (destroy), NULL);

    /* Create a vbox for menu bar and panels */
    main_vbox = gtk_vbox_new (FALSE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), TABLE_BORDER_WIDTH);
    gtk_container_add (GTK_CONTAINER (isst_window), main_vbox);

    /* Create menu bar */
    menu_bar = build_menubar (isst_window);
    gtk_box_pack_start (GTK_BOX (main_vbox), menu_bar, FALSE, FALSE, 0);
    isst_project_widgets ();

    hp = gtk_hpaned_new();
    gtk_box_pack_end (GTK_BOX (main_vbox), hp, TRUE, TRUE, 0);

    vp = gtk_vpaned_new();
    gtk_paned_add2(GTK_PANED(hp), GTK_WIDGET(vp));

    /* Create the rendering context */
    isst_context = gtk_drawing_area_new ();
    gtk_widget_set_size_request (GTK_WIDGET (isst_context), isst.context_width, isst.context_height);
    gtk_paned_add1(GTK_PANED(vp), GTK_WIDGET(isst_context));

    /* Signals for drawing pixmap */
    g_signal_connect (G_OBJECT (isst_context), "configure_event", G_CALLBACK (context_configure_event), NULL);
    g_signal_connect (G_OBJECT (isst_context), "expose_event", G_CALLBACK (context_expose_event), NULL);

    /* Event signals */
    g_signal_connect (G_OBJECT (isst_context), "button_press_event", G_CALLBACK (context_button_event), NULL);
    g_signal_connect (G_OBJECT (isst_context), "scroll_event", G_CALLBACK (context_scroll_event), NULL);
    g_signal_connect (G_OBJECT (isst_context), "motion_notify_event", G_CALLBACK (context_motion_event), NULL);

    gtk_widget_set_events (isst_context, GDK_EXPOSURE_MASK
	    | GDK_LEAVE_NOTIFY_MASK
	    | GDK_BUTTON_PRESS_MASK
	    | GDK_SCROLL_MASK
	    | GDK_POINTER_MOTION_MASK);


    {	/*** the shotline info (bottom) ***/
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *sw;
	GtkTreeModel *table;
	GtkWidget *treeview;

	/* Create the table under the context */
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_add2(GTK_PANED(vp), GTK_WIDGET(sw));
	/* Make room for horizontal scroll bar */
	gtk_widget_set_size_request (GTK_WIDGET (sw), isst.context_width, ISST_WINDOW_H-isst.context_height - 28);

	/* create the project table */
	isst_shotline_store = gtk_list_store_new (3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING);

	table = GTK_TREE_MODEL (isst_shotline_store);

	/* create tree view */
	treeview = gtk_tree_view_new_with_model (table);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview), 0);

	g_object_unref (table);
	gtk_container_add (GTK_CONTAINER (sw), treeview);

	/* add columns to the table */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("#", renderer, "text", 0, NULL);
	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Component", renderer, "text", 1, NULL);
	gtk_tree_view_column_set_sort_column_id (column, 1);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Thick (mm)", renderer, "text", 2, NULL);
	gtk_tree_view_column_set_sort_column_id (column, 2);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
    }

    /*** the left chunk ***/
    /* Create the notebook to contain widgets for each mode */
    isst_notebook = gtk_notebook_new ();
    gtk_widget_set_size_request (GTK_WIDGET (isst_notebook), ISST_WINDOW_W - isst.context_width, ISST_WINDOW_H);
    gtk_paned_add1(GTK_PANED(hp), GTK_WIDGET(isst_notebook));
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (isst_notebook), 0);

    {
	GtkWidget *navigation_tab, *navigation_label;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *alignment;
	GtkWidget *radio_perspective;
	GtkWidget *radio_orthographic;
	GtkWidget *fov_spin;
	GtkWidget *update_button;
	GtkWidget **wlist;

	navigation_tab = gtk_frame_new ("navigation");
	navigation_label = gtk_label_new ("navigation");
	isst.notebook_index[ISST_MODE_SHADED] = gtk_notebook_append_page (GTK_NOTEBOOK (isst_notebook), navigation_tab, navigation_label);


	alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (navigation_tab), alignment);

	/* Create a table for inputs */
	table = gtk_table_new (8, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), TABLE_BORDER_WIDTH);
	gtk_container_add (GTK_CONTAINER (alignment), table);

	/* Position X */
	label = gtk_label_new ("Position X");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

	isst_posx_spin = gtk_spin_button_new_with_range (-100.0, 100.0, 0.1);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_posx_spin, 1, 2, 0, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_posx_spin), isst.camera_pos[0]);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_posx_spin), 3);

	/* Position Y */
	label = gtk_label_new ("Position Y");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

	isst_posy_spin = gtk_spin_button_new_with_range (-100.0, 100.0, 0.1);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_posy_spin, 1, 2, 1, 2);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_posy_spin), isst.camera_pos[1]);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_posy_spin), 3);

	/* Position Z */
	label = gtk_label_new ("Position Z");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);

	isst_posz_spin = gtk_spin_button_new_with_range (-100.0, 100.0, 0.1);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_posz_spin, 1, 2, 2, 3);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_posz_spin), isst.camera_pos[2]);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_posz_spin), 3);

	/* Azimuth */
	label = gtk_label_new ("Azimuth");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);

	isst_azim_spin = gtk_spin_button_new_with_range (0.0, 360.0, 1.0);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_azim_spin, 1, 2, 3, 4);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_azim_spin), isst.camera_az);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_azim_spin), 3);

	/* Elevation */
	label = gtk_label_new ("Elevation");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);

	isst_elev_spin = gtk_spin_button_new_with_range (-90.0, 90.0, 1.0);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_elev_spin, 1, 2, 4, 5);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_elev_spin), isst.camera_el);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_elev_spin), 3);

	/* Perspective */
	radio_perspective = gtk_radio_button_new_with_label (NULL, "Perspective");
	gtk_table_attach_defaults (GTK_TABLE (table), radio_perspective, 0, 1, 5, 6);

	fov_spin = gtk_spin_button_new_with_range (15.0, 60.0, 1.0);
	gtk_table_attach_defaults (GTK_TABLE (table), fov_spin, 1, 2, 5, 6);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (fov_spin), isst.camera_fov);

	/* Orthographic */
	radio_orthographic = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio_perspective), "Orthographic");
	gtk_table_attach_defaults (GTK_TABLE (table), radio_orthographic, 0, 1, 6, 7);

	isst_grid_spin = gtk_spin_button_new_with_range (0.1, 2000.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_grid_spin, 1, 2, 6, 7);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_grid_spin), isst.camera_grid);

	/* Mouse Speed */
	label = gtk_label_new ("Mouse Speed");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 7, 8);

	isst_mouse_speed_spin = gtk_spin_button_new_with_range (0.0001, 2.0, 0.001);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_mouse_speed_spin, 1, 2, 7, 8);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_mouse_speed_spin), isst.mouse_speed);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_mouse_speed_spin), 4);

	/* Update Button */ 
	update_button = gtk_button_new_with_label ("Update View");
	gtk_table_attach_defaults (GTK_TABLE (table), update_button, 1, 2, 8, 9);


	gwlist = wlist = (GtkWidget **) malloc (7 * sizeof (GtkWidget *));
	wlist[0] = isst_posx_spin;
	wlist[1] = isst_posy_spin;
	wlist[2] = isst_posz_spin;
	wlist[3] = isst_azim_spin;
	wlist[4] = isst_elev_spin;
	wlist[5] = radio_perspective;
	wlist[6] = fov_spin;

	g_signal_connect (G_OBJECT (update_button), "clicked", G_CALLBACK (update_view_callback), wlist);
    }


    {
	GtkWidget *component_tab, *component_label;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *alignment;
	GtkWidget *component_entry;
	GtkWidget *select_button;
	GtkWidget *deselect_button;

	component_tab = gtk_frame_new ("component");
	component_label = gtk_label_new ("component");
	isst.notebook_index[ISST_MODE_COMPONENT] = gtk_notebook_append_page (GTK_NOTEBOOK (isst_notebook), component_tab, component_label);

	alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (component_tab), alignment);

	/* Create a table for inputs */
	table = gtk_table_new (2, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), TABLE_BORDER_WIDTH);
	gtk_container_add (GTK_CONTAINER (alignment), table);

	/* Component name */
	label = gtk_label_new ("Component Name");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

	component_entry = gtk_entry_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), component_entry, 1, 2, 0, 1);

	deselect_button = gtk_button_new_with_label ("Deselect All");
	gtk_table_attach_defaults (GTK_TABLE (table), deselect_button, 0, 1, 1, 2);
	g_signal_connect (G_OBJECT (deselect_button), "clicked", G_CALLBACK (component_deselect_all_callback), NULL);

	select_button = gtk_button_new_with_label ("Select");
	gtk_table_attach_defaults (GTK_TABLE (table), select_button, 1, 2, 1, 2);
	g_signal_connect (G_OBJECT (select_button), "clicked", G_CALLBACK (component_select_callback), component_entry);
    }


    {
	GtkWidget *cut_tab, *cut_label;

	cut_tab = gtk_frame_new ("cut");
	cut_label = gtk_label_new ("cut");
	isst.notebook_index[ISST_MODE_CUT] = gtk_notebook_append_page (GTK_NOTEBOOK (isst_notebook), cut_tab, cut_label);
    }


    {
	GtkWidget *flos_tab, *flos_label;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *alignment;
	GtkWidget *current_button;

	flos_tab = gtk_frame_new ("fragment line of sight");
	flos_label = gtk_label_new ("fragment line of sight");
	isst.notebook_index[ISST_MODE_FLOS] = gtk_notebook_append_page (GTK_NOTEBOOK (isst_notebook), flos_tab, flos_label);

	alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (flos_tab), alignment);

	/* Create a table for inputs */
	table = gtk_table_new (4, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), TABLE_BORDER_WIDTH);
	gtk_container_add (GTK_CONTAINER (alignment), table);

	/* Position X */
	label = gtk_label_new ("Position X");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

	isst_flos_posx_spin = gtk_spin_button_new_with_range (-100.0, 100.0, 0.1);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_flos_posx_spin, 1, 2, 0, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_flos_posx_spin), 0.0);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_flos_posx_spin), 3);

	/* Position Y */
	label = gtk_label_new ("Position Y");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

	isst_flos_posy_spin = gtk_spin_button_new_with_range (-100.0, 100.0, 0.1);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_flos_posy_spin, 1, 2, 1, 2);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_flos_posy_spin), 0.0);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_flos_posy_spin), 3);

	/* Position Z */
	label = gtk_label_new ("Position Z");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);

	isst_flos_posz_spin = gtk_spin_button_new_with_range (-100.0, 100.0, 0.1);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_flos_posz_spin, 1, 2, 2, 3);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_flos_posz_spin), 0.0);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_flos_posz_spin), 3);

	/* Use current position button */
	current_button = gtk_button_new_with_label ("Use Current Position");
	gtk_table_attach_defaults (GTK_TABLE (table), current_button, 0, 2, 3, 4);
	g_signal_connect (G_OBJECT (current_button), "clicked", G_CALLBACK (flos_use_current_position_callback), NULL);
    }


    {
	GtkWidget *shotline_tab, *shotline_label;
	GtkWidget *alignment;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *apply_button;
	GtkWidget *save_button;
	GtkWidget *fire_button;

	shotline_tab = gtk_frame_new ("shotline");
	shotline_label = gtk_label_new ("shotline");
	isst.notebook_index[ISST_MODE_SHOTLINE] = gtk_notebook_append_page (GTK_NOTEBOOK (isst_notebook), shotline_tab, shotline_label);

	alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (shotline_tab), alignment);

	/* Create a table for inputs */
	table = gtk_table_new (4, 5, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), TABLE_BORDER_WIDTH);
	gtk_container_add (GTK_CONTAINER (alignment), table);

	/* Cell X */
	label = gtk_label_new ("Cell X");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

	isst_cellx_spin = gtk_spin_button_new_with_range (0, isst.context_width, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_cellx_spin, 1, 2, 0, 1);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_cellx_spin), 0);
	g_signal_connect (G_OBJECT (isst_cellx_spin), "value-changed", G_CALLBACK (update_cellx_callback), NULL);

	/* Cell Y */
	label = gtk_label_new ("Cell Y");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);

	isst_celly_spin = gtk_spin_button_new_with_range (0, isst.context_height, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_celly_spin, 3, 4, 0, 1);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_celly_spin), 0);
	g_signal_connect (G_OBJECT (isst_celly_spin), "value-changed", G_CALLBACK (update_celly_callback), NULL);

	/* Delta X */
	label = gtk_label_new ("Delta X");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

	isst_deltax_spin = gtk_spin_button_new_with_range (-1000, 1000, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_deltax_spin, 1, 2, 1, 2);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_deltax_spin), 0);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_deltax_spin), 0);

	/* Delta Y */
	label = gtk_label_new ("Delta Y");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 1, 2);

	isst_deltay_spin = gtk_spin_button_new_with_range (-1000, 1000, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), isst_deltay_spin, 3, 4, 1, 2);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (isst_deltay_spin), 0);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (isst_deltay_spin), 0);

	/* Apply Button */
	apply_button = gtk_button_new_with_label ("Apply Delta");
	gtk_table_attach_defaults (GTK_TABLE (table), apply_button, 4, 5, 1, 2);
	g_signal_connect (G_OBJECT (apply_button), "clicked", G_CALLBACK (apply_delta_callback), NULL);

	/* Name Entry */
	label = gtk_label_new ("Name");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);

	isst_name_entry = gtk_entry_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), isst_name_entry, 1, 4, 2, 3);

	/* Save Button */
	save_button = gtk_button_new_with_label ("Save Shotline");
	gtk_table_attach_defaults (GTK_TABLE (table), save_button, 4, 5, 2, 3);
	g_signal_connect (G_OBJECT (save_button), "clicked", G_CALLBACK (save_shotline_callback), NULL);

	/* In-Hit Info */
	label = gtk_label_new ("In-Hit");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);

	isst_inhit_entry = gtk_entry_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), isst_inhit_entry, 1, 4, 3, 4);
	gtk_entry_set_editable (GTK_ENTRY (isst_inhit_entry), FALSE);

	/* Fire Button */
	fire_button = gtk_button_new_with_label ("Fire Shotline");
	gtk_table_attach_defaults (GTK_TABLE (table), fire_button, 4, 5, 3, 4);
	g_signal_connect (G_OBJECT (fire_button), "clicked", G_CALLBACK (fire_shotline_callback), NULL);
    }

    /* Display everything and enter the event loop */
    gtk_widget_show_all (isst_window);
}


void
isst_setup ()
{
    /* User Settings */
    isst.uid = -1;
    isst.pid = -1;
    isst.wid = 0;

    /* Initial Mode */
    isst.mode = ISST_MODE_SHADED;
    isst.mode_updated = 1;

    /* Authentication Info */
    strcpy (isst.username, "");
    strcpy (isst.password, "");
    isst.connected = 0;

    /* Camera Info */
    isst.context_width = ISST_CONTEXT_W;
    isst.context_height = ISST_CONTEXT_H;
    VSET(isst.camera_pos,  10,  10,  10);
    isst.camera_az = 45;
    isst.camera_el = 35;
    isst.camera_type = RENDER_CAMERA_PERSPECTIVE;
    isst.camera_fov = 25;
    isst.camera_grid = 10;
    AZEL_TO_FOC ();

    /* Buffers */
    TIENET_BUFFER_INIT(isst.buffer);
    TIENET_BUFFER_INIT(isst.buffer_comp);
    TIENET_BUFFER_INIT(isst.buffer_image);
    TIENET_BUFFER_SIZE(isst.buffer_image, 3 * isst.context_width * isst.context_height);

    /* Input */
    isst.mouse_speed = 0.02;

    /* Misccelaneous */
    pthread_mutex_init (&isst.update_mut, 0);
    isst.update_avail = 0;
    isst.update_idle = 1;

    tie = (struct tie_s *)bu_malloc(sizeof(struct tie_s), "tie");
}

void
isst_free ()
{
}

void
isst_init (int argc, char **argv)
{
    FILE *sfh;	/* shot file handle (~/.isst) */
    char shotfile[BUFSIZ];
    GtkTreeIter iter;

    snprintf (shotfile, BUFSIZ, "%s/.isst", getenv("HOME"));
    isst_setup ();

    /* Start application with all flags off */
    isst_flags = 0;

    /* Initialize G-Threads */
    g_thread_init (NULL);
    gdk_threads_init ();

    /* Initialize GTK */
    gtk_init (&argc, &argv);

    argv[argc] = NULL;	/* make sure there's a terminator */

    isst_gui ();

    /* populate the saved shotline model from the ~/.isst file */
    isst_saved_shotline_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
    if((sfh = fopen(shotfile, "r")) != NULL) {
	char buf[BUFSIZ], shotname[BUFSIZ];
	while(fgets(buf, BUFSIZ-1, sfh)) {
	    if(*buf == 'S') {
		char *s1 = buf + 2, *s2 = shotname;
		while(*s1 != ' ')
		    *s2++ = *s1++;
		*s2 = 0;
#if 0
		/* chomp() */
		for(s1=buf;*s1=='\n';s1++);
		*s1 = '\0';
		s1 = buf;
#endif
		/* and save */
		gtk_list_store_append(isst_saved_shotline_store, &iter);
		gtk_list_store_set(isst_saved_shotline_store, &iter, 0, shotname, 1, buf, -1);
	    }
	}
	fclose(sfh);
    }

    GTK_WIDGET_UNSET_FLAGS (isst_context, GTK_DOUBLE_BUFFERED);
    gtk_widget_show_all (isst_window);

    if(argc>=2) {
	snprintf(filename, BUFSIZ, "%s", *argv);
	load_g_project_callback((const char *)*argv, (const char **)argv+1);
    }

    gtk_main ();
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
