/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@ximian.com>
 *
 *  Copyright 2002 Iain Holmes
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

#include <gnome.h>
#include <gconf/gconf-client.h>

static GConfClient *client = NULL;
static GConfChangeSet *changeset;

typedef struct _PropertyDialog {
	GtkWidget *dialog;
	
	GtkWidget *no_info;
	GtkWidget *real_info;
	GtkWidget *specific_info;
	GtkWidget *name_box;
	GtkWidget *real_name;
	GtkWidget *real_host;

	GtkWidget *round_robin;
	GtkWidget *other_freedb;
	GtkWidget *freedb_box;
	GtkWidget *freedb_server;
	GtkWidget *update;
	
	GtkWidget *other_server;
	GtkWidget *other_box;
	GtkWidget *other_host;
	GtkWidget *other_port;
} PropertyDialog;

enum {
	CDDB_SEND_FAKE_INFO,
	CDDB_SEND_REAL_INFO,
	CDDB_SEND_OTHER_INFO
};

enum {
	CDDB_ROUND_ROBIN,
	CDDB_OTHER_FREEDB,
	CDDB_OTHER_SERVER
};

static void
destroy_window (GtkWidget *window,
		PropertyDialog *pd)
{
	g_free (pd);
	gtk_main_quit ();
}

static void
dialog_button_clicked_cb (GtkDialog *dialog,
			  int response_id,
			  GConfChangeSet *changeset)
{
	switch (response_id) {
	case GTK_RESPONSE_APPLY:
		gconf_client_commit_change_set (client, changeset, TRUE, NULL);
		break;

	default:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;
	}
}

static void
no_info_toggled (GtkToggleButton *tb,
		 PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	gtk_widget_set_sensitive (pd->name_box, FALSE);
	gconf_change_set_set_int (changeset, "/apps/CDDB-Slave2/info", CDDB_SEND_FAKE_INFO);
}

static void
real_info_toggled (GtkToggleButton *tb,
		   PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	gtk_widget_set_sensitive (pd->name_box, FALSE);
	gconf_change_set_set_int (changeset, "/apps/CDDB-Slave2/info", CDDB_SEND_REAL_INFO);
}

static void
specific_info_toggled (GtkToggleButton *tb,
		       PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	gtk_widget_set_sensitive (pd->name_box, TRUE);
	gconf_change_set_set_int (changeset, "/apps/CDDB-Slave2/info", CDDB_SEND_OTHER_INFO);
}

static void
real_name_changed (GtkEntry *entry,
		   PropertyDialog *pd)
{
	gconf_change_set_set_string (changeset, "/apps/CDDB-Slave2/name",
				     gtk_entry_get_text (entry));
}

static void
real_host_changed (GtkEntry *entry,
		   PropertyDialog *pd)
{
	gconf_change_set_set_string (changeset, "/apps/CDDB-Slave2/hostname",
				     gtk_entry_get_text (entry));
}

#define DEFAULT_SERVER "freedb.freedb.org"
#define DEFAULT_PORT 888

static void
round_robin_toggled (GtkToggleButton *tb,
		     PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	gtk_widget_set_sensitive (pd->freedb_box, FALSE);
	gtk_widget_set_sensitive (pd->other_box, FALSE);
	gconf_change_set_set_int (changeset, "/apps/CDDB-Slave2/server-type", CDDB_ROUND_ROBIN);
	gconf_change_set_set_string (changeset, "/apps/CDDB-Slave2/server", DEFAULT_SERVER);
	gconf_change_set_set_int (changeset, "/apps/CDDB-Slave2/port", DEFAULT_PORT);
}

static void
other_freedb_toggled (GtkToggleButton *tb,
		      PropertyDialog *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	gtk_widget_set_sensitive (pd->freedb_box, TRUE);
	gtk_widget_set_sensitive (pd->other_box, FALSE);

	gconf_change_set_set_int (changeset, "/apps/CDDB-Slave2/server-type", CDDB_OTHER_FREEDB);
	/* Set it to the default selection */
}

static void
other_server_toggled (GtkToggleButton *tb,
		      PropertyDialog *pd)
{
	char *str;
	
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	gtk_widget_set_sensitive (pd->freedb_box, FALSE);
	gtk_widget_set_sensitive (pd->other_box, TRUE);

	gconf_change_set_set_int (changeset,
				  "/apps/CDDB-Slave2/server-type", CDDB_OTHER_SERVER);
	str = gtk_entry_get_text (GTK_ENTRY (pd->other_host));
	if (str != NULL) {
		gconf_change_set_set_string (changeset,
					     "/apps/CDDB-Slave2/server", str);
	}

	str = gtk_entry_get_text (GTK_ENTRY (pd->other_port));
	if (str != NULL) {
		gconf_change_set_set_int (changeset,
					  "/apps/CDDB-Slave2/port", atoi (str));
	}
}

static void
other_host_changed (GtkEntry *entry,
		    PropertyDialog *pd)
{
	gconf_change_set_set_string (changeset, "/apps/CDDB-Slave2/server",
				     gtk_entry_get_text (entry));
}

static void
other_port_changed (GtkEntry *entry,
		    PropertyDialog *pd)
{
	gconf_change_set_set_int (changeset, "/apps/CDDB-Slave2/port",
				  atoi (gtk_entry_get_text (entry)));
}
	
static GtkTreeModel *
make_tree_model (void)
{
	GtkListStore *store;
	GtkTreeIter iter;

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

	/* Default */
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, "freedb.freedb.org", 1, "888", -1);

	return GTK_TREE_MODEL (store);
}

static void
create_dialog (GtkWidget *window)
{
	PropertyDialog *pd;
	
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;
	GtkWidget *label;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
	GtkTreeModel *model;

	char *str;
	int info = CDDB_SEND_FAKE_INFO;
	int port;
	
	pd = g_new (PropertyDialog, 1);
	pd->dialog = window;
	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (destroy_window), pd);

	/* Log on info */
	frame = gtk_frame_new (_("Log on information"));
	gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), frame, FALSE, FALSE, GNOME_PAD_SMALL);

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	info = gconf_client_get_int (client, "/apps/CDDB-Slave2/info", NULL);
	g_print ("info: %d\n", info);
	pd->no_info = gtk_radio_button_new_with_mnemonic (NULL, _("Send _no info"));
	if (info == CDDB_SEND_FAKE_INFO) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->no_info), TRUE);
	}
	g_signal_connect (G_OBJECT (pd->no_info), "toggled",
			  G_CALLBACK (no_info_toggled), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->no_info, FALSE, FALSE, 0);

	pd->real_info = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pd->no_info),
									_("Send real _info"));
	if (info == CDDB_SEND_REAL_INFO) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->real_info), TRUE);
	}
	g_signal_connect (G_OBJECT (pd->real_info), "toggled",
			  G_CALLBACK (real_info_toggled), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->real_info, FALSE, FALSE, 0);

	pd->specific_info = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pd->real_info),
									    _("Send _other info..."));
	if (info == CDDB_SEND_OTHER_INFO) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->specific_info), TRUE);
	}
	
	g_signal_connect (G_OBJECT (pd->specific_info), "toggled",
			  G_CALLBACK (specific_info_toggled), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->specific_info, FALSE, FALSE, 0);

	pd->name_box = gtk_vbox_new (TRUE, 0);
	if (info != CDDB_SEND_OTHER_INFO) {
		gtk_widget_set_sensitive (pd->name_box, FALSE);
	}
	
	gtk_box_pack_start (GTK_BOX (vbox), pd->name_box, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (pd->name_box), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("N_ame:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	pd->real_name = gtk_entry_new ();
	str = gconf_client_get_string (client, "/apps/CDDB-Slave2/name", NULL);
	if (str != NULL) {
		gtk_entry_set_text (GTK_ENTRY (pd->real_name), str);
	}
	g_signal_connect (G_OBJECT (pd->real_name), "changed",
			  G_CALLBACK (real_name_changed), pd);
	gtk_box_pack_start (GTK_BOX (hbox), pd->real_name, TRUE, TRUE, 0);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (pd->name_box), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("_Hostname:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	pd->real_host = gtk_entry_new ();
	str = gconf_client_get_string (client, "/apps/CDDB-Slave2/hostname", NULL);
	if (str != NULL) {
		gtk_entry_set_text (GTK_ENTRY (pd->real_host), str);
	}
	g_signal_connect (G_OBJECT (pd->real_host), "changed",
			  G_CALLBACK (real_host_changed), pd);
	gtk_box_pack_start (GTK_BOX (hbox), pd->real_host, TRUE, TRUE, 0);

	/* Server info */
	frame = gtk_frame_new (_("Server"));
	gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), frame, TRUE, TRUE, GNOME_PAD_SMALL);

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	info = gconf_client_get_int (client, "/apps/CDDB-Slave2/server-type", NULL);
	pd->round_robin = gtk_radio_button_new_with_mnemonic (NULL, _("FreeDB _round robin server"));
	g_signal_connect (G_OBJECT (pd->round_robin), "toggled",
			  G_CALLBACK (round_robin_toggled), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->round_robin, FALSE, FALSE, 0);

	pd->other_freedb = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pd->round_robin),
									   _("Other _FreeDB server..."));
	g_signal_connect (G_OBJECT (pd->other_freedb), "toggled",
			  G_CALLBACK (other_freedb_toggled), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->other_freedb, FALSE, FALSE, 0);

	pd->freedb_box = gtk_vbox_new (FALSE, 0);
	gtk_widget_set_sensitive (pd->freedb_box, FALSE);
	gtk_box_pack_start (GTK_BOX (vbox), pd->freedb_box, TRUE, TRUE, 0);

	model = make_tree_model ();
	pd->freedb_server = gtk_tree_view_new_with_model (model);
	g_object_unref (model);

	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Server"), cell,
							"text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pd->freedb_server), col);
	col = gtk_tree_view_column_new_with_attributes (_("Port"), cell,
							"text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pd->freedb_server), col);
	
	gtk_box_pack_start (GTK_BOX (pd->freedb_box), pd->freedb_server, TRUE, TRUE, 2);

	pd->update = gtk_button_new_with_label (_("Update server list"));
	gtk_box_pack_start (GTK_BOX (pd->freedb_box), pd->update, FALSE, FALSE, 2);

	pd->other_server = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pd->other_freedb),
									   _("Other _server..."));
	g_signal_connect (G_OBJECT (pd->other_server), "toggled",
			  G_CALLBACK (other_server_toggled), pd);
	gtk_box_pack_start (GTK_BOX (vbox), pd->other_server, FALSE, FALSE, 0);

	pd->other_box = gtk_vbox_new (TRUE, 0);
	gtk_widget_set_sensitive (pd->other_box, FALSE);
	gtk_box_pack_start (GTK_BOX (vbox), pd->other_box, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (pd->other_box), hbox, TRUE, TRUE, 0);
	
	label = gtk_label_new (_("Hostname:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	pd->other_host = gtk_entry_new ();
	g_signal_connect (G_OBJECT (pd->other_host), "changed",
			  G_CALLBACK (other_host_changed), pd);
	str = gconf_client_get_string (client, "/apps/CDDB-Slave2/server", NULL);
	if (str != NULL) {
		gtk_entry_set_text (GTK_ENTRY (pd->other_host), str);
	}
	gtk_box_pack_start (GTK_BOX (hbox), pd->other_host, TRUE, TRUE, 0);

	label = gtk_label_new_with_mnemonic (_("_Port:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	pd->other_port = gtk_entry_new ();
	g_signal_connect (G_OBJECT (pd->other_port), "changed",
			  G_CALLBACK (other_port_changed), pd);
	port = gconf_client_get_int (client, "/apps/CDDB-Slave2/port", NULL);
	str = g_strdup_printf ("%d", port);
	gtk_entry_set_text (GTK_ENTRY (pd->other_port), str);
	g_free (str);
	
	gtk_box_pack_start (GTK_BOX (hbox), pd->other_port, FALSE, FALSE, 0);

	switch (info) {
	case CDDB_ROUND_ROBIN:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->round_robin), TRUE);
		break;

	case CDDB_OTHER_FREEDB:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->other_freedb), TRUE);
		break;

	case CDDB_OTHER_SERVER:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->other_server), TRUE);
		break;

	default:
		break;
	}
}
	
int
main (int argc,
      char **argv)
{
	GtkWidget *dialog_win;
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);

	gnome_program_init (argv[0], VERSION, LIBGNOMEUI_MODULE, argc, argv, NULL);

	client = gconf_client_get_default ();
	gconf_client_add_dir (client, "/apps/CDDB-Slave2",
			      GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	changeset = gconf_change_set_new ();

	dialog_win = gtk_dialog_new_with_buttons (_("CDDBSlave 2 Properties"),
						  NULL, -1,
						  GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
						  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
						  NULL);
	create_dialog (dialog_win);
	
  	g_signal_connect (G_OBJECT (dialog_win), "response",
  			  G_CALLBACK (dialog_button_clicked_cb), changeset);

	gtk_widget_show_all (dialog_win);

	gtk_main ();
	gconf_change_set_unref (changeset);

	return 0;
}
