#include <config.h>
#include <gnome.h>
#include <sys/types.h>
#include <string.h>
#include <linux/cdrom.h>
#include "linux-cdrom.h"

#include "gtcd_public.h"

GtkWidget *trwin;

void destroy_window(GtkWidget *widget, gboolean save);
void update_list(GtkWidget *list, int track);
void fill_list(GtkWidget *list);
void dtitle_changed(GtkWidget *widget, gpointer data);
void activate_entry(GtkWidget *widget, GtkWidget *list);
void select_row_cb(GtkCList *clist,
		   gint row,
		   gint column,
		   GdkEventButton *event,
		   GtkWidget *entry);
void edit_window(GtkWidget *widget, gpointer data);

void destroy_window (GtkWidget *widget, gboolean save)
{
	if( save )
		tcd_writediskinfo(&cd);
        gtk_widget_destroy(trwin);
	trwin = NULL;
	make_goto_menu();
	return;
}

void update_list( GtkWidget *list, int track )
{
	gtk_clist_freeze(GTK_CLIST(list));
	gtk_clist_set_text(GTK_CLIST(list), track-1, 2, cd.trk[track].name );
	gtk_clist_thaw(GTK_CLIST(list));
	return;
}

void fill_list( GtkWidget *list )
{
	int i;
	char *tmp[4];
	
	for(i=0; i < 4; i++)
		tmp[i] = g_malloc(255);

	gtk_clist_freeze(GTK_CLIST(list));

	for( i=1; i <= cd.last_t; i++ )
	{
		g_snprintf(tmp[0], 255, "%d", i);
		g_snprintf(tmp[1], 255, "%2d:%02d",
			cd.trk[i].tot_min, cd.trk[i].tot_sec);
		strncpy(tmp[2], cd.trk[i].name, 255);
		tmp[3] = NULL;
	
		gtk_clist_append(GTK_CLIST(list), tmp);
		g_snprintf(tmp[0],255, "%d", i);
	}

	gtk_clist_thaw(GTK_CLIST(list));
	for(i=0; i < 4; i++)
		g_free(tmp[i]);
		
	return;
}

void dtitle_changed( GtkWidget *widget, gpointer data )
{
	strncpy(cd.dtitle, gtk_entry_get_text(GTK_ENTRY(widget)), DISC_INFO_LEN);
	parse_dtitle(&cd);
	return;
}

void activate_entry( GtkWidget *widget, GtkWidget *list )
{
	int trk = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(list)));
	strncpy(cd.trk[trk].name, 
		gtk_entry_get_text(GTK_ENTRY(widget)), TRK_NAME_LEN);
	update_list(list, trk);
	return;
}

void next_entry( GtkWidget *widget, GtkWidget *list )
{
	int trk = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(list)));

	if (trk<cd.last_t)
           gtk_clist_select_row(GTK_CLIST(list), trk, 0);
}

void select_row_cb( GtkCList *clist,
		    gint row,
                    gint column,
                    GdkEventButton *event,
                    GtkWidget *entry)
{
	row++;

	gtk_object_set_user_data(GTK_OBJECT(clist), GINT_TO_POINTER(row));
	gtk_entry_set_text(GTK_ENTRY(entry), cd.trk[row].name);

	if(gtk_events_pending())
		gtk_main_iteration();
	if( event && event->type == GDK_2BUTTON_PRESS )
		tcd_playtracks(&cd, row, -1);

	return;
}
	
void edit_window(GtkWidget *widget, gpointer data)
{
	char *titles[] = {N_("Trk"),N_("Time"),N_("Title")};
	int i;

	GtkWidget *disc_entry, *disc_ext;
	GtkWidget *label, *disc_frame, *button_box;
	GtkWidget *main_box, *disc_table, *button;

	GtkWidget *track_list, *track_frame;
	GtkWidget *track_vbox, *track_entry;
	GtkWidget *track_ext, *entry_box;
	
	if( trwin )
		return;

	trwin = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_container_border_width(GTK_CONTAINER(trwin), 5);
	gtk_window_set_title(GTK_WINDOW(trwin), _("Track Editor"));
	gtk_window_set_wmclass(GTK_WINDOW(trwin), "track_editor","gtcd");

	gtk_signal_connect(GTK_OBJECT(trwin), "delete_event",
		GTK_SIGNAL_FUNC(destroy_window), trwin);
	
	main_box = gtk_vbox_new(FALSE, 4);

	/* Disc area */
	disc_table  = gtk_table_new(2, 2, FALSE);
	disc_frame = gtk_frame_new(_("Disc Information"));
	label 	   = gtk_label_new(_("Artist / Title"));
	disc_ext   = gtk_button_new_with_label(_("Ext Data"));
	gtk_widget_set_sensitive(disc_ext, FALSE);
	disc_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(disc_entry), cd.dtitle);

	gtk_table_attach_defaults(GTK_TABLE(disc_table), label, 
		0,1, 0,1 );
	gtk_table_attach_defaults(GTK_TABLE(disc_table), disc_entry, 
		0,1, 1,2 );
	gtk_table_attach_defaults(GTK_TABLE(disc_table), disc_ext, 
		1,2, 1,2 );

	gtk_container_add(GTK_CONTAINER(disc_frame), disc_table);
	gtk_box_pack_start_defaults(GTK_BOX(main_box), disc_frame);
	/* END Disc area */
	
	/* Track area */
	track_vbox  = gtk_vbox_new(FALSE, 2);
	entry_box   = gtk_hbox_new(FALSE, 2);
	track_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(track_entry), cd.trk[1].name);
	track_ext   = gtk_button_new_with_label(_("Ext Data"));
	gtk_widget_set_sensitive(track_ext, FALSE);
	track_list = gtk_clist_new(3);
	for (i=0 ; i < 3 ; i++)
		gtk_clist_set_column_title(GTK_CLIST(track_list), i, _(titles[i]));
	gtk_clist_column_titles_show(GTK_CLIST(track_list));
	gtk_clist_set_border(GTK_CLIST(track_list), GTK_SHADOW_NONE);
	/* FIXME: Setting the column size needs to be redone in a way that
	   supports i18n */
	gtk_clist_set_column_width(GTK_CLIST(track_list), 0, 20);
	gtk_clist_set_column_width(GTK_CLIST(track_list), 1, 36);
	gtk_clist_set_policy(GTK_CLIST(track_list), GTK_POLICY_AUTOMATIC,
						    GTK_POLICY_AUTOMATIC);
	gtk_clist_set_selection_mode(GTK_CLIST(track_list), 
						    GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_passive(GTK_CLIST(track_list));
	gtk_widget_set_usize(track_list, 150, 225 );
	fill_list(track_list);

	track_frame = gtk_frame_new(_("Track Information"));
	
	gtk_box_pack_start_defaults(GTK_BOX(entry_box), track_entry);
	gtk_box_pack_start_defaults(GTK_BOX(entry_box), track_ext);
	gtk_box_pack_start_defaults(GTK_BOX(track_vbox), entry_box);
	gtk_box_pack_start_defaults(GTK_BOX(track_vbox), track_list);
	gtk_container_add(GTK_CONTAINER(track_frame), track_vbox);
	gtk_box_pack_start_defaults(GTK_BOX(main_box), track_frame);	
	/* END Track area */

	button_box = gtk_hbox_new(FALSE,2);
	/* Clear button */
	button = gtk_button_new_with_label(_("Clear"));
	gtk_widget_set_sensitive(button, FALSE);
	gtk_box_pack_start_defaults(GTK_BOX(button_box), button);
	/* Playlist button */
	button = gtk_button_new_with_label(_("Playlist"));
	gtk_widget_set_sensitive(button, FALSE);
	gtk_box_pack_start_defaults(GTK_BOX(button_box), button);
	gtk_box_pack_start_defaults(GTK_BOX(main_box), button_box);

	button_box = gtk_hbox_new(TRUE,2);
	button = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
	gtk_box_pack_start_defaults(GTK_BOX(button_box), button);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		GTK_SIGNAL_FUNC(destroy_window), (gpointer)TRUE);
	button = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
	gtk_box_pack_start_defaults(GTK_BOX(button_box), button);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		GTK_SIGNAL_FUNC(destroy_window), (gpointer)FALSE);
	gtk_box_pack_start_defaults(GTK_BOX(main_box), button_box);

	gtk_signal_connect(GTK_OBJECT(track_list), "select_row",
		GTK_SIGNAL_FUNC(select_row_cb), track_entry);
	gtk_signal_connect(GTK_OBJECT(disc_entry), "changed",
		GTK_SIGNAL_FUNC(dtitle_changed), NULL);
	gtk_signal_connect(GTK_OBJECT(track_entry), "changed",
		GTK_SIGNAL_FUNC(activate_entry), track_list);
	gtk_signal_connect(GTK_OBJECT(track_entry), "activate",
		GTK_SIGNAL_FUNC(next_entry), track_list);
	
	gtk_container_add(GTK_CONTAINER(trwin), main_box);
	gtk_widget_show_all(trwin);
	return;
}
