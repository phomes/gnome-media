/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * callbacks.c
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtkbutton.h>
#include <gtk/gtkoptionmenu.h>
#include <glib/gerror.h>

#include <libgnome/gnome-i18n.h>

#include "cdrom.h"
#include "display.h"
#include "gnome-cd.h"

static void
maybe_close_tray (GnomeCD *gcd)
{
	GnomeCDRomStatus *status;
	GError *error;

	if (gnome_cdrom_get_status (gcd->cdrom, &status, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
		
		return;
	}

	if (status->cd == GNOME_CDROM_STATUS_TRAY_OPEN) {
		if (gnome_cdrom_close_tray (gcd->cdrom, &error) == FALSE) {
			g_warning ("%s: %s", __FUNCTION__, error->message);
			g_error_free (error);
		}
	}

	g_free (status);
	return;
}

void
eject_cb (GtkButton *button,
	  GnomeCD *gcd)
{
	GError *error;

	if (gnome_cdrom_eject (gcd->cdrom, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
	}

	if (gcd->current_image == gcd->pause_image) {
		AtkObject *aob;

		aob = gtk_widget_get_accessible (GTK_WIDGET (gcd->play_b));
		atk_object_set_name (aob, _("Play"));

		gtk_container_remove (GTK_CONTAINER (gcd->play_b), gcd->current_image);
		gtk_container_add (GTK_CONTAINER (gcd->play_b), gcd->play_image);
		gcd->current_image = gcd->play_image;
	}
}

void
play_cb (GtkButton *button,
	 GnomeCD *gcd)
{
	GError *error;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF msf;
	AtkObject *aob;

	if (gnome_cdrom_get_status (gcd->cdrom, &status, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
		return;

	}

	switch (status->cd) {
	case GNOME_CDROM_STATUS_TRAY_OPEN:
		if (gnome_cdrom_close_tray (gcd->cdrom, &error) == FALSE) {
			g_warning ("Cannot close tray: %s", error->message);
			g_error_free (error);

			g_free (status);
			return;
		}

		/* Tray is closed, now play */
		msf.minute = 0;
		msf.second = 0;
		msf.frame = 0;
		if (gnome_cdrom_play (gcd->cdrom, 1, &msf, &error) == FALSE) {
			g_warning ("%s: %s", __FUNCTION__, error->message);
			g_error_free (error);

			g_free (status);
			return;
		}
		break;

	case GNOME_CDROM_STATUS_DRIVE_NOT_READY:
		g_warning ("Drive not ready");

		g_free (status);
		return;

	case GNOME_CDROM_STATUS_OK:
	default:
		break;
	}

	switch (status->audio) {
	case GNOME_CDROM_AUDIO_PLAY:
		if (gnome_cdrom_pause (gcd->cdrom, &error) == FALSE) {
			g_warning ("%s: %s", __FUNCTION__, error->message);
			g_error_free (error);

			g_free (status);
			return;
		}

		if (gcd->current_image == gcd->pause_image) {
			aob = gtk_widget_get_accessible (GTK_WIDGET (gcd->play_b));
			atk_object_set_name (aob, _("Play"));
			gtk_container_remove (GTK_CONTAINER (gcd->play_b), gcd->current_image);
			gtk_container_add (GTK_CONTAINER (gcd->play_b), gcd->play_image);
			gcd->current_image = gcd->play_image;
		}
		break;

	case GNOME_CDROM_AUDIO_PAUSE:
		if (gnome_cdrom_play (gcd->cdrom, status->track,
				      &status->relative, &error) == FALSE) {
			g_warning ("%s: %s", __FUNCTION__, error->message);
			g_error_free (error);
			
			g_free (status);
			return;
		}

		if (gcd->current_image == gcd->play_image) {
			aob = gtk_widget_get_accessible (GTK_WIDGET (gcd->play_b));
			atk_object_set_name (aob, _("Pause"));
			gtk_container_remove (GTK_CONTAINER (gcd->play_b), gcd->current_image);
			gtk_container_add (GTK_CONTAINER (gcd->play_b), gcd->pause_image);
			gcd->current_image = gcd->pause_image;
		}
		break;

	case GNOME_CDROM_AUDIO_COMPLETE:
	case GNOME_CDROM_AUDIO_STOP:
		msf.minute = 0;
		msf.second = 0;
		msf.frame = 0;

		if (gnome_cdrom_play (gcd->cdrom, 1, &msf, &error) == FALSE) {
			g_warning ("%s: %s", __FUNCTION__, error->message);
			g_error_free (error);

			g_free (status);
			return;
		}

		if (gcd->current_image == gcd->play_image) {
			aob = gtk_widget_get_accessible (GTK_WIDGET (gcd->play_b));
			atk_object_set_name (aob, _("Pause"));
			gtk_container_remove (GTK_CONTAINER (gcd->play_b), gcd->current_image);
			gtk_container_add (GTK_CONTAINER (gcd->play_b), gcd->pause_image);
			gcd->current_image = gcd->pause_image;
		}
		break;

	case GNOME_CDROM_AUDIO_ERROR:
		g_warning ("Error playing CD");
		g_free (status);
		return;

	case GNOME_CDROM_AUDIO_NOTHING:
	default:
		break;
	}

	g_free (status);
}

void
stop_cb (GtkButton *button,
	 GnomeCD *gcd)
{
	GError *error;

	/* Close the tray if needed */
	maybe_close_tray (gcd);

	if (gnome_cdrom_stop (gcd->cdrom, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
	}

	if (gcd->current_image == gcd->pause_image) {
		AtkObject *aob;

		aob = gtk_widget_get_accessible (GTK_WIDGET (gcd->play_b));
		atk_object_set_name (aob, _("Play"));

		gtk_container_remove (GTK_CONTAINER (gcd->play_b), gcd->current_image);
		gtk_container_add (GTK_CONTAINER (gcd->play_b), gcd->play_image);
		gcd->current_image = gcd->play_image;
	}
}

static gboolean
ffwd_timeout_cb (gpointer data)
{
	GError *error;
	GnomeCD *gcd = data;

	if (gnome_cdrom_fast_forward (gcd->cdrom, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
		return FALSE;
	}

	return TRUE;
}

int
ffwd_press_cb (GtkButton *button,
	       GdkEvent *ev,
	       GnomeCD *gcd)
{
	maybe_close_tray (gcd);

	gcd->timeout = gtk_timeout_add (140, ffwd_timeout_cb, gcd);

	if (gcd->current_image == gcd->play_image) {
		AtkObject *aob;

		aob = gtk_widget_get_accessible (GTK_WIDGET (gcd->play_b));
		atk_object_set_name (aob, _("Pause"));

		gtk_container_remove (GTK_CONTAINER (gcd->play_b), gcd->current_image);
		gtk_container_add (GTK_CONTAINER (gcd->play_b), gcd->pause_image);
		gcd->current_image = gcd->pause_image;
	}

	return FALSE;
}

int
ffwd_release_cb (GtkButton *button,
		 GdkEvent *ev,
		 GnomeCD *gcd)
{
	if (gcd->timeout > 0) {
		gtk_timeout_remove (gcd->timeout);
	}

	return FALSE;
}

void
next_cb (GtkButton *button,
	 GnomeCD *gcd)
{
	GError *error;

	maybe_close_tray (gcd);

	if (gnome_cdrom_next (gcd->cdrom, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
	}

	if (gcd->current_image == gcd->play_image) {
		AtkObject *aob;

		aob = gtk_widget_get_accessible (GTK_WIDGET (gcd->play_b));
		atk_object_set_name (aob, _("Pause"));
		gtk_container_remove (GTK_CONTAINER (gcd->play_b), gcd->current_image);
		gtk_container_add (GTK_CONTAINER (gcd->play_b), gcd->pause_image);
		gcd->current_image = gcd->pause_image;
	}
}

void
back_cb (GtkButton *button,
	 GnomeCD *gcd)
{
	GError *error;

	maybe_close_tray (gcd);

	if (gnome_cdrom_back (gcd->cdrom, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
	}

	if (gcd->current_image == gcd->play_image) {
		AtkObject *aob;

		aob = gtk_widget_get_accessible (GTK_WIDGET (gcd->play_b));
		atk_object_set_name (aob, _("Pause"));
		gtk_container_remove (GTK_CONTAINER (gcd->play_b), gcd->current_image);
		gtk_container_add (GTK_CONTAINER (gcd->play_b), gcd->pause_image);
		gcd->current_image = gcd->pause_image;
	}
}

static gboolean
rewind_timeout_cb (gpointer data)
{
	GError *error;
	GnomeCD *gcd = data;
	
	if (gnome_cdrom_rewind (gcd->cdrom, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
		return FALSE;
	}
	
	return TRUE;
}

int
rewind_press_cb (GtkButton *button,
		 GdkEvent *ev,
		 GnomeCD *gcd)
{
	maybe_close_tray (gcd);

	gcd->timeout = gtk_timeout_add (140, rewind_timeout_cb, gcd);

	if (gcd->current_image == gcd->play_image) {
		AtkObject *aob;

		aob = gtk_widget_get_accessible (GTK_WIDGET (gcd->play_b));
		atk_object_set_name (aob, _("Pause"));
		gtk_container_remove (GTK_CONTAINER (gcd->play_b), gcd->current_image);
		gtk_container_add (GTK_CONTAINER (gcd->play_b), gcd->pause_image);
		gcd->current_image = gcd->pause_image;
	}

	return FALSE;
}

int
rewind_release_cb (GtkButton *button,
		   GdkEvent *ev,
		   GnomeCD *gcd)
{
	if (gcd->timeout > 0) {
		gtk_timeout_remove (gcd->timeout);
	}
	
	return FALSE;
}
	
void
mixer_cb (GtkButton *button,
	  GnomeCD *gcd)
{
	char *mixer_path;
	char *argv[2] = {NULL, NULL};
	GError *error = NULL;
	gboolean ret;

	/* Open the mixer */
	mixer_path = g_find_program_in_path ("gmix");

	argv[0] = mixer_path;
	ret = g_spawn_async (NULL, argv, NULL, 0, NULL, NULL, NULL, &error);
	if (ret == FALSE) {
		g_warning ("There was an error starting %s: %s", mixer_path,
			   error->message);
		g_error_free (error);
	}

	g_free (mixer_path);
}

void
cd_status_changed_cb (GnomeCDRom *cdrom,
		      GnomeCDRomStatus *status,
		      GnomeCD *gcd)
{
	char *text;
	int track;
	
	switch (status->cd) {
	case GNOME_CDROM_STATUS_OK:

		track = gtk_option_menu_get_history (GTK_OPTION_MENU (gcd->tracks));
		if (track + 1 != status->track) {
			gtk_option_menu_set_history (GTK_OPTION_MENU (gcd->tracks), status->track - 1);
		}
		
		if (gcd->last_status == NULL || gcd->last_status->cd != GNOME_CDROM_STATUS_OK) {
			cddb_get_query (gcd);
		}

		cd_display_clear (CD_DISPLAY (gcd->display));
		if (status->relative.second >= 10) {
			text = g_strdup_printf ("%d:%d", status->relative.minute, status->relative.second);
		} else {
			text = g_strdup_printf ("%d:0%d", status->relative.minute, status->relative.second);
		}

		cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_TIME, text);
		g_free (text);

		if (gcd->disc_info == NULL) {
#if 0
			text = g_strdup_printf ("Track %d", status->track);
			cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_TRACK, text);
			g_free (text);
#endif
			
			cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_ARTIST, "Unknown Artist");
			cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_ALBUM, "Unknown Album");
			gnome_cd_set_window_title (gcd, NULL, NULL);
		} else {
			GnomeCDDiscInfo *info = gcd->disc_info;

#if 0
			if (info->tracknames[status->track - 1] != NULL) {
				cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_TRACK, info->tracknames[status->track - 1]);
			}
#endif
			if (info->artist != NULL) {
				cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_ARTIST, info->artist);
			}
			if (info->title != NULL) {
				cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_ALBUM, info->title);
			}

			if (status->audio == GNOME_CDROM_AUDIO_PLAY ||
			    status->audio == GNOME_CDROM_AUDIO_PAUSE) {
				gnome_cd_set_window_title (gcd, info->artist, info->tracknames[status->track - 1]);
			} else {
				gnome_cd_set_window_title (gcd, info->artist, info->title);
			}
		}
		break;

		
	case GNOME_CDROM_STATUS_NO_DISC:
		cd_display_clear (CD_DISPLAY (gcd->display));
		cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_TIME, "No disc");
		gnome_cd_set_window_title (gcd, NULL, NULL);
		break;

	case GNOME_CDROM_STATUS_TRAY_OPEN:
		cd_display_clear (CD_DISPLAY (gcd->display));
		cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_TIME, "Drive open");
		gnome_cd_set_window_title (gcd, NULL, NULL);
		break;

	default:
		cd_display_clear (CD_DISPLAY (gcd->display));
		cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_TIME, "Drive Error");
		gnome_cd_set_window_title (gcd, NULL, NULL);
		break;
	}

#ifdef DEBUG
	g_print ("Status changed\n"
		 "Device: %d\nAudio %d\n"
		 "Track: %d (%d:%d:%d)\n"
		 "--------------------\n",
		 status->cd, status->audio,
		 status->track, status->relative.minute,
		 status->relative.second, status->relative.frame);
#endif
	if (gcd->last_status != NULL) {
		g_free (gcd->last_status);
	}

	gcd->last_status = gnome_cdrom_copy_status (status);
}
		 
