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
#include <bonobo-activation/bonobo-activation.h>
#include <libbonoboui.h>

#include "cddb-disclosure.h"

#define CDDBSLAVE_TRACK_EDITOR_IID "OAFIID:GNOME_Media_CDDBSlave2_Track_Editor_Factory"

typedef struct _TrackEditorDialog {
	GtkWidget *parent;
	GtkWidget *artist;
	GtkWidget *disctitle;
	GtkWidget *disccomments;
	GtkWidget *year;
	GtkWidget *genre;
	GtkWidget *tracks;
	GtkWidget *extra_info;
	
	GtkTreeModel *model;
} TrackEditorDialog;

static char *fake_names[13] = {
	"Everyday",
	"Our Way to Fall",
	"Saturday",
	"Let's Save Tony Orlando's House",
	"Last Days of Disco",
	"The Crying of Lot G",
	"You Can Have It All",
	"Tears Are in Your Eyes",
	"Cherry Chapstick",
	"From Black To Blue",
	"Madeline",
	"Tired Hippo",
	"Night Falls on Hoboken"
};

static char *fake_lengths[13] = {
	"6:30",
	"4:17",
	"4:17",
	"4:59",
	"6:27",
	"4:43",
	"4:34",
	"4:33",
	"6:09",
	"4:46",
	"3:35",
	"4:44",
	"17:41"
};

static GtkTreeModel *
make_tree_model (void)
{
	GtkListStore *store;
	GtkTreeIter iter;
	int i;
	
	store = gtk_list_store_new (3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
	/* Pretend there's 13 tracks */
	for (i = 0; i < 13; i++) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, i, 1, fake_names[i], 2, fake_lengths[i], -1);
	}

	return GTK_TREE_MODEL (store);
}
	
#if 0
static BonoboControl *
#else
static TrackEditorDialog *
#endif
make_track_editor_control (void)
{
	GtkWidget *hbox, *vbox, *ad_vbox, *ad_vbox2;
	GtkWidget *label;
	GtkWidget *sep;
	GtkWidget *advanced;
	GtkWidget *sw;
	TrackEditorDialog *td;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
	
	td = g_new0 (TrackEditorDialog, 1);

	td->parent = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (td->parent), 2);

	/* Info label */
	label = gtk_label_new (_("Editting discid: 4df231ca"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (td->parent), label, FALSE, FALSE, 0);

	sep = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (td->parent), sep, FALSE, FALSE, 0);

	/* Artist */
	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (td->parent), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("_Artist:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	td->artist = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), td->artist, TRUE, TRUE, 0);

	/* Disc title */
	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (td->parent), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("Disc _Title:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	td->disctitle = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), td->disctitle, TRUE, TRUE, 0);

	advanced = cddb_disclosure_new (_("Show advanced disc options"),
					_("Hide advanced disc options"));
	gtk_box_pack_start (GTK_BOX (td->parent), advanced, FALSE, FALSE, 0);

	/* Advanced disc options */
	ad_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (ad_vbox), 2);
	gtk_box_pack_start (GTK_BOX (td->parent), ad_vbox, FALSE, FALSE, 0);
	cddb_disclosure_set_container (CDDB_DISCLOSURE (advanced), ad_vbox);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (ad_vbox), hbox, FALSE, FALSE, 0);

	/* Top box: Disc comments. Maybe should be a GtkText? */
	label = gtk_label_new (_("Disc comments:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	td->disccomments = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), td->disccomments, TRUE, TRUE, 0);

	/* Bottom box */
	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (ad_vbox), hbox, FALSE, FALSE, 0);

	/* Genre */
	label = gtk_label_new (_("Genre:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	td->genre = gtk_combo_new ();
	gtk_box_pack_start (GTK_BOX (hbox), td->genre, TRUE, TRUE, 0);

	/* Year */
	label = gtk_label_new (_("Year:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	td->year = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), td->year, FALSE, FALSE, 0);

	sep = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (td->parent), sep, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (td->parent), hbox, TRUE, TRUE, 0);
	
	/* Tracks */
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (hbox), sw, TRUE, TRUE, 2);

	td->model = make_tree_model ();
	td->tracks = gtk_tree_view_new_with_model (td->model);
	g_object_unref (td->model);

	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (" ", cell, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (td->tracks), col);
	
	col = gtk_tree_view_column_new_with_attributes (_("Title"), cell,
							"text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (td->tracks), col);

	col = gtk_tree_view_column_new_with_attributes (_("Length"), cell,
							"text", 2, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (td->tracks), col);

	gtk_container_add (GTK_CONTAINER (sw), td->tracks);

	sep = gtk_vseparator_new ();
	gtk_box_pack_start (GTK_BOX (hbox), sep, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	
	/* More advanced options */
	advanced = cddb_disclosure_new (_("Show advanced track options"),
					_("Hide advanced track options"));
	gtk_box_pack_start (GTK_BOX (vbox), advanced, FALSE, FALSE, 0);

	ad_vbox2 = gtk_vbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), ad_vbox2, TRUE, TRUE, 0);
	cddb_disclosure_set_container (CDDB_DISCLOSURE (advanced), ad_vbox2);
	
	/* Extra data */
	label = gtk_label_new (_("Extra track data:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (ad_vbox2), label, FALSE, FALSE, 0);

	td->extra_info = gtk_text_view_new ();
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (td->extra_info),
				     GTK_WRAP_WORD);
	gtk_box_pack_start (GTK_BOX (ad_vbox2), td->extra_info, TRUE, TRUE, 0);
	
	/* Special show hide all the stuff we want */
	gtk_widget_show_all (td->parent);
	gtk_widget_hide (ad_vbox);
	gtk_widget_hide (ad_vbox2);
	
	return td;
}

int
main (int argc,
      char **argv)
{
	GtkWidget *window;
	TrackEditorDialog *td;
	
	gnome_program_init ("CDDBSlave2 Track Editor", VERSION,
			    LIBGNOMEUI_MODULE, argc, argv, NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	td = make_track_editor_control ();
	gtk_container_add (GTK_CONTAINER (window), td->parent);

	gtk_widget_show (window);
	gtk_main ();
}

#if 0
static gboolean
track_editor_init (gpointer data)
{
	BonoboGenericFactory *factory;

	factory = bonobo_generic_factory_new (CDDBSLAVE_TRACK_EDITOR_IID,
					      make_track_editor_control, NULL);
	if (factory == NULL) {
		g_error ("Cannot create factory");
		exit (1);
	}

	bonobo_running_context_auto_exit_unref (BONOBO_OBJECT (factory));
	return FALSE;
}

int
main (int argc,
      char **argv)
{
	CORBA_ORB orb;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init ("CDDBSlave2 Track Editor", VERSION, LIBGNOMEUI_MODULE,
			    argc, argv, NULL);

	g_idle_add (track_editor_init, NULL);
	bonobo_main ();

	exit (0);
}
#endif
