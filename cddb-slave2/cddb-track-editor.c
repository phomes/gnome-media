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
#include "cddb-slave-client.h"
#include "GNOME_Media_CDDBSlave2.h"

#define CDDBSLAVE_TRACK_EDITOR_IID "OAFIID:GNOME_Media_CDDBSlave2_TrackEditorFactory"

static CDDBSlaveClient *client = NULL;
static int running_objects = 0;

typedef struct _CDDBInfo {
	char *discid;
	
	char *title;
	char *artist;
	char *comment;
	char *genre;
	int year;
	
	int ntrks;
	CDDBSlaveClientTrackInfo **track_info;
} CDDBInfo;

typedef struct _TrackEditorDialog {
	gboolean dirty;
	
	GtkWidget *parent;
	GtkWidget *artist;
	GtkWidget *discid;
	GtkWidget *disctitle;
	GtkWidget *disccomments;
	GtkWidget *year;
	GtkWidget *genre;
	GtkWidget *tracks;
	GtkWidget *extra_info;
	
	GtkTextBuffer *buffer;
	GtkTreeModel *model;

	CDDBInfo *info;
} TrackEditorDialog;

static char *genres[] = {
	N_("Blues"),
	N_("Classical Rock"),
	N_("Country"),
	N_("Dance"),
	N_("Disco"),
	N_("Funk"),
	N_("Grunge"),
	N_("Hip-Hop"),
	N_("Jazz"),
	N_("Metal"),
	N_("New Age"),
	N_("Oldies"),
	N_("Other"),
	N_("Pop"),
	N_("R&B"),
	N_("Rap"),
	N_("Reggae"),
	N_("Rock"),
	N_("Techno"),
	N_("Industrial"),
	N_("Alternative"),
	N_("Ska"),
	N_("Death Metal"),
	N_("Pranks"),
	N_("Soundtrack"),
	N_("Euro-Techno"),
	N_("Ambient"),
	N_("Trip-Hop"),
	N_("Vocal"),
	N_("Jazz+Funk"),
	N_("Fusion"),
	N_("Trance"),
	N_("Classical"),
	N_("Instrumental"),
	N_("Acid"),
	N_("House"),
	N_("Game"),
	N_("Sound Clip"),
	N_("Gospel"),
	N_("Noise"),
	N_("Alt"),
	N_("Bass"),
	N_("Soul"),
	N_("Punk"),
	N_("Space"),
	N_("Meditative"),
	N_("Instrumental Pop"),
	N_("Instrumental Rock"),
	N_("Ethnic"),
	N_("Gothic"),
	N_("Darkwave"),
	N_("Techno-Industrial"),
	N_("Electronic"),
	N_("Pop-Folk"),
	N_("Eurodance"),
	N_("Dream"),
	N_("Southern Rock"),
	N_("Comedy"),
	N_("Cult"),
	N_("Gangsta Rap"),
	N_("Top 40"),
	N_("Christian Rap"),
	N_("Pop/Funk"),
	N_("Jungle"),
	N_("Native American"),
	N_("Cabaret"),
	N_("New Wave"),
	N_("Psychedelic"),
	N_("Rave"),
	N_("Showtunes"),
	N_("Trailer"),
	N_("Lo-Fi"),
	N_("Tribal"),
	N_("Acid Punk"),
	N_("Acid Jazz"),
	N_("Polka"),
	N_("Retro"),
	N_("Musical"),
	N_("Rock & Roll"),
	N_("Hard Rock"),
	N_("Folk"),
	N_("Folk/Rock"),
	N_("National Folk"),
	N_("Swing"),
	N_("Fast-Fusion"),
	N_("Bebop"),
	N_("Latin"),
	N_("Revival"),
	N_("Celtic"),
	N_("Bluegrass"),
	N_("Avantgarde"),
	N_("Gothic Rock"),
	N_("Progressive Rock"),
	N_("Psychedelic Rock"),
	N_("Symphonic Rock"),
	N_("Slow Rock"),
	N_("Big Band"),
	N_("Chorus"),
	N_("Easy Listening"),
	N_("Acoustic"),
	N_("Humour"),
	N_("Speech"),
	N_("Chanson"),
	N_("Opera"),
	N_("Chamber Music"),
	N_("Sonata"),
	N_("Symphony"),
	N_("Booty Bass"),
	N_("Primus"),
	N_("Porn Groove"),
	N_("Satire"),
	N_("Slow Jam"),
	N_("Club"),
	N_("Tango"),
	N_("Samba"),
	N_("Folklore"),
	N_("Ballad"),
	N_("Power Ballad"),
	N_("Rythmic Soul"),
	N_("Freestyle"),
	N_("Duet"),
	N_("Punk Rock"),
	N_("Drum Solo"),
	N_("A Cappella"),
	N_("Euro-House"),
	N_("Dance Hall"),
	N_("Goa"),
	N_("Drum & Bass"),
	N_("Club-House"),
	N_("Hardcore"),
	N_("Terror"),
	N_("Indie"),
	N_("BritPop"),
	N_("Negerpunk"),
	N_("Polsk Punk"),
	N_("Beat"),
	N_("Christian Gangsta Rap"),
	N_("Heavy Metal"),
	N_("Black Metal"),
	N_("Crossover"),
	N_("Contemporary Christian"),
	N_("Christian Rock"),
	N_("Merengue"),
	N_("Salsa"),
	N_("Thrash Metal"),
	N_("Anime"),
	N_("JPop"),
	N_("Synthpop"),
	N_("Nu-Metal"),
	N_("Art Rock"),
	NULL
};

static GtkTreeModel *
make_tree_model (void)
{
	GtkListStore *store;
	GtkTreeIter iter;
	int i;
	
	store = gtk_list_store_new (4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

	return GTK_TREE_MODEL (store);
}

static char *
secs_to_string (int seconds)
{
	int min, sec;

	min = seconds / 60;
	sec = seconds % 60;

	return g_strdup_printf ("%d:%02d", min, sec);
}

static void
build_track_list (TrackEditorDialog *td)
{
	GtkTreeIter iter;
	GtkListStore *store = GTK_LIST_STORE (td->model);
	int i;
	
	g_return_if_fail (td->info != NULL);
	
	for (i = 0; i < td->info->ntrks; i++) {
		char *length;

		length = secs_to_string (td->info->track_info[i]->length);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, i + 1,
				    1, td->info->track_info[i]->name,
				    2, length, -1);
		g_free (length);
	}
}

static void
clear_track_list (TrackEditorDialog *td)
{
	gtk_list_store_clear (GTK_LIST_STORE (td->model));
}

static void
free_track_info (TrackEditorDialog *td)
{
	CDDBInfo *info;

	info = td->info;
	
	g_free (info->discid);
	g_free (info->title);
	g_free (info->artist);
	g_free (info->comment);
	g_free (info->genre);

	cddb_slave_client_free_track_info (info->track_info);
	
	g_free (info);
	td->info = NULL;
}

static void
load_new_track_data (TrackEditorDialog *td,
		     const char *discid)
{
	CDDBInfo *info;
	char *title;
	
	if (client == NULL) {
		client = cddb_slave_client_new ();
	}

	info = g_new (CDDBInfo, 1);
	td->info = info;

	info->discid = g_strdup (discid);
	info->title = cddb_slave_client_get_disc_title (client, discid);
	info->artist = cddb_slave_client_get_artist (client, discid);
	info->comment = cddb_slave_client_get_comment (client, discid);
	info->genre = cddb_slave_client_get_genre (client, discid);
	info->year = cddb_slave_client_get_year (client, discid);
	info->ntrks = cddb_slave_client_get_ntrks (client, discid);
	info->track_info = cddb_slave_client_get_tracks (client, discid);

	title = g_strdup_printf (_("Editting discid: %s"), info->discid);
	gtk_label_set_text (GTK_LABEL (td->discid), title);
	g_free (title);

	if (info->title != NULL) {
		gtk_entry_set_text (GTK_ENTRY (td->disctitle), info->title);
	}

	if (info->artist != NULL) {
		gtk_entry_set_text (GTK_ENTRY (td->artist), info->artist);
	}

	if (info->year != -1) {
		char *year;

		year = g_strdup_printf ("%d", info->year);
		gtk_entry_set_text (GTK_ENTRY (td->year), year);
		g_free (year);
	}

	if (info->comment != NULL) {
		gtk_entry_set_text (GTK_ENTRY (td->disccomments), info->comment);
	}

	if (info->genre != NULL) {
		gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (td->genre)->entry), info->genre);
	} else {
		gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (td->genre)->entry), "");
	}
	
	/* Rebuild track list */
	clear_track_list (td);
	build_track_list (td);

	td->dirty = FALSE;
}

static GList *
make_genre_list (void)
{
	GList *genre_list = NULL;
	int i;
	
	for (i = 0; genres[i]; i++) {
		genre_list = g_list_prepend (genre_list, genres[i]);
	}

	genre_list = g_list_sort (genre_list, strcmp);
	
	return genre_list;
}

static void
track_selection_changed (GtkTreeSelection *selection,
			 TrackEditorDialog *td)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	int track;

	if (gtk_tree_selection_get_selected (selection, &model, &iter) == TRUE) {
		char *comment;
		
		gtk_tree_model_get (model, &iter, 0, &track, -1);
		comment = td->info->track_info[track - 1]->comment;

		if (comment != NULL) {
			gtk_text_buffer_set_text (td->buffer, comment, -1);
		} else {
			gtk_text_buffer_set_text (td->buffer, "", -1);
		}
	}
}

static void
artist_changed (GtkEntry *entry,
		TrackEditorDialog *td)
{
	const char *artist;

	if (td->info == NULL) {
		return;
	}
		       
	artist = gtk_entry_get_text (entry);
	if (td->info->artist != NULL) {
		g_free (td->info->artist);
	}
	td->info->artist = g_strdup (artist);
	td->dirty = TRUE;
}

static void
disctitle_changed (GtkEntry *entry,
		   TrackEditorDialog *td)
{
	const char *title;

	if (td->info == NULL) {
		return;
	}
	
	title = gtk_entry_get_text (entry);
	if (td->info->title != NULL) {
		g_free (td->info->title);
	}

	td->info->title = g_strdup (title);
	td->dirty = TRUE;
}

static void
year_changed (GtkEntry *entry,
	      TrackEditorDialog *td)
{
	const char *year;

	if (td->info == NULL) {
		return;
	}
		      
	year = gtk_entry_get_text (entry);

	if (year != NULL) {
		td->info->year = atoi (year);
	} else {
		td->info->year = -1;
	}

	td->dirty = TRUE;
}

static void
genre_changed (GtkEntry *entry,
	       TrackEditorDialog *td)
{
	const char *genre;

	if (td->info == NULL) {
		return;
	}
	
	genre = gtk_entry_get_text (entry);

	if (td->info->genre != NULL) {
		g_free (td->info->genre);
	}

	td->info->genre = g_strdup (genre);
	td->dirty = TRUE;
}

static void
comment_changed (GtkEntry *entry,
		 TrackEditorDialog *td)
{
	char *comment;

	if (td->info == NULL) {
		return;
	}
	
	comment = gtk_entry_get_text (entry);

	if (td->info->comment != NULL) {
		g_free (td->info->comment);
	}

	td->info->comment = g_strdup (comment);
	td->dirty = TRUE;
}

static TrackEditorDialog *
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
	GtkTreeSelection *selection;
	
	td = g_new0 (TrackEditorDialog, 1);

	td->info = NULL;
	td->dirty = FALSE;
	
	td->parent = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (td->parent), 2);

	/* Info label */
	td->discid = gtk_label_new (_("Editting discid: "));
	gtk_misc_set_alignment (GTK_MISC (td->discid), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (td->parent), td->discid, FALSE, FALSE, 0);

	sep = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (td->parent), sep, FALSE, FALSE, 0);

	/* Artist */
	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (td->parent), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("_Artist:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	td->artist = gtk_entry_new ();
	g_signal_connect (G_OBJECT (td->artist), "changed",
			  G_CALLBACK (artist_changed), td);
	gtk_box_pack_start (GTK_BOX (hbox), td->artist, TRUE, TRUE, 0);

	/* Disc title */
	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (td->parent), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("Disc _Title:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	td->disctitle = gtk_entry_new ();
	g_signal_connect (G_OBJECT (td->disctitle), "changed",
			  G_CALLBACK (disctitle_changed), td);
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
	g_signal_connect (G_OBJECT (td->disccomments), "changed",
			  G_CALLBACK (comment_changed), td);
	gtk_box_pack_start (GTK_BOX (hbox), td->disccomments, TRUE, TRUE, 0);

	/* Bottom box */
	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (ad_vbox), hbox, FALSE, FALSE, 0);

	/* Genre */
	label = gtk_label_new (_("Genre:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	td->genre = gtk_combo_new ();
	g_signal_connect (G_OBJECT (GTK_COMBO (td->genre)->entry), "changed",
			  G_CALLBACK (genre_changed), td);
	gtk_combo_set_popdown_strings (GTK_COMBO (td->genre),
				       make_genre_list ());
	gtk_combo_set_value_in_list (GTK_COMBO (td->genre), FALSE, TRUE);
	
	gtk_box_pack_start (GTK_BOX (hbox), td->genre, TRUE, TRUE, 0);

	/* Year */
	label = gtk_label_new (_("Year:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	td->year = gtk_entry_new ();
	g_signal_connect (G_OBJECT (td->year), "changed",
			  G_CALLBACK (year_changed), td);
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

	selection = gtk_tree_view_get_selection (td->tracks);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (track_selection_changed), td);
	
	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (" ", cell, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (td->tracks), col);
	
	col = gtk_tree_view_column_new_with_attributes (_("Title"), cell,
							"text", 1,
							"editable", 3,
							NULL);
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
	td->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (td->extra_info));
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (td->extra_info),
				     GTK_WRAP_WORD);
	gtk_box_pack_start (GTK_BOX (ad_vbox2), td->extra_info, TRUE, TRUE, 0);
	
	/* Special show hide all the stuff we want */
	gtk_widget_show_all (td->parent);
	gtk_widget_hide (ad_vbox);
	gtk_widget_hide (ad_vbox2);

	return td;
}

/* Implement GNOME::Media::CDDBTrackEditor */
#define PARENT_TYPE BONOBO_OBJECT_TYPE
static BonoboObjectClass *parent_class = NULL;

typedef struct _CDDBTrackEditor {
	BonoboObject parent;
	
	TrackEditorDialog *td;
	GtkWidget *dialog;
	char *discid;
} CDDBTrackEditor;

typedef struct _CDDBTrackEditorClass {
	BonoboObjectClass parent_class;

	POA_GNOME_Media_CDDBTrackEditor__epv epv;
} CDDBTrackEditorClass;

static inline CDDBTrackEditor *
cddb_track_editor_from_servant (PortableServer_Servant servant)
{
	return (CDDBTrackEditor *) bonobo_object_from_servant (servant);
}

static void
dialog_response (GtkDialog *dialog,
		 int response_id,
		 CDDBTrackEditor *editor)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
impl_GNOME_Media_CDDBTrackEditor_showWindow (PortableServer_Servant servant,
					     CORBA_Environment *ev)
{
	CDDBTrackEditor *editor;

	editor = cddb_track_editor_from_servant (servant);
	if (editor->dialog == NULL) {
		editor->td = make_track_editor_control ();
		if (editor->discid != NULL) {
			load_new_track_data (editor->td, editor->discid);
		}
		
		editor->dialog = gtk_dialog_new_with_buttons (_("CDDB Track Editor"),
							      NULL, 0,
							      GTK_STOCK_CANCEL,
							      GTK_RESPONSE_CANCEL,
							      GTK_STOCK_SAVE,
							      1, NULL);
		g_signal_connect (G_OBJECT (editor->dialog), "response",
				  G_CALLBACK (dialog_response), editor);
		
		gtk_window_set_default_size (GTK_WINDOW (editor->dialog), 640, 400);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (editor->dialog)->vbox),
				    editor->td->parent, TRUE, TRUE, 0);
		gtk_widget_show (editor->dialog);
	} else {
		gdk_window_show (editor->dialog->window);
		gdk_window_raise (editor->dialog->window);
	}
}

static void
impl_GNOME_Media_CDDBTrackEditor_setDiscID (PortableServer_Servant servant,
					    const CORBA_char *discid,
					    CORBA_Environment *ev)
{
	CDDBTrackEditor *editor;

	editor = cddb_track_editor_from_servant (servant);
	if (editor->discid != NULL &&
	    strcmp (editor->discid, discid) == 0) {
		return;
	}

	if (editor->discid != NULL) {
		g_free (editor->discid);
	}
	editor->discid = g_strdup (discid);

	if (editor->td != NULL) {
		if (editor->td->info != NULL) {
			free_track_info (editor->td);
		}
		load_new_track_data (editor->td, discid);
	}	       
}

static void
finalise (GObject *object)
{
	CDDBTrackEditor *editor;

	editor = (CDDBTrackEditor *) object;

	if (editor->dialog == NULL) {
		return;
	}
	
	gtk_widget_destroy (editor->dialog);
	editor->dialog = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cddb_track_editor_class_init (CDDBTrackEditorClass *klass)
{
	GObjectClass *object_class;
	POA_GNOME_Media_CDDBTrackEditor__epv *epv = &klass->epv;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = finalise;

	parent_class = g_type_class_peek_parent (klass);

	epv->showWindow = impl_GNOME_Media_CDDBTrackEditor_showWindow;
	epv->setDiscID = impl_GNOME_Media_CDDBTrackEditor_setDiscID;
}

static void
cddb_track_editor_init (CDDBTrackEditor *editor)
{
	editor->td = NULL;
	editor->dialog = NULL;
	editor->discid = NULL;
}

BONOBO_TYPE_FUNC_FULL (CDDBTrackEditor, GNOME_Media_CDDBTrackEditor,
		       PARENT_TYPE, cddb_track_editor);

static void
track_editor_destroy_cb (GObject *editor,
			 gpointer data)
{
	running_objects--;

	if (running_objects <= 0) {
		if (client != NULL) {
			g_object_unref (G_OBJECT (client));
		}
		
		bonobo_main_quit ();
	}
}

static BonoboObject *
factory_fn (BonoboGenericFactory *factory,
	    const char *component_id,
	    void *closure)
{
	CDDBTrackEditor *editor;

	editor = g_object_new (cddb_track_editor_get_type (), NULL);

	/* Keep track of our objects */
	running_objects++;
	g_signal_connect (G_OBJECT (editor), "destroy",
			  G_CALLBACK (track_editor_destroy_cb), NULL);

	return BONOBO_OBJECT (editor);
}

static gboolean
track_editor_init (gpointer data)
{
	BonoboGenericFactory *factory;

	factory = bonobo_generic_factory_new (CDDBSLAVE_TRACK_EDITOR_IID,
					      factory_fn, NULL);
	if (factory == NULL) {
		g_print (_("Cannot create CDDBTrackEditor factory.\n"
			   "This may be caused by another copy of cddb-track-editor already running.\n"));
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
