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

#include <pango/pangoft2.h>

#include "gnome-cd.h"
#include "cdrom.h"
#include "callbacks.h"
#include "display.h"

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

static int
window_delete_cb (GtkWidget *window,
		  GdkEvent *ev,
		  gpointer data)
{
	bonobo_main_quit ();
}

static GnomeCD *
init_player (void) 
{
	GnomeCD *gcd;
	GtkWidget *container;
	GtkWidget *top_hbox, *bottom_hbox, *button_hbox, *side_vbox;
	GtkWidget *button, *arrow, *frame;
	GdkPixbuf *pixbuf;
	GError *error;
	char *fullname;

	gcd = g_new0 (GnomeCD, 1);

	gcd->cdrom = gnome_cdrom_new ("/dev/cdrom", GNOME_CDROM_UPDATE_CONTINOUS, &error);
	if (gcd->cdrom == NULL) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);

		g_free (gcd);
		return NULL;
	}
	g_signal_connect (G_OBJECT (gcd->cdrom), "status-changed",
			  G_CALLBACK (cd_status_changed_cb), gcd);

	gcd->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (gcd->window), "Gnome-CD "VERSION);
	gtk_window_set_wmclass (GTK_WINDOW (gcd->window), "main_window", "gnome-cd");
	gtk_window_set_policy (GTK_WINDOW (gcd->window), FALSE, TRUE, TRUE);

	pixbuf = pixbuf_from_file ("gnome-cd/cd.png");
	if (pixbuf == NULL) {
		g_warning ("Error finding gnome-cd/cd.png");
	} else {
		gtk_window_set_icon (GTK_WINDOW (gcd->window), pixbuf);
		g_object_unref (G_OBJECT (pixbuf));
	}

	g_signal_connect (G_OBJECT (gcd->window), "delete_event",
			  G_CALLBACK (window_delete_cb), NULL);
	gcd->vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gcd->tooltips = gtk_tooltips_new ();
	top_hbox = gtk_hbox_new (FALSE, 0);

	/* Create app controls */
	side_vbox = gtk_vbox_new (TRUE, 0);
	button = make_button_from_stock (gcd, GTK_STOCK_PREFERENCES, NULL, _("Open track editor"), _("Track editor"));
	gtk_widget_set_sensitive (button, FALSE);
	gtk_box_pack_start (GTK_BOX (side_vbox), button, FALSE, FALSE, 0);
	gcd->trackeditor_b = button;

	button = make_button_from_stock (gcd, GTK_STOCK_PROPERTIES, NULL, _("Preferences"), _("Preferences"));
	gtk_widget_set_sensitive (button, FALSE);
	gtk_box_pack_start (GTK_BOX (side_vbox), button, FALSE, FALSE, 0);
	gcd->properties_b = button;

	gtk_box_pack_start (GTK_BOX (top_hbox), side_vbox, FALSE, FALSE, 0);

	/* Create the display */
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

	gcd->display = GTK_WIDGET (cd_display_new ());

	gtk_container_add (GTK_CONTAINER (frame), gcd->display);

	gtk_box_pack_start (GTK_BOX (top_hbox), frame, TRUE, TRUE, 0);
	
	gtk_box_pack_start (GTK_BOX (gcd->vbox), top_hbox, TRUE, TRUE, 0);
	
	bottom_hbox = gtk_hbox_new (FALSE, 0);
	button_hbox = gtk_hbox_new (TRUE, 0);
	
	button = make_button_from_file (gcd, "gnome-cd/back.xpm", G_CALLBACK (back_cb), _("Previous track"), _("Previous"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->back_b = button;

	button = make_button_from_file (gcd, "gnome-cd/rewind.xpm", NULL, _("Rewind"), _("Rewind"));
	g_signal_connect (G_OBJECT (button), "button-press-event",
			  G_CALLBACK (rewind_press_cb), gcd);
	g_signal_connect (G_OBJECT (button), "button-release-event",
			  G_CALLBACK (rewind_release_cb), gcd);
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->rewind_b = button;

	/* Create the play and pause images, and ref them so they never
	   get destroyed */
	fullname = gnome_pixmap_file ("gnome-cd/play.xpm");
	gcd->play_image = gtk_image_new_from_file (fullname);
	g_object_ref (gcd->play_image);
	g_free (fullname);

	fullname = gnome_pixmap_file ("gnome-cd/pause.xpm");
	gcd->pause_image = gtk_image_new_from_file (fullname);
	gtk_widget_show (gcd->pause_image);
	g_object_ref (gcd->pause_image);
	g_free (fullname);

	button = make_button_from_widget (gcd, gcd->play_image, G_CALLBACK (play_cb), _("Play / Pause"), _("Play"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->play_b = button;
	gcd->current_image = gcd->play_image;
	
	button = make_button_from_file (gcd, "gnome-cd/stop.xpm", G_CALLBACK (stop_cb), _("Stop"), _("Stop"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->stop_b = button;

	button = make_button_from_file (gcd, "gnome-cd/ffwd.xpm", NULL, _("Fast forward"), _("Fast forward"));
	g_signal_connect (G_OBJECT (button), "button-press-event",
			  G_CALLBACK (ffwd_press_cb), gcd);
	g_signal_connect (G_OBJECT (button), "button-release-event",
			  G_CALLBACK (ffwd_release_cb), gcd);
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->ffwd_b = button;

	button = make_button_from_file (gcd, "gnome-cd/next.xpm", G_CALLBACK (next_cb), _("Next track"), _("Next track"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->next_b = button;
	
	button = make_button_from_file (gcd, "gnome-cd/eject.xpm", G_CALLBACK (eject_cb), _("Eject CD"), _("Eject"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->eject_b = button;
	gtk_box_pack_start (GTK_BOX (bottom_hbox), button_hbox, FALSE, FALSE, 0);

	button = make_button_from_file (gcd, "gnome-cd/mixer.png", G_CALLBACK (mixer_cb), _("Open mixer"), _("Open Mixer"));
	gtk_box_pack_start (GTK_BOX (bottom_hbox), button, FALSE, FALSE, 0);
	gcd->mixer_b = button;

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	button = make_button_from_widget (gcd, arrow, NULL, _("Change volume"), _("Volume"));
	gcd->volume_b = button;
	gtk_box_pack_start (GTK_BOX (bottom_hbox), button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (gcd->vbox), bottom_hbox, FALSE, FALSE, 0);

	gtk_container_add (GTK_CONTAINER (gcd->window), gcd->vbox);
	gtk_widget_show_all (gcd->vbox);

	return gcd;
}

int 
main (int argc,
      char *argv[])
{
	GnomeCD *gcd;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init ("Gnome-CD", VERSION, LIBGNOMEUI_MODULE, 
			    argc, argv, NULL);
	gcd = init_player ();
	if (gcd == NULL) {
		g_error (_("Cannot create player"));
		exit (0);
	}

	gtk_widget_show (gcd->window);

	bonobo_main ();
	return 0;
}
