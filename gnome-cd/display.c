/*
 * display.c
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#include "gnome-cd.h"
#include "display.h"

#define X_OFFSET 2
#define Y_OFFSET 2

static char *default_text[NUMBER_OF_DISPLAY_LINES] = {
	"14:52",
	"---------------",
	"Godspeed You Black Emperor!",
	"Slow Riot For New Zer0 Kanada",
	"Blaize Bailey Finnigan 3"
};

void
display_change_line (GnomeCD *gcd,
		     int line,
		     const char *new_str)
{
	GnomeCDText *text;
	PangoRectangle rect;
	int height, max_width;

	text = gcd->layout[line];
	height = gcd->height - text->height;
	
	g_free (text->text);
	text->text = g_strdup (new_str);
	text->length = strlen (new_str);
	pango_layout_set_text (text->layout, text->text, text->length);
	pango_layout_get_extents (text->layout, NULL, &rect);
	text->height = rect.height / 1000;
	
	gcd->height = height + text->height;
	gcd->max_width = MAX (gcd->max_width, rect.width / 1000);

	gtk_widget_set_usize (gcd->display, gcd->max_width, gcd->height + (Y_OFFSET * 2));
}

void
display_size_allocate_cb (GtkWidget *drawing_area,
			  GtkAllocation *allocation,
			  GnomeCD *gcd)
{
	PangoDirection base_dir;
	int i;

	base_dir = pango_context_get_base_dir (gcd->pango_context);

	for (i = 0; i < NUMBER_OF_DISPLAY_LINES; i++) {
		PangoRectangle rect;

		pango_layout_set_alignment (gcd->layout[i]->layout,
					    base_dir == PANGO_DIRECTION_LTR ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);
		pango_layout_set_width (gcd->layout[i]->layout, gcd->display->allocation.width * 1000);
		pango_layout_get_extents (gcd->layout[i]->layout, NULL, &rect);
		gcd->layout[i]->height = rect.height / 1000;
	}
}

gboolean
display_expose_cb (GtkWidget *drawing_area,
		   GdkEventExpose *event,
		   GnomeCD *gcd)
{
	PangoDirection base_dir;
	GdkRectangle *area;
	int height = 0;
	int i;

	area = &event->area;

	gdk_draw_rectangle (drawing_area->window,
			    drawing_area->style->black_gc, TRUE,
			    area->x, area->y,
			    area->width, area->height);

	gdk_gc_set_clip_rectangle (drawing_area->style->text_gc[GTK_STATE_NORMAL], area);

	base_dir = pango_context_get_base_dir (gcd->pango_context);

	for (i = 0; i < NUMBER_OF_DISPLAY_LINES &&
		     height < area->y + area->height + Y_OFFSET; i++) {
		if (height + gcd->layout[i]->height >= Y_OFFSET + area->y) {
			pango_layout_set_alignment (gcd->layout[i]->layout,
						    base_dir == PANGO_DIRECTION_LTR ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);
			gdk_draw_layout_with_colors (drawing_area->window,
						     drawing_area->style->white_gc,
						     X_OFFSET, 
						     Y_OFFSET + height,
						     gcd->layout[i]->layout,
						     NULL, NULL);
		}
		height += gcd->layout[i]->height;
	}

	gdk_gc_set_clip_rectangle (drawing_area->style->text_gc[GTK_STATE_NORMAL], NULL);
}

int
display_update_cb (gpointer data)
{
	GnomeCDRomStatus *status;
	GnomeCD *gcd = data;
	GError *error;

	if (gnome_cdrom_get_status (gcd->cdrom, &status, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
		return TRUE;
	}

	switch (status->cd) {
	case GNOME_CDROM_STATUS_TRAY_OPEN:
		/* Do something to indicate the tray is open */

		g_free (status);
		return;

	case GNOME_CDROM_STATUS_DRIVE_NOT_READY:
		/* Do something to indicate drive not ready */

		g_free (status);
		return;

	case GNOME_CDROM_STATUS_OK:
	default:
		break;
	}

	switch (status->audio) {
	case GNOME_CDROM_AUDIO_NOTHING:
		break;

	case GNOME_CDROM_AUDIO_PLAY:
	{
		char *str;
		
		str = g_strdup_printf ("%d:%d", status->relative.minute,
				       status->relative.second);
		display_change_line (gcd, DISPLAY_LINE_TIME, str);
		g_free (str);

		str = g_strdup_printf ("Track %d", status->track);
		display_change_line (gcd, DISPLAY_LINE_TRACK, str);
		g_free (str);
		break;
	}

	case GNOME_CDROM_AUDIO_PAUSE:
		/* Flash display */
		break;

	case GNOME_CDROM_AUDIO_COMPLETE:
		/* ? */
		break;

	case GNOME_CDROM_AUDIO_STOP:
		break;

	case GNOME_CDROM_AUDIO_ERROR:
		break;

	default:
		break;
	}

	g_free (status);
	return TRUE;
}

void
display_setup_layout (GnomeCD *gcd)
{
	int i;
	
	for (i = 0; i < NUMBER_OF_DISPLAY_LINES; i++) {
		PangoRectangle rect;
		
		gcd->layout[i] = g_new (GnomeCDText, 1);
		
		gcd->layout[i]->text = g_strdup (default_text[i]);
		gcd->layout[i]->length = strlen (default_text[i]);
		gcd->layout[i]->layout = gtk_widget_create_pango_layout (gcd->display, gcd->layout[i]->text);
		pango_layout_set_text (gcd->layout[i]->layout,
				       gcd->layout[i]->text,
				       gcd->layout[i]->length);

		pango_layout_get_extents (gcd->layout[i]->layout, NULL, &rect);
		gcd->layout[i]->height = rect.height / 1000;

		gcd->max_width = MAX (gcd->max_width, rect.width / 1000);
		gcd->height += gcd->layout[i]->height;
	}

  	gtk_widget_set_usize (gcd->display, gcd->max_width, gcd->height + (Y_OFFSET * 2));
}

