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
			 const char *tooltip)
{
	GtkWidget *button;

	button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (button), widget);

	if (func) {
		g_signal_connect (G_OBJECT (button), "clicked",
				  G_CALLBACK (func), gcd);
	}
	gtk_tooltips_set_tip (gcd->tooltips, button, tooltip, "");

	return button;
}

static GtkWidget *
make_button_from_file (GnomeCD *gcd,
		       const char *filename,
		       GCallback func,
		       const char *tooltip)
{
	GtkWidget *pixmap;
	char *fullname;

	fullname = gnome_pixmap_file (filename);
	g_return_val_if_fail (fullname != NULL, NULL);

	pixmap = gtk_image_new_from_file (fullname);
	g_free (fullname);

	return make_button_from_widget (gcd, pixmap, func, tooltip);
}

static GtkWidget *
make_button_from_stock (GnomeCD *gcd,
			const char *stock,
			GCallback func,
			const char *tooltip)
{
	GtkWidget *pixmap;

	pixmap = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_BUTTON);

	return make_button_from_widget (gcd, pixmap, func, tooltip);
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
	GtkWidget *top_hbox, *bottom_hbox, *button_hbox, *side_vbox;
	GtkWidget *button, *arrow, *frame;
	PangoLanguage *language;
	GError *error;

	gcd = g_new0 (GnomeCD, 1);

	gcd->state = GNOME_CD_STOP;
	gcd->cdrom = gnome_cdrom_new ("/dev/cdrom", &error);
	if (gcd->cdrom == NULL) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		g_error_free (error);

		g_free (gcd);
		return NULL;
	}

	gcd->pango_context = pango_ft2_get_context ();

	gcd->window = bonobo_window_new ("Gnome-CD", "Gnome-CD "VERSION);
	gtk_window_set_title (GTK_WINDOW (gcd->window), "Gnome-CD "VERSION);
	gtk_window_set_wmclass (GTK_WINDOW (gcd->window), "main_window", "gnome-cd");
	gtk_window_set_policy (GTK_WINDOW (gcd->window), FALSE, TRUE, TRUE);

	g_signal_connect (G_OBJECT (gcd->window), "delete_event",
			  G_CALLBACK (window_delete_cb), NULL);
	gcd->vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gcd->tooltips = gtk_tooltips_new ();
	top_hbox = gtk_hbox_new (FALSE, 0);

	/* Create app controls */
	side_vbox = gtk_vbox_new (TRUE, 0);
	button = make_button_from_stock (gcd, GTK_STOCK_PREFERENCES, NULL, _("Open track editor"));
	gtk_box_pack_start (GTK_BOX (side_vbox), button, FALSE, FALSE, 0);
	gcd->trackeditor_b = button;

	button = make_button_from_stock (gcd, GTK_STOCK_PROPERTIES, NULL, _("Preferences"));
	gtk_box_pack_start (GTK_BOX (side_vbox), button, FALSE, FALSE, 0);
	gcd->properties_b = button;

	gtk_box_pack_start (GTK_BOX (top_hbox), side_vbox, FALSE, FALSE, 0);

	/* Create the display */
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

	language = pango_language_from_string ("en_US");
	pango_context_set_language (gcd->pango_context, language);
	pango_context_set_base_dir (gcd->pango_context, PANGO_DIRECTION_LTR);

	gcd->font_desc = pango_font_description_new ();
	pango_context_set_font_description (gcd->pango_context, gcd->font_desc);
	
	gcd->display = gtk_drawing_area_new ();
	g_signal_connect (G_OBJECT (gcd->display), "expose-event",
			  G_CALLBACK (display_expose_cb), gcd);
	g_signal_connect (G_OBJECT (gcd->display), "size-allocate",
			  G_CALLBACK (display_size_allocate_cb), gcd);
	display_setup_layout (gcd);

	gtk_container_add (GTK_CONTAINER (frame), gcd->display);

	gtk_box_pack_start (GTK_BOX (top_hbox), frame, TRUE, TRUE, 0);
	
	gtk_box_pack_start (GTK_BOX (gcd->vbox), top_hbox, TRUE, TRUE, 0);
	
	bottom_hbox = gtk_hbox_new (FALSE, 0);
	button_hbox = gtk_hbox_new (TRUE, 0);
	
	button = make_button_from_file (gcd, "gnome-cd/back.xpm", G_CALLBACK (back_cb), _("Previous track"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->back_b = button;

	button = make_button_from_file (gcd, "gnome-cd/rewind.xpm", NULL, _("Rewind"));
	g_signal_connect (G_OBJECT (button), "button-press-event",
			  G_CALLBACK (rewind_press_cb), gcd);
	g_signal_connect (G_OBJECT (button), "button-release-event",
			  G_CALLBACK (rewind_release_cb), gcd);
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->rewind_b = button;

	button = make_button_from_file (gcd, "gnome-cd/play.xpm", G_CALLBACK (play_cb), _("Play / Pause"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->play_b = button;
	
	button = make_button_from_file (gcd, "gnome-cd/stop.xpm", G_CALLBACK (stop_cb), _("Stop"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->stop_b = button;

	button = make_button_from_file (gcd, "gnome-cd/ffwd.xpm", NULL, _("Fast forward"));
	g_signal_connect (G_OBJECT (button), "button-press-event",
			  G_CALLBACK (ffwd_press_cb), gcd);
	g_signal_connect (G_OBJECT (button), "button-release-event",
			  G_CALLBACK (ffwd_release_cb), gcd);
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->ffwd_b = button;

	button = make_button_from_file (gcd, "gnome-cd/next.xpm", G_CALLBACK (next_cb), _("Next track"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->next_b = button;
	
	button = make_button_from_file (gcd, "gnome-cd/eject.xpm", G_CALLBACK (eject_cb), _("Eject CD"));
	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, TRUE, 0);
	gcd->eject_b = button;
	gtk_box_pack_start (GTK_BOX (bottom_hbox), button_hbox, FALSE, FALSE, 0);

	button = make_button_from_file (gcd, "gnome-cd/volume.xpm", G_CALLBACK (mixer_cb), _("Open mixer"));
	gtk_box_pack_start (GTK_BOX (bottom_hbox), button, FALSE, FALSE, 0);
	gcd->mixer_b = button;

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	button = make_button_from_widget (gcd, arrow, NULL, _("Volume"));
	gcd->volume_b = button;
	gtk_box_pack_start (GTK_BOX (bottom_hbox), button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (gcd->vbox), bottom_hbox, FALSE, FALSE, 0);

	bonobo_window_set_contents (BONOBO_WINDOW (gcd->window), gcd->vbox);
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

	gcd->display_timeout = gtk_timeout_add (1000, display_update_cb, gcd);
	bonobo_main ();
	return 0;
}
