/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@ximian.com>
 *
 *  Copyright 2001, 2002 Iain Holmes 
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

#ifndef __GNOME_CD_H__
#define __GNOME_CD_H__

#include <gtk/gtkwidget.h>
#include <gtk/gtkstatusicon.h>
#include <gtk/gtktooltips.h>

#include <pango/pango.h>

#include <cddb-slave-client.h>
#include <gconf/gconf-client.h>

#include "gnome-cd-type.h"
#include "cd-selection.h"
#include "preferences.h"
#include "cdrom.h"

#define NUMBER_OF_DISPLAY_LINES 5

/* Stock icons */
#define GNOME_CD_EJECT "media-eject"

typedef struct _GnomeCDDiscInfo {
	char *discid;
	char *title;
	char *artist;
	int ntracks;
	CDDBSlaveClientTrackInfo **track_info;
} GnomeCDDiscInfo;

typedef struct _GnomeCDText {
	char *text;
	int length;
	int height;
	PangoLayout *layout;
	GdkColor *foreground;
	GdkColor *background;
} GnomeCDText;

typedef struct _GCDTheme {
	char *name;
} GCDTheme;

struct _GnomeCD {
	GtkStatusIcon *tray;

	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *display;
	GtkObject *position_adj;
	GtkWidget *position_slider;
	GtkWidget *tracks;
	GtkWidget *menu;
	GtkWidget *slider;
	GtkTooltips *tooltips;
	GConfClient * client;

	GtkWidget *trackeditor_b, *properties_b;

	gboolean not_ready;
	/* FIXME: Make this a control */
	GtkWidget *back_b, *rewind_b;
	GtkWidget *play_b, *stop_b;
	GtkWidget *ffwd_b, *next_b;
	GtkWidget *eject_b;

	GtkWidget *play_image, *pause_image, *current_image;

	/* FIXME: Make this a control too */
	GtkWidget *mixer_b, *volume_b;

	GnomeCDRom *cdrom;

	GnomeCDRomStatus *last_status;

	guint32 timeout;
	guint32 display_timeout;

	int height, max_width;

	GnomeCDDiscInfo *disc_info;

	GCDTheme *theme;
	GnomeCDPreferences *preferences;
	CDSelection *cd_selection;

	/* Set if if --device was given on the command line */
	char *device_override;

	char *discid;	/* used to track which one we're looking up */
};

void skip_to_track (GtkWidget *item,
		    GnomeCD *gcd);
void gnome_cd_set_window_title (GnomeCD *gcd,
				const char *artist,
				const char *track);
void gnome_cd_build_track_list_menu (GnomeCD *gcd);

void gcd_warning (const char *message, GError *error);
void gcd_error (const char *message, GError *error);
void gcd_debug (const gchar *format, ...) G_GNUC_PRINTF (1, 2);

/* theme.c */
GCDTheme *theme_load (GnomeCD    *gcd,
		      const char *theme_name);
void theme_change_widgets (GnomeCD *gcd);
void theme_free (GCDTheme *theme);

void make_popup_menu (GnomeCD *gcd,
                GdkEventButton *event, gboolean iconify);

void tray_icon_create (GnomeCD *gcd);

#endif
