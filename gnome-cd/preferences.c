/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@ximian.com>
 *
 *  Copyright 2002 Ximain, Inc. (www.ximian.com)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtkdialog.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbutton.h>

#include <libgnome/gnome-i18n.h>

#include <gconf/gconf-client.h>

#include "gnome-cd.h"
#include "preferences.h"

static GConfClient *client = NULL;

static void
do_device_changed (GnomeCDPreferences *prefs,
		   const char *device)
{
	if (prefs->device != NULL) {
		g_free (prefs->device);
	}

	prefs->device = g_strdup (device);
	gnome_cdrom_set_device (prefs->gcd->cdrom, prefs->device, NULL);
}

static void
device_changed (GConfClient *_client,
		guint cnxn_id,
		GConfEntry *entry,
		gpointer user_data)
{
	GnomeCDPreferences *prefs = user_data;
	GConfValue *value = gconf_entry_get_value (entry);

	do_device_changed (prefs, gconf_value_get_string (value));
}

static void
on_start_changed (GConfClient *_client,
		  guint cnxn_id,
		  GConfEntry *entry,
		  gpointer user_data)
{
	GnomeCDPreferences *prefs = user_data;
	GConfValue *value = gconf_entry_get_value (entry);

	prefs->start = gconf_value_get_int (value);
	/* This doesn't take effect till next start,
	   so we don't need to do anything for it */
}

static void
close_on_start_changed (GConfClient *_client,
			guint cnxn_id,
			GConfEntry *entry,
			gpointer user_data)
{
	GnomeCDPreferences *prefs = user_data;
	GConfValue *value = gconf_entry_get_value (entry);

	prefs->start_close = gconf_value_get_bool (value);
	/* This doesn't take effect till next start either */
}

static void
on_stop_changed (GConfClient *_client,
		 guint cnxn_id,
		 GConfEntry *entry,
		 gpointer user_data)
{
	GnomeCDPreferences *prefs = user_data;
	GConfValue *value = gconf_entry_get_value (entry);

	prefs->stop = gconf_value_get_int (value);
	/* This doesn't take effect till we quit... */
}

static void
do_theme_changed (GnomeCDPreferences *prefs,
		  const char *theme_name)
{
	if (prefs->theme_name != NULL) {
		g_free (prefs->theme_name);
	}

	prefs->theme_name = g_strdup (theme_name);
	/* FIXME: Change */
}

static void
theme_changed (GConfClient *_client,
	       guint cnxn_id,
	       GConfEntry *entry,
	       gpointer user_data)
{
	GnomeCDPreferences *prefs = user_data;
	GConfValue *value = gconf_entry_get_value (entry);

	do_theme_changed (prefs, gconf_value_get_string (value));
}

static void
restore_preferences (GnomeCDPreferences *prefs)
{
	GError *error = NULL;

	/* Add the dir, cos we're getting all the stuff from it anyway */
	gconf_client_add_dir (client, "/apps/gnome-cd",
			      GCONF_CLIENT_PRELOAD_NONE, &error);
	if (error != NULL) {
		g_warning ("Error: %s", error->message);
		error = NULL;
	}
	
	prefs->device = gconf_client_get_string (client,
						 "/apps/gnome-cd/device", NULL);
	prefs->device_id = gconf_client_notify_add (client,
						    "/apps/gnome-cd/device", device_changed,
						    prefs, NULL, &error);
	if (error != NULL) {
		g_warning ("Error: %s", error->message);
	}
						    
	prefs->start = gconf_client_get_int (client,
					     "/apps/gnome-cd/on-start", NULL);
	prefs->start_id = gconf_client_notify_add (client,
						   "/apps/gnome-cd/on-start",
						   on_start_changed, prefs,
						   NULL, NULL);
	
	prefs->start_close = gconf_client_get_bool (client,
						    "/apps/gnome-cd/close-on-start", NULL);
	prefs->close_id = gconf_client_notify_add (client,
						   "/apps/gnome-cd/close-on-start",
						   close_on_start_changed, prefs,
						   NULL, NULL);
	
	prefs->stop = gconf_client_get_int (client,
					    "/apps/gnome-cd/on-stop", NULL);
	prefs->stop_id = gconf_client_notify_add (client,
						  "/apps/gnome-cd/on-stop",
						  on_stop_changed, prefs,
						  NULL, NULL);

	prefs->theme_name = gconf_client_get_string (client,
						     "/apps/gnome-cd/theme-name", NULL);
	prefs->theme_id = gconf_client_notify_add (client,
						   "/apps/gnome-cd/theme-name",
						   theme_changed, prefs,
						   NULL, NULL);
}

void
preferences_free (GnomeCDPreferences *prefs)
{
	GConfClient *client;
	
	if (prefs->device != NULL) {
		g_free (prefs->device);
	}

	if (prefs->theme_name != NULL) {
		g_free (prefs->theme_name);
	}

	/* Remove the listeners */
	gconf_client_notify_remove (client, prefs->device_id);
	gconf_client_notify_remove (client, prefs->start_id);
	gconf_client_notify_remove (client, prefs->close_id);
	gconf_client_notify_remove (client, prefs->stop_id);

	g_free (prefs);
}

GnomeCDPreferences *
preferences_new (GnomeCD *gcd)
{
	GnomeCDPreferences *prefs;

	prefs = g_new (GnomeCDPreferences, 1);
	prefs->gcd = gcd;

	client = gconf_client_get_default ();
	
	restore_preferences (prefs);
	
	return prefs;
}

typedef struct _PropertyDialog {
	GnomeCD *gcd; /* The GnomeCD object this is connected to */
	GtkWidget *window;

	GtkWidget *cd_device;
	GtkWidget *apply;
	
	GtkWidget *start_nothing;
	GtkWidget *start_play;
	GtkWidget *start_stop;
	GtkWidget *start_close;

	GtkWidget *stop_nothing;
	GtkWidget *stop_stop;
	GtkWidget *stop_open;
	GtkWidget *stop_close;

	guint start_id;
	guint device_id;
	guint close_id;
	guint stop_id;
} PropertyDialog;

static void
prefs_response_cb (GtkWidget *dialog,
		   int response_id,
		   PropertyDialog *pd)
{
	switch (response_id) {
	case GTK_RESPONSE_CLOSE:
	case GTK_RESPONSE_NONE:
	case GTK_RESPONSE_DELETE_EVENT:
		gtk_widget_destroy (dialog);
		break;

	default:
		g_print ("Response %d\n", response_id);
		g_assert_not_reached ();
		break;
	}
}

static void
prefs_destroy_cb (GtkDialog *dialog,
		  PropertyDialog *pd)
{
	gconf_client_notify_remove (client, pd->device_id);
	gconf_client_notify_remove (client, pd->start_id);
	gconf_client_notify_remove (client, pd->stop_id); 
	gconf_client_notify_remove (client, pd->close_id);
	
	g_free (pd);
}

static void
apply_clicked_cb (GtkWidget *apply,
		  PropertyDialog *pd)
{
	char *new_device;

	new_device = gtk_entry_get_text (GTK_ENTRY (pd->cd_device));
	if (strcmp (new_device, pd->gcd->preferences->device) == 0) {
		return;
	}
	gconf_client_set_string (client, "/apps/gnome-cd/device", new_device, NULL);
	gtk_widget_set_sensitive (pd->apply, FALSE);
}

static void
device_changed_cb (GtkWidget *entry,
		   PropertyDialog *pd)
{
	char *new_device;

	new_device = gtk_entry_get_text (GTK_ENTRY (entry));
	if (strcmp (new_device, pd->gcd->preferences->device) == 0) {
		gtk_widget_set_sensitive (pd->apply, FALSE);
		return;
	}

	gtk_widget_set_sensitive (pd->apply, TRUE);
}

static void
change_device_widget (GConfClient *_client,
		      guint cnxn,
		      GConfEntry *entry,
		      gpointer user_data)
{
	PropertyDialog *pd = user_data;
	GConfValue *value = gconf_entry_get_value (entry);

	g_signal_handlers_block_matched (G_OBJECT (pd->cd_device), G_SIGNAL_MATCH_FUNC,
					 0, 0, NULL, G_CALLBACK (device_changed_cb), pd);
	gtk_entry_set_text (GTK_ENTRY (pd->cd_device), gconf_value_get_string (value));
	g_signal_handlers_unblock_matched (G_OBJECT (pd->cd_device), G_SIGNAL_MATCH_FUNC,
					   0, 0, NULL, G_CALLBACK (device_changed_cb), pd);
}

static void
set_start (PropertyDialog *pd,
	   GnomeCDPreferencesStart start)
{
	gconf_client_set_int (client,
			      "/apps/gnome-cd/on-start",
			      start, NULL);
}

static void
start_nothing_toggled_cb (GtkToggleButton *tb,
			  PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	set_start (pd, GNOME_CD_PREFERENCES_START_NOTHING);
}

static void
start_play_toggled_cb (GtkToggleButton *tb,
		       PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	set_start (pd, GNOME_CD_PREFERENCES_START_START);
}

static void
start_stop_toggled_cb (GtkToggleButton *tb,
		       PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	set_start (pd, GNOME_CD_PREFERENCES_START_STOP);
}

static void
set_stop (PropertyDialog *pd,
	  GnomeCDPreferencesStop stop)
{
	gconf_client_set_int (client,
			      "/apps/gnome-cd/on-stop",
			      stop, NULL);
}

static void
stop_nothing_toggled_cb (GtkToggleButton *tb,
			 PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	set_stop (pd, GNOME_CD_PREFERENCES_STOP_NOTHING);
}

static void
stop_stop_toggled_cb (GtkToggleButton *tb,
		      PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	set_stop (pd, GNOME_CD_PREFERENCES_STOP_STOP);
}

static void
stop_open_toggled_cb (GtkToggleButton *tb,
		      PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	set_stop (pd, GNOME_CD_PREFERENCES_STOP_OPEN);
}

static void
stop_close_toggled_cb (GtkToggleButton *tb,
		       PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	set_stop (pd, GNOME_CD_PREFERENCES_STOP_CLOSE);
}

static void
change_start_widget (GConfClient *client,
		     guint cnxn,
		     GConfEntry *entry,
		     gpointer user_data)
{
	PropertyDialog *pd = user_data;
	GConfValue *value = gconf_entry_get_value (entry);
	GCallback *func;
	GtkToggleButton *tb;
	
	switch (gconf_value_get_int (value)) {
	case GNOME_CD_PREFERENCES_START_NOTHING:
		tb = GTK_TOGGLE_BUTTON (pd->start_nothing);
		func = G_CALLBACK (start_nothing_toggled_cb);
		break;

	case GNOME_CD_PREFERENCES_START_START:
		tb = GTK_TOGGLE_BUTTON (pd->start_play);
		func = G_CALLBACK (start_play_toggled_cb);
		break;

	case GNOME_CD_PREFERENCES_START_STOP:
		tb = GTK_TOGGLE_BUTTON (pd->start_stop);
		func = G_CALLBACK (start_stop_toggled_cb);
		break;

	default:
		g_warning ("Unknown value: %d", gconf_value_get_int (value));
		return;
	}
	
	g_signal_handlers_block_matched (G_OBJECT (tb), G_SIGNAL_MATCH_FUNC,
					 0, 0, NULL, func, pd);
	gtk_toggle_button_set_active (tb, TRUE);
	g_signal_handlers_unblock_matched (G_OBJECT (tb), G_SIGNAL_MATCH_FUNC,
					   0, 0, NULL, func, pd);
}

static void
change_stop_widget (GConfClient *client,
		    guint cnxn,
		    GConfEntry *entry,
		    gpointer user_data)
{
	PropertyDialog *pd = user_data;
	GConfValue *value = gconf_entry_get_value (entry);
	GCallback *func;
	GtkToggleButton *tb;

	switch (gconf_value_get_int (value)) {
	case GNOME_CD_PREFERENCES_STOP_NOTHING:
		tb = GTK_TOGGLE_BUTTON (pd->stop_nothing);
		func = G_CALLBACK (stop_nothing_toggled_cb);
		break;

	case GNOME_CD_PREFERENCES_STOP_STOP:
		tb = GTK_TOGGLE_BUTTON (pd->stop_stop);
		func = G_CALLBACK (stop_stop_toggled_cb);
		break;

	case GNOME_CD_PREFERENCES_STOP_OPEN:
		tb = GTK_TOGGLE_BUTTON (pd->stop_open);
		func = G_CALLBACK (stop_open_toggled_cb);
		break;

	case GNOME_CD_PREFERENCES_STOP_CLOSE:
		tb = GTK_TOGGLE_BUTTON (pd->stop_close);
		func = G_CALLBACK (stop_close_toggled_cb);
		break;

	default:
		g_warning ("Unknown stop value: %d", gconf_value_get_int (value));
		return;
	}

	g_signal_handlers_block_matched (G_OBJECT (tb), G_SIGNAL_MATCH_FUNC,
					 0, 0, NULL, func, pd);
	gtk_toggle_button_set_active (tb, TRUE);
	g_signal_handlers_unblock_matched (G_OBJECT (tb), G_SIGNAL_MATCH_FUNC,
					   0, 0, NULL, func, pd);
}

static void
start_close_toggled_cb (GtkToggleButton *tb,
			PropertyDialog *pd)
{
	gboolean on;

	on = gtk_toggle_button_get_active (tb);
	gconf_client_set_bool (client,
			       "/apps/gnome-cd/close-on-start", on, NULL);
}

static void
change_start_close_widget (GConfClient *client,
			    guint cnxn,
			    GConfEntry *entry,
			    gpointer user_data)
{
	PropertyDialog *pd = user_data;
	GConfValue *value = gconf_entry_get_value (entry);

	g_signal_handlers_block_matched (G_OBJECT (pd->start_close), G_SIGNAL_MATCH_FUNC,
					 0, 0, NULL, G_CALLBACK (start_close_toggled_cb), pd);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->start_close),
				      gconf_value_get_bool (value));
	g_signal_handlers_unblock_matched (G_OBJECT (pd->start_close), G_SIGNAL_MATCH_FUNC,
					   0, 0, NULL, G_CALLBACK (start_close_toggled_cb), pd);
}

GtkWidget *
preferences_dialog_show (GnomeCD *gcd)
{
	PropertyDialog *pd;
	GtkWidget *hbox, *vbox, *label, *frame;

	pd = g_new (PropertyDialog, 1);
	
	pd->gcd = gcd;
	
	pd->window = gtk_dialog_new_with_buttons (_("Gnome-CD Preferences"),
						  GTK_WINDOW (gcd->window),
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	g_signal_connect (G_OBJECT (pd->window), "response",
			  G_CALLBACK (prefs_response_cb), pd);
	g_signal_connect (G_OBJECT (pd->window), "destroy",
			  G_CALLBACK (prefs_destroy_cb), pd);

	/* General */
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (pd->window)->vbox), 2);
	
	/* Top stuff */
	hbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pd->window)->vbox), hbox, FALSE, FALSE, 4);

	label = gtk_label_new_with_mnemonic (_("CD player de_vice"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	pd->cd_device = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (pd->cd_device), gcd->preferences->device);
	g_signal_connect (G_OBJECT (pd->cd_device), "changed",
			  G_CALLBACK (device_changed_cb), pd);
	g_signal_connect (G_OBJECT (pd->cd_device), "activate",
			  G_CALLBACK (apply_clicked_cb), pd);
	gtk_box_pack_start (GTK_BOX (hbox), pd->cd_device, TRUE, TRUE, 0);
	
	pd->apply = gtk_button_new_with_mnemonic (_("_Apply change"));
	gtk_widget_set_sensitive (pd->apply, FALSE);
	g_signal_connect (G_OBJECT (pd->apply), "clicked",
			  G_CALLBACK (apply_clicked_cb), pd);
	gtk_box_pack_start (GTK_BOX (hbox), pd->apply, FALSE, FALSE, 0);

	pd->device_id = gconf_client_notify_add (client,
						 "/apps/gnome-cd/device",
						 change_device_widget, pd, NULL, NULL);
	hbox = gtk_hbox_new (TRUE, 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pd->window)->vbox), hbox, TRUE, TRUE, 4);

	/* left side */
	frame = gtk_frame_new (_("When Gnome-CD starts"));
	gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
	
	vbox = gtk_vbox_new (TRUE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	pd->start_nothing = gtk_radio_button_new_with_mnemonic (NULL, _("Do _nothing"));
	if (gcd->preferences->start == GNOME_CD_PREFERENCES_START_NOTHING) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->start_nothing), TRUE);
	}
	g_signal_connect (G_OBJECT (pd->start_nothing), "toggled",
			  G_CALLBACK (start_nothing_toggled_cb), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->start_nothing, FALSE, FALSE, 0);

	pd->start_play = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pd->start_nothing),
									 _("Start _playing CD"));
	if (gcd->preferences->start == GNOME_CD_PREFERENCES_START_START) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->start_play), TRUE);
	}
	g_signal_connect (G_OBJECT (pd->start_play), "toggled",
			  G_CALLBACK (start_play_toggled_cb), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->start_play, FALSE, FALSE, 0);
	
	pd->start_stop = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pd->start_play),
									 _("_Stop playing CD"));
	if (gcd->preferences->start == GNOME_CD_PREFERENCES_START_STOP) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->start_stop), TRUE);
	}
	g_signal_connect (G_OBJECT (pd->start_stop), "toggled",
			  G_CALLBACK (start_stop_toggled_cb), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->start_stop, FALSE, FALSE, 0);

	pd->start_id = gconf_client_notify_add (client,
						"/apps/gnome-cd/on-start",
						change_start_widget, pd, NULL, NULL);

	pd->start_close = gtk_check_button_new_with_mnemonic (_("Attempt to _close CD tray"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->start_close),
				      gcd->preferences->start_close);
	g_signal_connect (G_OBJECT (pd->start_close), "toggled",
			  G_CALLBACK (start_close_toggled_cb), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->start_close, FALSE, FALSE, 0);

	pd->close_id = gconf_client_notify_add (client,
						"/apps/gnome-cd/close-on-start",
						change_start_close_widget, pd, NULL, NULL);
	/* Right side */
	frame = gtk_frame_new (_("When Gnome-CD quits"));
	gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

	vbox = gtk_vbox_new (TRUE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	pd->stop_nothing = gtk_radio_button_new_with_mnemonic (NULL, _("Do not_hing"));
	if (gcd->preferences->stop == GNOME_CD_PREFERENCES_STOP_NOTHING) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->stop_nothing), TRUE);
	}
	g_signal_connect (G_OBJECT (pd->stop_nothing), "toggled",
			  G_CALLBACK (stop_nothing_toggled_cb), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->stop_nothing, FALSE, FALSE, 0);

	pd->stop_stop = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pd->stop_nothing),
									_("S_top playing CD"));
	if (gcd->preferences->stop == GNOME_CD_PREFERENCES_STOP_STOP) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->stop_stop), TRUE);
	}
	g_signal_connect (G_OBJECT (pd->stop_stop), "toggled",
			  G_CALLBACK (stop_stop_toggled_cb), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->stop_stop, FALSE, FALSE, 0);

	pd->stop_open = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pd->stop_stop),
									_("Attempt to _open CD tray"));
	if (gcd->preferences->stop == GNOME_CD_PREFERENCES_STOP_OPEN) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->stop_open), TRUE);
	}
	g_signal_connect (G_OBJECT (pd->stop_open), "toggled",
			  G_CALLBACK (stop_open_toggled_cb), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->stop_open, FALSE, FALSE, 0);

	pd->stop_close = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pd->stop_open),
									 _("Attempt to c_lose CD tray"));
	if (gcd->preferences->stop == GNOME_CD_PREFERENCES_STOP_CLOSE) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->stop_close), TRUE);
	}
	g_signal_connect (G_OBJECT (pd->stop_close), "toggled",
			  G_CALLBACK (stop_close_toggled_cb), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->stop_close, FALSE, FALSE, 0);

	pd->stop_id = gconf_client_notify_add (client,
					       "/apps/gnome-cd/on-stop",
					       change_stop_widget, pd, NULL, NULL);
	gtk_widget_show_all (pd->window);

	return pd->window;
}
