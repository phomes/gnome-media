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
	int end_track;
	GnomeCDRomMSF *endmsf;

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
		if (gcd->cdrom->playmode == GNOME_CDROM_WHOLE_CD) {
			end_track = -1;
			endmsf = NULL;
		} else {
			end_track == 2;
			endmsf = &msf;
		}
		
		if (gnome_cdrom_play (gcd->cdrom, 1, &msf, end_track, endmsf, &error) == FALSE) {
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
		if (gcd->cdrom->playmode == GNOME_CDROM_WHOLE_CD) {
			end_track = -1;
			endmsf = NULL;
		} else {
			end_track = status->track + 1;
			msf.minute = 0;
			msf.second = 0;
			msf.frame = 0;
			endmsf = &msf;
		}
		if (gnome_cdrom_play (gcd->cdrom, status->track,
				      &status->relative, end_track, endmsf, &error) == FALSE) {
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

		if (gcd->cdrom->playmode == GNOME_CDROM_WHOLE_CD) {
			end_track = -1;
			endmsf = NULL;
		} else {
			end_track = 2;
			endmsf = &msf;
		}
		if (gnome_cdrom_play (gcd->cdrom, 1, &msf, end_track, endmsf, &error) == FALSE) {
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

	if (gcd->not_ready == TRUE) {
		return;
	}
	
	switch (status->cd) {
	case GNOME_CDROM_STATUS_OK:
		if (status->audio == GNOME_CDROM_AUDIO_COMPLETE) {
			if (cdrom->loopmode == GNOME_CDROM_LOOP) {
				if (cdrom->playmode == GNOME_CDROM_WHOLE_CD) {
					/* Around we go */
					play_cb (NULL, gcd);
				} else {
					GnomeCDRomMSF msf;
					int start_track, end_track;
					GError *error;

					/* CD has gone to the start of the next track.
					   Track we want to loop is track - 1 */
					start_track = status->track - 1;
					end_track = status->track;

					msf.minute = 0;
					msf.second = 0;
					msf.frame = 0;
					
					if (gnome_cdrom_play (gcd->cdrom, start_track, &msf,
							      end_track, &msf, &error) == FALSE) {
						g_warning ("%s: %s", __FUNCTION__, error->message);
						g_error_free (error);
						
						g_free (status);
						return;
					}
				}
			}
			break;
		}
		
		track = gtk_option_menu_get_history (GTK_OPTION_MENU (gcd->tracks));
		if (track + 1 != status->track) {
			g_signal_handlers_block_matched (G_OBJECT (gcd->tracks), G_SIGNAL_MATCH_FUNC,
							 0, 0, NULL, G_CALLBACK (skip_to_track), gcd);
			gtk_option_menu_set_history (GTK_OPTION_MENU (gcd->tracks), status->track - 1);
			g_signal_handlers_unblock_matched (G_OBJECT (gcd->tracks), G_SIGNAL_MATCH_FUNC,
							   0, 0, NULL, G_CALLBACK (skip_to_track), gcd);
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
			cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_ARTIST, "Unknown Artist");
			cd_display_set_line (CD_DISPLAY (gcd->display), CD_DISPLAY_LINE_ALBUM, "Unknown Album");
			gnome_cd_set_window_title (gcd, NULL, NULL);
		} else {
			GnomeCDDiscInfo *info = gcd->disc_info;

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

void
about_cb (GtkWidget *widget,
	  gpointer data)
{
	static GtkWidget *about = NULL;
	const char *authors[2] = {"Iain Holmes", NULL};
	
	if (about == NULL) {
		about = gnome_about_new ("Gnome CD", VERSION,
					 _("Copyright (C) 2001, 2002"),
					 _("A GNOME cd player"),
					 authors, NULL, NULL, NULL);
		g_signal_connect (G_OBJECT (about), "destroy",
				  G_CALLBACK (gtk_widget_destroyed), &about);
		gtk_widget_show (about);
	}
}

void
loopmode_changed_cb (GtkWidget *display,
		     GnomeCDRomMode mode,
		     GnomeCD *gcd)
{
	gcd->cdrom->loopmode = mode;
}

void
playmode_changed_cb (GtkWidget *display,
		     GnomeCDRomMode mode,
		     GnomeCD *gcd)
{
	gcd->cdrom->playmode = mode;
}

void
open_preferences (GtkWidget *widget,
		  GnomeCD *gcd)
{
	static GtkWidget *dialog = NULL;

	if (dialog == NULL) {
		dialog = preferences_dialog_show (gcd, FALSE);
		g_signal_connect (G_OBJECT (dialog), "destroy",
				  G_CALLBACK (gtk_widget_destroyed), &dialog);
	} else {
		gdk_window_show (dialog->window);
		gdk_window_raise (dialog->window);
	}
}
