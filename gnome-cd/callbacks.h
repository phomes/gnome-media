/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@ximian.com>
 *
 *  Copyright 2001 Iain Holmes
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include "gnome-cd.h"

void eject_cb (GtkButton *button, 
	       GnomeCD *cdrom);
void play_cb (GtkButton *button,
	      GnomeCD *cdrom);
void stop_cb (GtkButton *button,
	      GnomeCD *cdrom);
int ffwd_press_cb (GtkButton *button,
		   GdkEvent *ev,
		   GnomeCD *gcd);
int ffwd_release_cb (GtkButton *button,
		     GdkEvent *ev,
		     GnomeCD *gcd);
void next_cb (GtkButton *button,
	      GnomeCD *cdrom);
void back_cb (GtkButton *button,
	      GnomeCD *cdrom);
int rewind_press_cb (GtkButton *button,
		     GdkEvent *ev,
		     GnomeCD *gcd);
int rewind_release_cb (GtkButton *button,
		       GdkEvent *ev,
		       GnomeCD *gcd);
void mixer_cb (GtkButton *button,
	       GnomeCD *gcd);

void cd_status_changed_cb (GnomeCDRom *cdrom,
			   GnomeCDRomStatus *status,
			   GnomeCD *gcd);
void about_cb (GtkWidget *widget,
	       gpointer data);
void help_cb (GtkWidget *widget,
	      gpointer data);
		
void loopmode_changed_cb (GtkWidget *widget,
			  GnomeCDRomMode mode,
			  GnomeCD *gcd);
void remainingtime_mode_changed_cb (GtkWidget *widget,
		                    GnomeCDRomMode mode,
                		    GnomeCD *gcd);
void playmode_changed_cb (GtkWidget *widget,
			  GnomeCDRomMode mode,
			  GnomeCD *gcd);
void open_preferences (GtkWidget *widget,
		       GnomeCD *gcd);
void open_track_editor (GtkWidget *widget,
			GnomeCD *gcd);
void destroy_track_editor (void);
void volume_changed (GtkRange *range,
		     GnomeCD *gcd);
void position_changed (GtkRange *range,
		       GnomeCD *gcd);
void position_slider_enter (GtkRange *range, 
			    GdkEventCrossing *event,
			    GnomeCD *gcd);
void position_slider_leave (GtkRange *range, 
			    GdkEventCrossing *event,
			    GnomeCD *gcd);

void tray_popup_menu_cb (GtkStatusIcon *icon,
			 guint          button,
			 guint          activate_time,
			 GnomeCD       *gcd);
void tray_icon_activated (GtkStatusIcon *icon,
			  GnomeCD *gcd);

gboolean button_press_event_cb (GtkWidget *widget,
	                      GdkEventButton *event,
	                      GnomeCD *gcd);
gboolean popup_menu_cb (GtkWidget *widget,
			GnomeCD *gcd);

gboolean tray_icon_destroyed (GtkWidget *widget,
			      GnomeCD *gcd);

#endif
