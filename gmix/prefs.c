/*
 * GMIX 3.0
 *
 * Copyright (C) 1998 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
 * Config dialog added by Matt Martin <Matt.Martin@ieee.org>, Sept 1999
 * ALSA driver by Brian J. Murrell <gnome-alsa@interlinx.bc.ca> Dec 1999
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#ifdef ALSA
#include <sys/asoundlib.h>
#else
#ifdef HAVE_LINUX_SOUNDCARD_H 
#include <linux/soundcard.h>
#else 
#include <machine/soundcard.h>
#endif
#endif

#include <gnome.h>

#include "gmix.h"
#include "prefs.h"

/*
 * Gnome info:
 */
GtkWidget *configwin;


mixerprefs prefs={FALSE,FALSE,TRUE,TRUE};


static void retrieve_prefs(GtkWidget *page)
{
	mixerprefs *prefs_copy;

	prefs_copy = gtk_object_get_data(GTK_OBJECT(page), PREFS_COPY);
	prefs = *prefs_copy;
}

static void apply_cb(GtkWidget *widget, gint page_num, void *data)
{       
        GtkWidget *page;

	if (page_num != -1)
		return;

	/* Copy out the user's new preferences */
	page = gtk_object_get_data(GTK_OBJECT(configwin), PREFS_PAGE);
	retrieve_prefs(page);

        put_gui_config();

	gmix_build_slidernotebook();
}

static void destroy_prefs(GtkWidget *page)
{
	mixerprefs *prefs_copy;

	prefs_copy = gtk_object_get_data(GTK_OBJECT(page), PREFS_COPY);
	g_free(prefs_copy);
}

static void cancel_cb(GtkWidget *widget, void *data)
{       
        GtkWidget *page;

	/* Find and free the copy of the preferences data */
	page = gtk_object_get_data(GTK_OBJECT(configwin), PREFS_PAGE);
	destroy_prefs(page);

	gtk_widget_destroy (configwin);
        configwin=NULL;
}

static void bool_changed_cb(GtkWidget *widget, gboolean *data)
{
	if( *data )
		*data = FALSE;
	else        
		*data = TRUE;
	gnome_property_box_changed(GNOME_PROPERTY_BOX(configwin));
}

GtkWidget *optpage(void)
{
	GtkWidget *start_frame, *gui_frame;
	GtkWidget *ubervbox;
	GtkWidget *vbox, *init_start, *menu_hide, *temp;
	mixerprefs *prefs_copy;

	ubervbox = gtk_vbox_new(TRUE, 0);

	prefs_copy = g_malloc(sizeof(mixerprefs));
	g_assert(prefs_copy);
	*prefs_copy = prefs;
	gtk_object_set_data(GTK_OBJECT(ubervbox), PREFS_COPY, prefs_copy);

	start_frame = gtk_frame_new(_("On startup"));
	gtk_container_border_width(GTK_CONTAINER(start_frame), GNOME_PAD_SMALL);
    
	vbox = gtk_vbox_new(TRUE, 0);

	/* Set on start */
	init_start = gtk_check_button_new_with_label(_("Restore saved mixer levels on startup"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(init_start), \
				     prefs_copy->set_mixer_on_start);
	gtk_signal_connect(GTK_OBJECT(init_start), "clicked",
			   GTK_SIGNAL_FUNC(bool_changed_cb), &prefs_copy->set_mixer_on_start);

	menu_hide = gtk_check_button_new_with_label(_("Hide menu on start"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(menu_hide), \
				     prefs_copy->hide_menu);
	gtk_signal_connect(GTK_OBJECT(menu_hide), "clicked",
			   GTK_SIGNAL_FUNC(bool_changed_cb), &prefs_copy->hide_menu);

	gtk_box_pack_start_defaults(GTK_BOX(vbox), init_start);
	/*	gtk_box_pack_start_defaults(GTK_BOX(vbox), menu_hide);*/

	gtk_widget_show_all(vbox);


	gtk_container_add(GTK_CONTAINER(start_frame), vbox);
	gtk_container_add(GTK_CONTAINER(ubervbox), start_frame);

	gui_frame = gtk_frame_new(_("GUI"));
	gtk_container_border_width(GTK_CONTAINER(gui_frame), GNOME_PAD_SMALL);

	vbox = gtk_vbox_new(TRUE, 0);
	temp = gtk_check_button_new_with_label(_("Use mixer icons"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), \
				     prefs_copy->use_icons);
	gtk_signal_connect(GTK_OBJECT(temp), "clicked",
			   GTK_SIGNAL_FUNC(bool_changed_cb), &prefs_copy->use_icons);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), temp);
	
	
	temp = gtk_check_button_new_with_label(_("Use mixer labels"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), \
				     prefs_copy->use_labels);
	gtk_signal_connect(GTK_OBJECT(temp), "clicked",
			   GTK_SIGNAL_FUNC(bool_changed_cb), &prefs_copy->use_labels);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), temp);
	
	
	gtk_container_add(GTK_CONTAINER(gui_frame), vbox);
	
	gtk_container_add(GTK_CONTAINER(ubervbox), gui_frame);
	
	gtk_widget_show_all(ubervbox);

	return ubervbox;
}

void prefs_make_window(void)
{
        GtkWidget *label, *page;

	if (!configwin) {
	        configwin=gnome_property_box_new();
		gtk_widget_realize(configwin);
		label = gtk_label_new(_("Preferences"));
		page = optpage();
		gtk_object_set_data(GTK_OBJECT(configwin), PREFS_PAGE, page);
		gtk_notebook_append_page(
			GTK_NOTEBOOK(
			GNOME_PROPERTY_BOX(configwin)->notebook), page, label);
		
		gtk_signal_connect(GTK_OBJECT(configwin), "apply",
				   GTK_SIGNAL_FUNC(apply_cb), NULL);
		gtk_signal_connect(GTK_OBJECT(configwin), "destroy",
				   GTK_SIGNAL_FUNC(cancel_cb), NULL);
		gtk_signal_connect(GTK_OBJECT(configwin), "delete_event",
				   GTK_SIGNAL_FUNC(cancel_cb), NULL);
		gtk_signal_connect(GTK_OBJECT(configwin), "help",
				   GTK_SIGNAL_FUNC(help_cb), NULL);

		gtk_widget_show_all(configwin);	
    
	};
};

void get_gui_config(void)
{
	prefs.set_mixer_on_start=gnome_config_get_bool("/gmix/startup/init=true");
	prefs.hide_menu=gnome_config_get_bool("/gmix/gui/menu=false");
	prefs.use_icons=gnome_config_get_bool("/gmix/gui/icons=true");
	prefs.use_labels=gnome_config_get_bool("/gmix/gui/labels=true");
}

void put_gui_config(void)
{
	gnome_config_set_bool("/gmix/startup/init",prefs.set_mixer_on_start);
	gnome_config_set_bool("/gmix/gui/menu",prefs.hide_menu);
	gnome_config_set_bool("/gmix/gui/icons",prefs.use_icons);
	gnome_config_set_bool("/gmix/gui/labels",prefs.use_labels);
}

