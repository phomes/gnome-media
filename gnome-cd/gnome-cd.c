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
#include "cdrom.h"
#include "callbacks.h"
#include "display.h"
#include "access/factory.h"

#define DEFAULT_THEME "lcd"

void
gnome_cd_set_window_title (GnomeCD *gcd,
			   const char *artist,
			   const char *track)
{
	char *title;

	if (artist == NULL &&
	    track == NULL) {
		title = g_strdup ("Gnome-CD " VERSION);
	} else {
		title = g_strconcat (artist, " - ", track, NULL);
	}

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
	GError *error;
	
	track = gtk_option_menu_get_history (GTK_OPTION_MENU (gcd->tracks));

	if (gnome_cdrom_get_status (GNOME_CDROM (gcd->cdrom), &status, NULL) == FALSE) {
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
		end_track = track + 2;
		endmsf = &msf;
	} else {
		end_track = -1;
		endmsf = NULL;
	}
	
	if (gnome_cdrom_play (GNOME_CDROM (gcd->cdrom), track + 1, &msf, end_track, endmsf, &error) == FALSE) {
		g_warning ("Error skipping %s", error->message);
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

	menu = gtk_menu_new ();
	if (gcd->disc_info != NULL) {
		int i;
		for (i = 0; i < gcd->disc_info->ntracks; i++) {
			char *title;

			title = g_strdup_printf ("%d - %s", i + 1, gcd->disc_info->track_info[i]->name);
			item = gtk_menu_item_new_with_label (title);
			g_free (title);
			gtk_widget_show (item);

			gtk_menu_append (menu, item);
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
							
							gtk_menu_append (menu, item);
						}
						
						g_free (data);
					}
				}
			}

			g_free (status);
		}
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (gcd->tracks), GTK_WIDGET (menu));
	gcd->menu = menu;

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
make_button_from_file (GnomeCD *gcd,
		       const char *filename,
		       GCallback func,
		       const char *tooltip,
		       const char *shortname)
{
	GtkWidget *pixmap;
	char *fullname;

	fullname = gnome_pixmap_file (filename);
	g_return_val_if_fail (fullname != NULL, NULL);

	pixmap = gtk_image_new_from_file (fullname);
	g_free (fullname);

	return make_button_from_widget (gcd, pixmap, func, tooltip, shortname);
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

	fullname = gnome_pixmap_file (filename);
	g_return_val_if_fail (fullname != NULL, NULL);

	pixbuf = gdk_pixbuf_new_from_file (fullname, NULL);
	g_free (fullname);

	return pixbuf;
}

static void
window_destroy_cb (GtkWidget *window,
		   gpointer data)
{
	GnomeCD *gcd = data;

	/* Before killing the cdrom object, do the shutdown on it */
	switch (gcd->preferences->stop) {
	case GNOME_CD_PREFERENCES_STOP_NOTHING:
		break;

	case GNOME_CD_PREFERENCES_STOP_STOP:
		gnome_cdrom_stop (gcd->cdrom, NULL);
		break;

	case GNOME_CD_PREFERENCES_STOP_OPEN:
		gnome_cdrom_eject (gcd->cdrom, NULL);
		break;

	case GNOME_CD_PREFERENCES_STOP_CLOSE:
		gnome_cdrom_close_tray (gcd->cdrom, NULL);
		break;

	default:
		g_assert_not_reached ();
		break;
	}

	/* Unref the cddb slave */
	close_cddb_client ();

	/* And the track editor */
	destroy_track_editor ();
	
	g_object_unref (gcd->cdrom);
	bonobo_main_quit ();
}

struct _MenuItem {
	char *name;
	char *icon;
	GCallback callback;
};

struct _MenuItem menuitems[] = {
	{N_("P_revious track"), "gnome-cd/a-first-menu.png", G_CALLBACK (back_cb)},
	{N_("_Stop"), "gnome-cd/a-stop-menu.png", G_CALLBACK (stop_cb)},
	{N_("_Play / Pause"), "gnome-cd/a-play-menu.png", G_CALLBACK (play_cb)},
	{N_("_Next track"), "gnome-cd/a-last-menu.png", G_CALLBACK (next_cb)},
	{N_("_Eject disc"), "gnome-cd/a-eject-menu.png", G_CALLBACK (eject_cb)},
	{N_("_About Gnome-CD"), GNOME_STOCK_ABOUT, G_CALLBACK (about_cb)},
	{NULL, NULL, NULL}
};

static GtkWidget *
make_popup_menu (GnomeCD *gcd)
{
	GtkWidget *menu;
	int i;

	menu = gtk_menu_new ();
	for (i = 0; menuitems[i].name != NULL; i++) {
		GtkWidget *item, *image;

		item = gtk_image_menu_item_new_with_mnemonic (menuitems[i].name);
		if (menuitems[i].icon != NULL) {
			char *ext = strrchr (menuitems[i].icon, '.');

			if (ext == NULL) {
				image = gtk_image_new_from_stock (menuitems[i].icon,
								  GTK_ICON_SIZE_MENU);
			} else {
				char *fullname;

				fullname = gnome_pixmap_file (menuitems[i].icon);
				if (fullname != NULL) {
					image = gtk_image_new_from_file (fullname);
				} else {
					image = NULL;
				}
				
				g_free (fullname);
			}

			if (image != NULL) {
				gtk_widget_show (image);
				gtk_image_menu_item_set_image (item, image);
			}
		}
				
		gtk_widget_show (item);

		gtk_menu_append (GTK_MENU (menu), item);
		if (menuitems[i].callback != NULL) {
			g_signal_connect (G_OBJECT (item), "activate",
					  G_CALLBACK (menuitems[i].callback), gcd);
		}
	}

	return menu;
}

static GnomeCD *
init_player (void) 
{
	GnomeCD *gcd;
	GtkWidget *display_box;
	GtkWidget *top_hbox, *button_hbox, *side_vbox;
	GtkWidget *button, *arrow;
	GdkPixbuf *pixbuf;
	GError *error = NULL;
	char *fullname;

	gcd = g_new0 (GnomeCD, 1);

	gcd->not_ready = TRUE;
	gcd->preferences = preferences_new (gcd);
	if (gcd->preferences->device == NULL || gcd->preferences->device[0] == 0) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_NONE,
						 _("There is no CD device set. This means that GnomeCD\n"
						   "will be unable to run. Press Set device to go to a dialog\n"
						   "where you can set the device, or click Quit to quit GnomeCD"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_QUIT, GTK_RESPONSE_CLOSE,
					_("Set device"), 1, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), 1);
		gtk_window_set_title (GTK_WINDOW (dialog), _("No CD device"));
		
		switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
		case 1:
			gtk_widget_destroy (dialog);
			dialog = preferences_dialog_show (gcd, TRUE);

			/* Don't care what it returns */
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);

			break;

		default:
			exit (0);
		}
	}
		
	gcd->cdrom = gnome_cdrom_new (gcd->preferences->device, GNOME_CDROM_UPDATE_CONTINOUS, &error);
	if (gcd->cdrom == NULL) {
		if (error != NULL) {
			g_warning ("%s: %s", __FUNCTION__, error->message);
			g_error_free (error);
		}
	} else {
		g_signal_connect (G_OBJECT (gcd->cdrom), "status-changed",
				  G_CALLBACK (cd_status_changed_cb), gcd);
	}

	if (error != NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_NONE,
						 _("The CD device that is set is not a CDRom drive. This means that GnomeCD\n"
						   "will be unable to run correctly. Press Set device to go to a dialog\n"
						   "where you can set the device, or click Quit to quit GnomeCD"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_QUIT, GTK_RESPONSE_CLOSE,
					_("Set device"), 1, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), 1);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Invalid CD device"));
		
		switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
		case 1:
			gtk_widget_destroy (dialog);
			dialog = preferences_dialog_show (gcd, TRUE);

			/* Don't care what it returns */
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);

			break;

		default:
			exit (0);
		}

		g_error_free (error);
	}
		
	gcd->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (gcd->window), "Gnome-CD "VERSION);
	gtk_window_set_wmclass (GTK_WINDOW (gcd->window), "main_window", "gnome-cd");
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
	gcd->vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gcd->tooltips = gtk_tooltips_new ();
	top_hbox = gtk_hbox_new (FALSE, 0);

	/* Create app controls */
	side_vbox = gtk_vbox_new (FALSE, 0);
	button = make_button_from_stock (gcd, GTK_STOCK_PREFERENCES, open_track_editor, _("Open track editor"), _("Track editor"));
	gtk_widget_set_sensitive (button, FALSE);
	gtk_box_pack_start (GTK_BOX (side_vbox), button, FALSE, FALSE, 0);
	gcd->trackeditor_b = button;

	button = make_button_from_stock (gcd, GTK_STOCK_PROPERTIES, open_preferences, _("Preferences"), _("Preferences"));
	gtk_box_pack_start (GTK_BOX (side_vbox), button, FALSE, FALSE, 0);
	gcd->properties_b = button;

	gtk_box_pack_start (GTK_BOX (top_hbox), side_vbox, FALSE, FALSE, 0);

	/* Create the display */
	display_box = gtk_vbox_new (FALSE, 1);

	gcd->display = GTK_WIDGET (cd_display_new ());
	GTK_WIDGET_SET_FLAGS (gcd->display, GTK_CAN_FOCUS | GTK_CAN_DEFAULT);
	g_signal_connect (G_OBJECT (gcd->display), "loopmode-changed",
			  G_CALLBACK (loopmode_changed_cb), gcd);
	g_signal_connect (G_OBJECT (gcd->display), "playmode-changed",
			  G_CALLBACK (playmode_changed_cb), gcd);

	/* Theme needs to be loaded after the display is created */
	gcd->theme = theme_load (gcd, gcd->preferences->theme_name);
	if (gcd->theme == NULL) {
		g_error ("Could not create theme");
	}
	
	gnome_popup_menu_attach (make_popup_menu (gcd), gcd->display, NULL);
	
	gtk_box_pack_start (GTK_BOX (display_box), gcd->display, TRUE, TRUE, 0);

	gcd->tracks = gtk_option_menu_new ();
	g_signal_connect (G_OBJECT (gcd->tracks), "changed",
			  G_CALLBACK (skip_to_track), gcd);
	gnome_cd_build_track_list_menu (gcd);
	gtk_box_pack_start (GTK_BOX (display_box), gcd->tracks, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (top_hbox), display_box, TRUE, TRUE, 0);
	
	gtk_box_pack_start (GTK_BOX (gcd->vbox), top_hbox, TRUE, TRUE, 0);
	
	button_hbox = gtk_hbox_new (TRUE, 2);
	
	button = make_button_from_file (gcd, "gnome-cd/a-first.png", G_CALLBACK (back_cb), _("Previous track"), _("Previous"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->back_b = button;

	button = make_button_from_file (gcd, "gnome-cd/a-rwnd.png", NULL, _("Rewind"), _("Rewind"));
	g_signal_connect (G_OBJECT (button), "button-press-event",
			  G_CALLBACK (rewind_press_cb), gcd);
	g_signal_connect (G_OBJECT (button), "button-release-event",
			  G_CALLBACK (rewind_release_cb), gcd);
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->rewind_b = button;

	/* Create the play and pause images, and ref them so they never
	   get destroyed */
	fullname = gnome_pixmap_file ("gnome-cd/a-play.png");
	gcd->play_image = gtk_image_new_from_file (fullname);
	g_object_ref (gcd->play_image);
	g_free (fullname);

	fullname = gnome_pixmap_file ("gnome-cd/a-pause.png");
	gcd->pause_image = gtk_image_new_from_file (fullname);
	gtk_widget_show (gcd->pause_image);
	g_object_ref (gcd->pause_image);
	g_free (fullname);

	button = make_button_from_widget (gcd, gcd->play_image, G_CALLBACK (play_cb), _("Play / Pause"), _("Play"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->play_b = button;
	gcd->current_image = gcd->play_image;
	
	button = make_button_from_file (gcd, "gnome-cd/a-stop.png", G_CALLBACK (stop_cb), _("Stop"), _("Stop"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->stop_b = button;

	button = make_button_from_file (gcd, "gnome-cd/a-fwd.png", NULL, _("Fast forward"), _("Fast forward"));
	g_signal_connect (G_OBJECT (button), "button-press-event",
			  G_CALLBACK (ffwd_press_cb), gcd);
	g_signal_connect (G_OBJECT (button), "button-release-event",
			  G_CALLBACK (ffwd_release_cb), gcd);
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->ffwd_b = button;

	button = make_button_from_file (gcd, "gnome-cd/a-last.png", G_CALLBACK (next_cb), _("Next track"), _("Next track"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->next_b = button;
	
	button = make_button_from_file (gcd, "gnome-cd/a-eject.png", G_CALLBACK (eject_cb), _("Eject CD"), _("Eject"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->eject_b = button;

#if 0
	button = make_button_from_file (gcd, "gnome-cd/mixer.png", G_CALLBACK (mixer_cb), _("Open mixer"), _("Open Mixer"));
	gtk_box_pack_start (GTK_BOX (bottom_hbox), button, FALSE, FALSE, 0);
	gcd->mixer_b = button;

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	button = make_button_from_widget (gcd, arrow, NULL, _("Change volume"), _("Volume"));
	gcd->volume_b = button;
	gtk_box_pack_start (GTK_BOX (bottom_hbox), button, FALSE, FALSE, 0);
#endif
	gtk_box_pack_start (GTK_BOX (gcd->vbox), button_hbox, FALSE, FALSE, 0);

	gtk_container_add (GTK_CONTAINER (gcd->window), gcd->vbox);
	gtk_widget_show_all (gcd->vbox);

	gcd->not_ready = FALSE;
	return gcd;
}

int 
main (int argc,
      char *argv[])
{
	GnomeCD *gcd;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init ("Gnome-CD", VERSION, LIBGNOMEUI_MODULE, 
			    argc, argv, NULL);
	gconf_init (argc, argv, NULL);
	gcd = init_player ();
	if (gcd == NULL) {
		g_error (_("Cannot create player"));
		exit (0);
	}

	/* Do the start up stuff */
	if (gcd->preferences->start_close) {
		gnome_cdrom_close_tray (gcd->cdrom, NULL);
	}
	switch (gcd->preferences->start) {
	case GNOME_CD_PREFERENCES_START_NOTHING:
		break;

	case GNOME_CD_PREFERENCES_START_START:
		/* Just fake a click on the button */
		play_cb (NULL, gcd);
		break;
		
	case GNOME_CD_PREFERENCES_START_STOP:
		gnome_cdrom_stop (gcd->cdrom, NULL);
		break;

	default:
		break;
	}
	
	gtk_widget_show (gcd->window);

	setup_factory();

	bonobo_main ();
	return 0;
}
