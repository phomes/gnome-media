/*
 * gnome-cd.h
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifndef __GNOME_CD_H__
#define __GNOME_CD_H__

#include <gtk/gtkwidget.h>
#include <gtk/gtktooltips.h>

#include <pango/pango.h>

#include "cdrom.h"

#define NUMBER_OF_DISPLAY_LINES 5

typedef struct _GnomeCDText {
	char *text;
	int length;
	int height;
	PangoLayout *layout;
	GdkColor *foreground;
	GdkColor *background;
} GnomeCDText;

typedef struct _GnomeCD {
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *display;
	GtkTooltips *tooltips;

	GtkWidget *trackeditor_b, *properties_b;

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
} GnomeCD;

#endif
