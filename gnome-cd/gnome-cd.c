/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * gnome-cd.c: GNOME-CD player.
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <gnome.h>
#include <bonobo.h>

#include "gnome-cd.h"
#include "cddb.h"
#include "cdrom.h"
#include "display.h"
#include "callbacks.h"
#include "preferences.h"
#include "access/factory.h"

#ifdef HAVE_GST
#include <gst/gst.h>
#endif

#define DEFAULT_THEME "lcd"
#define MAX_TRACKNAME_LENGTH 30

/* Debugging? */
gboolean debug_mode = FALSE;
static char *cd_option_device = NULL;

static gboolean cd_option_unique = FALSE;
static gboolean cd_option_play = FALSE;
static gboolean cd_option_tray = FALSE;

/* Popup Menu*/
static GtkWidget *popup_menu = NULL;

/* Gconf ID */
static guint volume_notify_id; 

/* if env var GNOME_CD_DEBUG is set,
 * g_warning the given message, and if there was an error, the error message
 */
void
gcd_warning (const char *message,
	     GError *error)
{
	if (debug_mode == FALSE) {
		return;
	}
	
	g_warning (message, error ? error->message : "(None)");
}

void
gcd_error (const gchar *message,
	   GError *error)
{
	GtkWidget *dlg;

	gcd_warning (message, error);

	dlg = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
				      GTK_BUTTONS_CLOSE, message,
				      error ? error->message : "(None)");
	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
}

/* if env var GNOME_CD_DEBUG is set, g_print the given message */
void
gcd_debug (const gchar *format,
           ...)
{
	va_list args;
	gchar *string;

	if (debug_mode == FALSE) {
		return;
	}
	va_start (args, format);
	string = g_strdup_vprintf (format, args);
	va_end (args);

	g_print ("DEBUG: gnome-cd: %s\n", string);
	g_free (string);
}

/*
 * Sets the window title based on the passed artist and track name.
 * If artist and track is NULL, then "CD Player" is displayed.
 * If either one is NULL, display the other.
 * If neither are NULL, display "(artist) - (title)"
 *
 * FIXME: it might be nice to change this function so that it takes
 * artist, track, album, status, so we can decide better what to display.
 * Right now, sometimes this is called with the disc title instead of
 * the track title */
void
gnome_cd_set_window_title (GnomeCD *gcd,
			   const char *artist,
			   const char *track)
{
	char *title;
	const char *old_title;

	if (artist == NULL && track == NULL) {
		title = g_strdup (_("CD Player"));
	} else if (artist == NULL) {
		title = g_strdup (track);
	} else if (track == NULL) {
		title = g_strdup (artist);
	} else {
		title = g_strconcat (artist, " - ", track, NULL);
	}
	/*
	 * Call gtk_window_set_title only if the title has changed
	 */
	old_title = gtk_window_get_title (GTK_WINDOW (gcd->window));
	if (!old_title || strcmp (title, old_title))
		gtk_window_set_title (GTK_WINDOW (gcd->window), title);

	g_free (title);
}

/* Can't be static because it ends up being referenced from another file */
void
skip_to_track (GtkWidget *item,
	       GnomeCD *gcd)
{
	int track, end_track;
	GnomeCDRomStatus *status = NULL;
	GnomeCDRomMSF msf, *endmsf;
	GError *error = NULL;
	
	track = gtk_option_menu_get_history (GTK_OPTION_MENU (gcd->tracks));

	if (gnome_cdrom_get_status (GNOME_CDROM (gcd->cdrom), &status, NULL) == FALSE) {
		g_free (status);
		return;
	}

	if (status->track - 1 == track && status->audio == GNOME_CDROM_AUDIO_PLAY) {
		g_free (status);
		return;
	}

	g_free (status);
	msf.minute = 0;
	msf.second = 0;
	msf.frame = 0;

	if (gcd->cdrom->playmode == GNOME_CDROM_SINGLE_TRACK) {
		if (gcd->disc_info && (track + 1) >= gcd->disc_info->ntracks) {
			end_track = -1;
			endmsf = NULL;
		} else {	
			end_track = track + 2;
			endmsf = &msf;
		}
	} else {
		end_track = -1;
		endmsf = NULL;
	}
	
	if (gnome_cdrom_play (GNOME_CDROM (gcd->cdrom), track + 1, &msf, end_track, endmsf, &error) == FALSE) {
		gcd_warning ("Error skipping %s", error);
		g_error_free (error);
	}
}

void
gnome_cd_build_track_list_menu (GnomeCD *gcd)
{
	GtkMenu *menu;
	GtkWidget *item;

	/* Block the changed signal */
	g_signal_handlers_block_matched (G_OBJECT (gcd->tracks), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
					 G_CALLBACK (skip_to_track), gcd);
	if (gcd->menu != NULL) {
		menu = GTK_MENU (gcd->menu);
		gtk_option_menu_remove_menu (GTK_OPTION_MENU (gcd->tracks));
	}

	menu = GTK_MENU(gtk_menu_new ());
	if (gcd->disc_info != NULL &&
	    gcd->disc_info->track_info) {
		int i;
		gchar *tmp;
		for (i = 0; i < gcd->disc_info->ntracks; i++) {
			char *title;
			CDDBSlaveClientTrackInfo *info;

			info = gcd->disc_info->track_info[i];
			/* If statement 'returns' item, which is a menu item
			   with all the info on the track that we have.  It will
			   also truncate the name if it's too long. */
			if (info == NULL) {
				title = g_strdup_printf ("%d - %s", i + 1,
							 _("Unknown track"));

				item = gtk_menu_item_new_with_label (title);
				g_free (title);
			} else {
				if (strlen(info->name) > MAX_TRACKNAME_LENGTH) {
					tmp = g_strndup (info->name, 
							 MAX_TRACKNAME_LENGTH);
					title = g_strdup_printf ("%d - %s...", 
								 i + 1, tmp);
					g_free (tmp);
				} else {
					title = g_strdup_printf ("%d - %s ", 
								 i + 1, info->name);
				}

				item = gtk_menu_item_new_with_label (title);
				gtk_tooltips_set_tip (gcd->tooltips, item, info->name, NULL);
				g_free (title);
			}
			
			gtk_widget_show (item);

			gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);
		}
	} else {
		GnomeCDRomCDDBData *data;
		GnomeCDRomStatus *status;

		if (gnome_cdrom_get_status (gcd->cdrom, &status, NULL) != FALSE) {

			if (status->cd == GNOME_CDROM_STATUS_OK) {
				if (gnome_cdrom_get_cddb_data (gcd->cdrom, &data, NULL) != FALSE) {
					int i;

					if (data != NULL) {
						for (i = 0; i < data->ntrks; i++) {
							char *label;
							
							label = g_strdup_printf (_("%d - Unknown"), i + 1);
							item = gtk_menu_item_new_with_label (label);
							g_free (label);
							
							gtk_widget_show (item);
							
							gtk_menu_shell_append ((GtkMenuShell*)(menu), item);
						}
						
						g_free (data);
					}
				}
			}

		}
		g_free (status);
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (gcd->tracks), GTK_WIDGET (menu));
	gcd->menu = GTK_WIDGET(menu);

	g_signal_handlers_unblock_matched (G_OBJECT (gcd->tracks), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
					   G_CALLBACK (skip_to_track), gcd);
}

static GtkWidget *
make_button_from_widget (GnomeCD *gcd,
			 GtkWidget *widget,
			 GCallback func,
			 const char *tooltip,
			 const char *shortname)
{
	GtkWidget *button;
	AtkObject *aob;

	button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (button), widget);
/*  	gtk_container_set_border_width (GTK_CONTAINER (button), 2); */
	
	if (func) {
		g_signal_connect (G_OBJECT (button), "clicked",
				  G_CALLBACK (func), gcd);
	}
	gtk_tooltips_set_tip (gcd->tooltips, button, tooltip, "");

	aob = gtk_widget_get_accessible (button);
	atk_object_set_name (aob, shortname);
	return button;
}

static GtkWidget *
make_button_from_stock (GnomeCD *gcd,
			const char *stock,
			GCallback func,
			const char *tooltip, 
			const char *shortname)
{
	GtkWidget *pixmap;

	pixmap = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_BUTTON);

	return make_button_from_widget (gcd, pixmap, func, tooltip, shortname);
}

static GdkPixbuf *
pixbuf_from_file (const char *filename)
{
	GdkPixbuf *pixbuf;
	char *fullname;

	fullname = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, 
                   filename, TRUE, NULL);
	if (fullname == NULL) {
		/* If the elegant way doesn't work, try and brute force it */
		fullname = g_strconcat(GNOME_ICONDIR, "/", filename, NULL);
	}

	pixbuf = gdk_pixbuf_new_from_file (fullname, NULL);
	g_free (fullname);

	return pixbuf;
}

static void
window_destroy_cb (GtkWidget *window,
		   gpointer data)
{
	GnomeCD *gcd = data;

	/* Make sure we stop playing the cdrom first */
	gnome_cdrom_stop(gcd->cdrom, NULL);

	/* Before killing the cdrom object, do the shutdown on it */
	if (gcd->preferences->stop_eject) {
		gnome_cdrom_eject (gcd->cdrom, NULL);
	}

	/* Unref the cddb slave */
	cddb_close_client ();

	/* And the track editor */
	destroy_track_editor ();
	
	/* Remove the gconf volume notify*/
	gconf_client_notify_remove (gcd->client, volume_notify_id);
	g_object_unref (gcd->cdrom);
	bonobo_main_quit ();
}

struct _MenuItem {
	char *name;
	char *icon;
	GCallback callback;
};

struct _MenuItem menuitems[] = {
	{N_("_Play / Pause"), GTK_STOCK_MEDIA_PLAY, G_CALLBACK (play_cb)},
	{N_("_Stop"), GTK_STOCK_MEDIA_STOP, G_CALLBACK (stop_cb)},
	{N_("P_revious"), GTK_STOCK_MEDIA_PREVIOUS, G_CALLBACK (back_cb)},
	{N_("_Next"), GTK_STOCK_MEDIA_NEXT, G_CALLBACK (next_cb)},
	{N_("_Eject disc"), GNOME_CD_EJECT, G_CALLBACK (eject_cb)},
	{N_("_Help"), GTK_STOCK_HELP, G_CALLBACK (help_cb)},
	{N_("_About"), GTK_STOCK_ABOUT, G_CALLBACK (about_cb)},
	{N_("_Quit"), GTK_STOCK_QUIT, G_CALLBACK (window_destroy_cb)},
	{NULL, NULL, NULL}
};

static void
popup_menu_detach (GtkWidget *attach_widget,
                GtkMenu *menu)
{
	popup_menu = NULL;
}

void
make_popup_menu (GnomeCD *gcd, GdkEventButton *event, gboolean iconify)
{
	int i, no_of_menu_items;
	GnomeCDRomStatus *status = NULL;

	if (gnome_cdrom_get_status (GNOME_CDROM (gcd->cdrom), &status, NULL) == FALSE) {
	}
	
	if (!GTK_WIDGET_REALIZED (gcd->display)) {
		if (!iconify)
		       	goto cleanup;
    	}

	if (popup_menu) 
       	gtk_widget_destroy (popup_menu);
	
	popup_menu = gtk_menu_new ();
	gtk_menu_attach_to_widget (GTK_MENU (popup_menu),
                               GTK_WIDGET (gcd->display),
                               popup_menu_detach);
	if (iconify == TRUE)
		no_of_menu_items = 6;
	else
		no_of_menu_items = 7;
	
	for (i = 0; i <= no_of_menu_items; i++) {
		GtkWidget *item, *image;
		gchar *icon_name;

		/* Add Quit menu item if iconify popup */
		if (i == 6 && iconify)
			i = 7;
		
		item = gtk_image_menu_item_new_with_mnemonic (_(menuitems[i].name));
	        
		/* If status is play, display the pause icon else display the play icon */
        	if (status && status->audio == GNOME_CDROM_AUDIO_PLAY && i == 0) 
	        	icon_name = GTK_STOCK_MEDIA_PAUSE;
		else 
		        icon_name = menuitems[i].icon;
		
		if (icon_name != NULL) {
	        	char *ext = strrchr (icon_name, '.');

		        if (ext == NULL) 
				image = gtk_image_new_from_stock (icon_name, GTK_ICON_SIZE_MENU);
		        else {
				char *fullname;
				fullname = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, 
				                                      icon_name, TRUE, NULL);
				if (fullname != NULL) 
					image = gtk_image_new_from_file (fullname);
				else 
       					image = NULL;
        	       	       
				g_free (fullname);
		     	}
        	}
		
		if (image != NULL) {
			gtk_widget_show (image);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), image);
		}

		if (status && status->cd == GNOME_CDROM_STATUS_OK) {
			/* Previous menuitem is sensitive when it is
			 * not the first track and we're playing */
	        	if (i == 2) {
	                	if (status->track <= 1 ||
				   (status->audio == GNOME_CDROM_AUDIO_STOP ||
				    status->audio == GNOME_CDROM_AUDIO_COMPLETE)) 
					gtk_widget_set_sensitive (item, FALSE);
				else 
					gtk_widget_set_sensitive (item, TRUE);
	            	}

		        /* Next menuitem is sensitive when this is not
			 * the last track */
			if (i == 3) {
				if (gcd->disc_info && status->track >= gcd->disc_info->ntracks) 
					gtk_widget_set_sensitive (item, FALSE);
				else
					gtk_widget_set_sensitive (item, TRUE);
	                }
	        }
		
		/* Disable the first four menu items when no disc is inserted
		 * Update as per the buttons on the main window
		 */
		if ((status == NULL || status->cd == GNOME_CDROM_STATUS_NO_DISC || status->cd == GNOME_CDROM_STATUS_EMPTY_DISC) && i < 4) 
			gtk_widget_set_sensitive (item, FALSE);
        
		gtk_widget_show (item);

		gtk_menu_shell_append ((GtkMenuShell *)(popup_menu), item);
		if (i == 4) {
			GtkWidget *s = gtk_separator_menu_item_new ();
			gtk_menu_shell_append ((GtkMenuShell *)(popup_menu), s);
			gtk_widget_show (s);
		}

		if (menuitems[i].callback != NULL) 
			g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menuitems[i].callback), gcd);
		
	}
	
	if (event != NULL) {
	   	gtk_menu_popup (GTK_MENU (popup_menu),
	    	                NULL, NULL,
	    	                NULL, NULL,
	    	                event->button,
	    	                event->time);
   	} else {
		gtk_menu_popup (GTK_MENU (popup_menu),
				NULL, NULL,
				NULL, NULL,
				0, gtk_get_current_event_time ());
    	}
cleanup:
        g_free (status);
}

static void
set_volume (GnomeCD *gcd)
{
	GError *error = NULL;
	GtkTooltips *volume_level_tooltip;
	gchar *volume_level;
	gint scaled_volume;
	double volume;
	
	scaled_volume = gconf_client_get_int (gcd->client, "/apps/gnome-cd/volume", NULL);
	/* On error set the Key */	
	if (scaled_volume < 0) {
		scaled_volume = 0;
		gconf_client_set_int (gcd->client, "/apps/gnome-cd/volume", 0, NULL);
	}	
	else if (scaled_volume > 100) {
		scaled_volume = 100;
		gconf_client_set_int (gcd->client, "/apps/gnome-cd/volume", 100, NULL);
	}
	volume = ((double)scaled_volume/100)*255;
        
	if (gnome_cdrom_set_volume (gcd->cdrom, (int) volume, &error) == FALSE) {
		gcd_warning ("Error setting volume: %s", error);
		g_error_free (error);
	}
	
	/* Set the value in the slider */	
	gtk_range_set_value (GTK_RANGE (gcd->slider), volume);
	/* Set the tooltip */
	volume_level = g_strdup_printf (_("Volume %d%%"), scaled_volume);
	gtk_tooltips_set_tip (gcd->tooltips, GTK_WIDGET (gcd->slider), volume_level, NULL);
	g_free (volume_level);
}

static void
volume_changed_cb (GConfClient *_client,
		guint cnxn_id,
		GConfEntry *entry,
		gpointer user_data)
{
	GnomeCD *gcd = user_data;
	set_volume (gcd);
}

static void
show_error (GtkWidget	*dialog,
	    GError 	*error,
	    GError	*detailed_error,
	    GnomeCD 	*gcd)
{
	switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
	case 1:
		gtk_label_set_text (GTK_LABEL (GTK_MESSAGE_DIALOG (dialog)->label),
				    detailed_error->message);

		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 1, FALSE);
		show_error (dialog, error, detailed_error, gcd);

		break;

	case 2:
		gtk_widget_destroy (dialog);
		dialog = GTK_WIDGET(preferences_dialog_show (gcd));

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		break;

	default:
		exit (0);
	}
}

static GnomeCD *
init_player (const char *device_override)
{
	GnomeCD *gcd;
	GtkWidget *display_box;
	GtkWidget *top_hbox, *button_hbox, *option_hbox;
	GtkWidget *button;
	GdkPixbuf *pixbuf;
	GtkWidget *box;
	GError *error = NULL;
	GError *detailed_error = NULL;

	gcd = g_new0 (GnomeCD, 1);

	gcd->not_ready = TRUE;
	gcd->client = gconf_client_get_default ();
	gcd->preferences = (GnomeCDPreferences *)preferences_new (gcd);
	gcd->device_override = g_strdup (device_override);
	
	if (gcd->preferences->device == NULL || gcd->preferences->device[0] == 0) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_NONE,
						 _("There is no CD device set. This means that the CD player\n"
						   "will be unable to run. Click 'Set device' to go to a dialog\n"
						   "where you can set the device, or click 'Quit' to quit the CD player."));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_QUIT, GTK_RESPONSE_CLOSE,
					_("Set device"), 1, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), 1);
		gtk_window_set_title (GTK_WINDOW (dialog), _("No CD device"));
		
		switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
		case 1:
			gtk_widget_destroy (dialog);
			dialog = GTK_WIDGET(preferences_dialog_show (gcd));

			/* Don't care what it returns */
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);

			break;

		default:
			exit (0);
		}
	}
 nodevice:		
	gcd->cdrom = gnome_cdrom_new (gcd->device_override ? gcd->device_override : gcd->preferences->device, 
				      GNOME_CDROM_UPDATE_CONTINOUS, &error);

	if (gcd->cdrom == NULL) {

		if (error != NULL) {
			gcd_warning ("%s", error);
		}
		if (gcd->device_override) {
			g_free (gcd->device_override);
			gcd->device_override = NULL;

			g_error_free (error);
			error = NULL;

			goto nodevice;
		}
	} else {
		g_signal_connect (G_OBJECT (gcd->cdrom), "status-changed",
				  G_CALLBACK (cd_status_changed_cb), gcd);
	}

	if (error != NULL) {
		GtkWidget *dialog;

		detailed_error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_OPENED,
					      "The CD player is unable to run correctly.\n\n"
					      "%s\n"
					      "Press 'Set device' to go to a dialog"
					      " where you can set the device, or press 'Quit' to quit the CD player", error->message ? error->message : " ");

		
		dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_NONE,
						 _("The CD player is unable to run correctly.\n\n"
						 "Press 'Details' for more details on reasons for the failure.\n\n" 
						 "Press 'Set device' to go to a dialog"
						 " where you can set the device, or press 'Quit' to quit the CD player"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), _("_Details"), 1, GTK_STOCK_QUIT, GTK_RESPONSE_CLOSE,
					_("_Set device"), 2, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), 2);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Invalid CD device"));
		show_error (dialog, error, detailed_error, gcd);

		g_error_free (error);
		g_error_free (detailed_error);
		error = NULL;
		detailed_error = NULL;

		goto nodevice;
	}
		
	gcd->cd_selection = cd_selection_start (gcd->device_override ?
						gcd->device_override : gcd->preferences->device);
	gcd->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (gcd->window), _("CD Player"));
	gtk_window_set_role (GTK_WINDOW (gcd->window), "gnome-cd-main-window");
	gtk_window_set_default_size (GTK_WINDOW (gcd->window), 350, 129);
	
	pixbuf = pixbuf_from_file ("gnome-cd/cd.png");
	if (pixbuf == NULL) {
		g_warning ("Error finding gnome-cd/cd.png");
	} else {
		gtk_window_set_icon (GTK_WINDOW (gcd->window), pixbuf);
		g_object_unref (G_OBJECT (pixbuf));
	}

	g_signal_connect (G_OBJECT (gcd->window), "destroy",
			  G_CALLBACK (window_destroy_cb), gcd);
	gcd->vbox = gtk_vbox_new (FALSE, 1);
	gcd->tooltips = gtk_tooltips_new ();
	top_hbox = gtk_hbox_new (FALSE, 1);

	/* Create the display */
	display_box = gtk_vbox_new (FALSE, 1);

	gcd->display = GTK_WIDGET (cd_display_new ());
	GTK_WIDGET_SET_FLAGS (gcd->display, GTK_CAN_FOCUS | GTK_CAN_DEFAULT);
	
	gtk_widget_add_events (gcd->display, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK); 
	
	g_signal_connect (G_OBJECT (gcd->display), "loopmode-changed",
			  G_CALLBACK (loopmode_changed_cb), gcd);
	g_signal_connect (G_OBJECT (gcd->display), "playmode-changed",
			  G_CALLBACK (playmode_changed_cb), gcd);
	g_signal_connect (G_OBJECT (gcd->display), "style_set",
			  G_CALLBACK (cd_display_set_style), NULL);
	g_signal_connect (G_OBJECT (gcd->display), "remainingtime-mode-changed",
			  G_CALLBACK (remainingtime_mode_changed_cb), gcd);
	/* Theme needs to be loaded after the display is created */
	gcd->theme = (GCDTheme *)theme_load (gcd, gcd->preferences->theme_name);
	if (gcd->theme == NULL) {
		g_error ("Could not create theme");
	}
	
	g_signal_connect (G_OBJECT (gcd->display), "popup_menu",
			  G_CALLBACK (popup_menu_cb), gcd);
	g_signal_connect (G_OBJECT (gcd->display), "button_press_event",
	          G_CALLBACK (button_press_event_cb), gcd);
	
	gtk_box_pack_start (GTK_BOX (display_box), gcd->display, TRUE, TRUE, 0);


	gtk_box_pack_start (GTK_BOX (top_hbox), display_box, TRUE, TRUE, 0);

	/* Volume slider */
	gcd->slider = gtk_vscale_new_with_range (0.0, 255.0, 1.0);
	gtk_scale_set_draw_value (GTK_SCALE (gcd->slider), FALSE);
	gtk_range_set_inverted(GTK_RANGE(gcd->slider), TRUE);
	gtk_box_pack_start (GTK_BOX (top_hbox), gcd->slider, FALSE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (gcd->vbox), top_hbox, TRUE, TRUE, 0);
	
	
	/* Position slider */
	gcd->position_adj = gtk_adjustment_new (0.0, 0.0, 100.0,
						1.0, 1.0, 1.0);
	gcd->position_slider = gtk_hscale_new (GTK_ADJUSTMENT (gcd->position_adj));
	gtk_scale_set_draw_value (GTK_SCALE (gcd->position_slider), FALSE);
	gtk_tooltips_set_tip (gcd->tooltips, GTK_WIDGET (gcd->position_slider),
			      _("Position"), NULL);
	gtk_range_set_update_policy (GTK_RANGE(gcd->position_slider), 
					       GTK_UPDATE_DISCONTINUOUS);
	gtk_box_pack_start (GTK_BOX (gcd->vbox), gcd->position_slider, 
			    FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (gcd->position_slider), "value-changed",
			  G_CALLBACK (position_changed), gcd);
	g_signal_connect (G_OBJECT (gcd->position_slider), "enter-notify-event",
			  G_CALLBACK (position_slider_enter), gcd);
	g_signal_connect (G_OBJECT (gcd->position_slider), "leave-notify-event",
			  G_CALLBACK (position_slider_leave), gcd);
	
	option_hbox = gtk_hbox_new (FALSE, 2);

	/* Create app controls */
	button = make_button_from_stock (gcd, GTK_STOCK_PREFERENCES,
					 G_CALLBACK(open_preferences),
					 _("Open preferences"),
					 _("Preferences"));
	gtk_box_pack_start (GTK_BOX (option_hbox), button, FALSE, FALSE, 0);
	gcd->properties_b = button;

	gcd->tracks = gtk_option_menu_new ();
	g_signal_connect (G_OBJECT (gcd->tracks), "changed",
			  G_CALLBACK (skip_to_track), gcd);
	gtk_tooltips_set_tip (gcd->tooltips, GTK_WIDGET (gcd->tracks), 
			      _("Track List"), NULL );
	gnome_cd_build_track_list_menu (gcd);
	gtk_box_pack_start (GTK_BOX (option_hbox), gcd->tracks, TRUE, TRUE, 0);

	button = make_button_from_stock (gcd, GTK_STOCK_INDEX,
					 G_CALLBACK(open_track_editor),
					 _("Open track editor"),
					 _("Track editor"));
	gtk_widget_set_sensitive (button, FALSE);
	gtk_box_pack_start (GTK_BOX (option_hbox), button, FALSE, FALSE, 0);
	gcd->trackeditor_b = button;

	gtk_box_pack_start (GTK_BOX (gcd->vbox), option_hbox, FALSE, FALSE, 0);
	
	/* Set the initial volume */
	set_volume (gcd);
	volume_notify_id = gconf_client_notify_add (gcd->client,
			"/apps/gnome-cd/volume", volume_changed_cb,
			gcd, NULL, &error);
	if (error != NULL) {
		g_warning ("Error: %s", error->message);
	}
	g_signal_connect (G_OBJECT (gcd->slider), "value-changed",
			  G_CALLBACK (volume_changed), gcd);
	
	button_hbox = gtk_hbox_new (TRUE, 2);

	/* Create the play and pause images, and ref them so they never
	   get destroyed */
	gcd->play_image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
	g_object_ref (gcd->play_image);

	gcd->pause_image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (gcd->pause_image);
	g_object_ref (gcd->pause_image);

	button = make_button_from_widget (gcd, gcd->play_image, G_CALLBACK (play_cb), _("Play / Pause"), _("Play"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->play_b = button;
	gcd->current_image = gcd->play_image;

	button = make_button_from_stock (gcd, GTK_STOCK_MEDIA_STOP, G_CALLBACK (stop_cb), _("Stop"), _("Stop"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->stop_b = button;

	button = make_button_from_stock (gcd, GTK_STOCK_MEDIA_PREVIOUS, G_CALLBACK (back_cb), _("Previous track"), _("Previous"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->back_b = button;

	button = make_button_from_stock (gcd, GTK_STOCK_MEDIA_REWIND, NULL, _("Rewind"), _("Rewind"));
	g_signal_connect (G_OBJECT (button), "button-press-event",
			  G_CALLBACK (rewind_press_cb), gcd);
	g_signal_connect (G_OBJECT (button), "button-release-event",
			  G_CALLBACK (rewind_release_cb), gcd);
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->rewind_b = button;
	
	button = make_button_from_stock (gcd, GTK_STOCK_MEDIA_FORWARD, NULL, _("Fast forward"), _("Fast forward"));
	g_signal_connect (G_OBJECT (button), "button-press-event",
			  G_CALLBACK (ffwd_press_cb), gcd);
	g_signal_connect (G_OBJECT (button), "button-release-event",
			  G_CALLBACK (ffwd_release_cb), gcd);
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->ffwd_b = button;

	button = make_button_from_stock (gcd, GTK_STOCK_MEDIA_NEXT, G_CALLBACK (next_cb), _("Next track"), _("Next track"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->next_b = button;

	button = make_button_from_stock (gcd, GNOME_CD_EJECT, G_CALLBACK (eject_cb), _("Eject CD"), _("Eject"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->eject_b = button;

	gtk_box_pack_start (GTK_BOX (gcd->vbox), button_hbox, FALSE, FALSE, 0);

	gtk_container_add (GTK_CONTAINER (gcd->window), gcd->vbox);
	gtk_widget_show_all (gcd->vbox);

	gcd->not_ready = FALSE;


	tray_icon_create (gcd);
	
	return gcd;
}

void
tray_icon_create (GnomeCD *gcd)
{
	GdkPixbuf *pixbuf;
	GdkPixbuf *icon;
	GtkWidget *box;

	icon = pixbuf_from_file ("gnome-cd/cd.png");
	pixbuf = gdk_pixbuf_scale_simple (icon, 16, 16, GDK_INTERP_BILINEAR);
	g_object_unref (G_OBJECT (icon));

	/* Tray icon */
	gcd->tray = gtk_status_icon_new_from_pixbuf (pixbuf);
	g_object_unref (G_OBJECT (pixbuf));

	gtk_status_icon_set_tooltip (gcd->tray, _("CD Player"));

	g_signal_connect (G_OBJECT (gcd->tray), "popup_menu",
			        G_CALLBACK (tray_popup_menu_cb), gcd);
	g_signal_connect (G_OBJECT (gcd->tray), "activate",
			 	G_CALLBACK (tray_icon_activated), gcd);

	g_object_set_data_full (G_OBJECT (gcd->tray), "tray-action-data", gcd,
				(GDestroyNotify) tray_icon_destroyed);
}

static int 
save_session(GnomeClient        *client,
             gint                phase,
             GnomeRestartStyle   save_style,
             gint                shutdown,
             GnomeInteractStyle  interact_style,
             gint                fast,
             gpointer            client_data) 
{

    gchar *argv[]= { NULL };

    argv[0] = (gchar*) client_data;
    gnome_client_set_clone_command (client, 1, argv);
    gnome_client_set_restart_command (client, 1, argv);

    return TRUE;
}

static void client_die(GnomeClient *client,
		       GnomeCD *gcd)
{
	gtk_widget_destroy (gcd->window);
	gtk_main_quit ();
}

static const char *items [] =
{
	GNOME_CD_EJECT
};

static void
register_stock_icons (void)
{
	GtkIconFactory *factory;
	int i;

	factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (factory);

	for (i = 0; i < (int) G_N_ELEMENTS (items); i++) {
		GtkIconSet *icon_set;
		GdkPixbuf *pixbuf;
		char *filename, *fullname;

		filename = g_strconcat ("gnome-cd/", items[i], ".png", NULL);
		fullname = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, filename, TRUE, NULL);
		if (fullname == NULL) {
			fullname = g_strconcat(GNOME_ICONDIR, "/", filename, NULL);
		}
		g_free (filename);

		pixbuf = gdk_pixbuf_new_from_file (fullname, NULL);
		g_free (fullname);

		icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
		gtk_icon_factory_add (factory, items[i], icon_set);
		gtk_icon_set_unref (icon_set);

		g_object_unref (G_OBJECT (pixbuf));
	}

	g_object_unref (G_OBJECT (factory));
}

static gboolean
start_playing (GnomeCD *gcd)
{
	/* Just fake a click on the button */
	play_cb (NULL, gcd);

	return FALSE; /* only call once */
}

int 
main (int argc, char *argv[])
{
	static const GOptionEntry cd_goptions[] = {
		{ "device", '\0', 0, G_OPTION_ARG_FILENAME, &cd_option_device,
		  N_("CD device to use"), NULL },
		{ "unique", '\0', 0, G_OPTION_ARG_NONE, &cd_option_unique,
		  N_("Only start if there isn't already a CD player application running"), NULL },
		{ "play",   '\0', 0, G_OPTION_ARG_NONE, &cd_option_play,
		  N_("Play the CD on startup"), NULL },
		{ "tray", '\0', 0, G_OPTION_ARG_NONE, &cd_option_tray,
		  N_("Start iconified in notification area"), NULL},
		{ NULL, '\0', 0, 0, NULL, NULL, NULL }
	};

	GnomeCD *gcd;
	GnomeClient *client;
	GOptionGroup *group;
	GOptionContext *ctx;

	free (malloc (8)); /* -lefence */

	g_thread_init (NULL);

	if (g_getenv ("GNOME_CD_DEBUG")) {
		debug_mode = TRUE;
	}
	
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	ctx = g_option_context_new ("gnome-cd");

#ifdef HAVE_GST
	g_option_context_add_group (ctx, gst_init_get_option_group ());
#endif

	g_option_context_add_main_entries (ctx, cd_goptions, GETTEXT_PACKAGE);

	gnome_program_init ("gnome-cd", VERSION, LIBGNOMEUI_MODULE, 
			    argc, argv, 
			    GNOME_PARAM_GOPTION_CONTEXT, ctx,
			    GNOME_PARAM_APP_DATADIR, DATADIR, NULL);

	register_stock_icons ();
	client = gnome_master_client ();
	g_signal_connect (client, "save_yourself",
                         G_CALLBACK (save_session), (gpointer) argv[0]);

	gcd = init_player (cd_option_device);
	if (gcd == NULL) {
		/* Stick a message box here? */
		g_error (_("Cannot create player"));
		exit (0);
	}

	if (!cd_option_tray)
		gtk_widget_show_all (gcd->window);

	g_signal_connect (client, "die",
			  G_CALLBACK (client_die), gcd);

	if (cd_option_play || gcd->preferences->start_play) {
		/* we do this delayed with a low priority to give the
		 * GStreamer backend a chance to process any pending TAG
		 * messages on the bus first (so it knows the CD layout) */
		g_idle_add_full (G_PRIORITY_LOW, (GSourceFunc) start_playing,
		                 gcd, NULL);
	}
	
	if (cd_option_unique &&
	    !cd_selection_is_master (gcd->cd_selection))
		return 0;

	setup_a11y_factory ();

	bonobo_main ();
	return 0;
}
