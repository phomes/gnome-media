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
#include <glib/gerror.h>

#include "cdrom.h"
#include "display.h"
#include "gnome-cd.h"

void
eject_cb (GtkButton *button,
	  GnomeCD *gcd)
{
	GError *error;

	if (gnome_cdrom_eject (gcd->cdrom, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
	}
}

void
play_cb (GtkButton *button,
	 GnomeCD *gcd)
{
	GError *error;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF msf;

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
		if (gnome_cdrom_play (gcd->cdrom, 0, &msf, &error) == FALSE) {
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

		break;

	case GNOME_CDROM_AUDIO_PAUSE:
		if (gnome_cdrom_play (gcd->cdrom, status->track,
				      &status->relative, &error) == FALSE) {
			g_warning ("%s: %s", __FUNCTION__, error->message);
			g_error_free (error);
			
			g_free (status);
			return;
		}

		break;

	case GNOME_CDROM_AUDIO_COMPLETE:
	case GNOME_CDROM_AUDIO_STOP:
		msf.minute = 0;
		msf.second = 0;
		msf.frame = 0;

		if (gnome_cdrom_play (gcd->cdrom, 0, &msf, &error) == FALSE) {
			g_warning ("%s: %s", __FUNCTION__, error->message);
			g_error_free (error);

			g_free (status);
			return;
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

	if (gnome_cdrom_stop (gcd->cdrom, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
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
	gcd->timeout = gtk_timeout_add (140, ffwd_timeout_cb, gcd);
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

	if (gnome_cdrom_next (gcd->cdrom, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
	}
}

void
back_cb (GtkButton *button,
	 GnomeCD *gcd)
{
	GError *error;

	if (gnome_cdrom_back (gcd->cdrom, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);
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
	gcd->timeout = gtk_timeout_add (140, rewind_timeout_cb, gcd);
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

int
update_cb (gpointer data)
{
	GnomeCDRomStatus *status;
	GnomeCD *gcd = data;
	GError *error;

	if (gnome_cdrom_get_status (gcd->cdrom, &status, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_warning ("No disc");
		g_error_free (error);
		return TRUE;
	}

	switch (status->cd) {
	case GNOME_CDROM_STATUS_TRAY_OPEN:
		g_warning ("Tray open");

		g_free (status);
		return TRUE;

	case GNOME_CDROM_STATUS_DRIVE_NOT_READY:
		g_warning ("Drive not ready");

		g_free (status);
		return TRUE;

	case GNOME_CDROM_STATUS_NO_DISC:
		g_warning ("No disc");

		return TRUE;

	case GNOME_CDROM_STATUS_OK:
		g_warning ("Drive OK");
		
		break;

	default:
		g_warning ("Unknown status: %d", status->cd);
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
		cd_display_set_line (CD_DISPLAY (gcd->display), 
				     CD_DISPLAY_LINE_TIME, str);
		g_free (str);

		str = g_strdup_printf ("Track %d", status->track);
		cd_display_set_line (CD_DISPLAY (gcd->display),
				     CD_DISPLAY_LINE_TRACK, str);
		g_free (str);
		break;
	}

	case GNOME_CDROM_AUDIO_PAUSE:

		break;

	case GNOME_CDROM_AUDIO_COMPLETE:
		
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
