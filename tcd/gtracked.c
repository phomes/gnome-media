#include <config.h>
#include <gnome.h>
#include <sys/types.h>
#include <string.h>
#include "linux-cdrom.h"

#include "gtcd_public.h"

GtkWidget *trwin;
static gint signal_id;

static void destroy_window(GtkWidget *widget, gboolean save);
static void update_list(GtkWidget *list, int track);
static void fill_list(GtkWidget *list);
static void dtitle_changed(GtkWidget *widget, gpointer data);
static void activate_entry(GtkWidget *widget, GtkWidget *list);
static void select_row_cb(GtkCList *clist,
			  gint row,
			  gint column,
			  GdkEventButton *event,
			  GtkWidget *trwin);
static void next_entry(GtkWidget *widget, GtkWidget *list);

static void edit_disc_extra(GtkWidget *widget);
static void edit_track_extra(GtkWidget *widget, GtkWidget *list);

static void destroy_window (GtkWidget *widget, gboolean save)
{
    if(save)
	tcd_writediskinfo(&cd);
    gtk_widget_destroy(trwin);
    trwin = NULL;
    make_goto_menu();
    return;
}

void update_editor(void)
{
    GtkWidget *clist, *dtitle, *track;

    if(!trwin)
	return;			/* return if there is no tracked editor open */

    clist = gtk_object_get_data(GTK_OBJECT(trwin), "clist");
    dtitle= gtk_object_get_data(GTK_OBJECT(trwin), "dtitle");
    track = gtk_object_get_data(GTK_OBJECT(trwin), "track");

    gtk_signal_disconnect(GTK_OBJECT(track), signal_id);
    gtk_entry_set_text(GTK_ENTRY(dtitle), cd.dtitle);
    gtk_entry_set_text(GTK_ENTRY(track), "");
    signal_id = gtk_signal_connect(GTK_OBJECT(track), "changed",
                       GTK_SIGNAL_FUNC(activate_entry), clist);
                           
    gtk_clist_clear(GTK_CLIST(clist));
    fill_list(clist);
}

static void update_list( GtkWidget *list, int track )
{
    gtk_clist_freeze(GTK_CLIST(list));
    gtk_clist_set_text(GTK_CLIST(list), track-1, 2, cd.trk[track].name );
    gtk_clist_thaw(GTK_CLIST(list));
    return;
}

static void fill_list( GtkWidget *list )
{
    int i;
    char *tmp[4];

    for(i=0; i < 4; i++)
	tmp[i] = g_malloc(TRK_NAME_LEN);

    gtk_clist_freeze(GTK_CLIST(list));

    for( i=cd.first_t; i <= cd.last_t; i++ )
    {
	g_snprintf(tmp[0], TRK_NAME_LEN-1, "%d", i);
	g_snprintf(tmp[1], TRK_NAME_LEN-1, "%2d:%02d",
		   cd.trk[i].tot_min, cd.trk[i].tot_sec);
	g_snprintf(tmp[2], TRK_NAME_LEN-1, "%s", cd.trk[i].name);
	tmp[3] = NULL;
	
	gtk_clist_append(GTK_CLIST(list), tmp);
	g_snprintf(tmp[0],255, "%d", i);
    }

    gtk_clist_thaw(GTK_CLIST(list));
    for(i=0; i < 4; i++)
	g_free(tmp[i]);
		
    return;
}

static void dtitle_changed( GtkWidget *widget, gpointer data )
{
    strncpy(cd.dtitle, gtk_entry_get_text(GTK_ENTRY(widget)), DISC_INFO_LEN);
    parse_dtitle(&cd);
    return;
}

static void activate_entry( GtkWidget *widget, GtkWidget *list )
{
    int trk = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(list)));
    strncpy(cd.trk[trk].name, 
	    gtk_entry_get_text(GTK_ENTRY(widget)), TRK_NAME_LEN);
    update_list(list, trk);
    return;
}

static void edit_disc_extra(GtkWidget *widget)
{
    gchar *text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

    strncpy(cd.extd, text, EXT_DATA_LEN);
    g_free(text);
}

static void edit_track_extra(GtkWidget *widget, GtkWidget *list)
{
    int trk = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(list)));
    gchar *text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

    strncpy(cd.trk[trk].extd, text, EXT_DATA_LEN);
    g_free(text);
}

static void next_entry( GtkWidget *widget, GtkWidget *list )
{
    int trk = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(list)));

    if (trk<cd.last_t)
	gtk_clist_select_row(GTK_CLIST(list), trk, 0);
}

static void select_row_cb( GtkCList *clist,
			   gint row,
			   gint column,
			   GdkEventButton *event,
			   GtkWidget *trwin)
{
    GtkWidget *entry = gtk_object_get_data(GTK_OBJECT(trwin), "track");
    GtkWidget *extra = gtk_object_get_data(GTK_OBJECT(trwin), "extt");
    gint pos = 0;

    row++;
    gtk_object_set_user_data(GTK_OBJECT(clist), GINT_TO_POINTER(row));

    gtk_entry_set_text(GTK_ENTRY(entry), cd.trk[row].name);
    gtk_signal_handler_block_by_func(GTK_OBJECT(extra),
				     GTK_SIGNAL_FUNC(edit_track_extra), clist);
    gtk_editable_delete_text(GTK_EDITABLE(extra), 0, -1);
    gtk_editable_insert_text(GTK_EDITABLE(extra), cd.trk[row].extd,
			     strlen(cd.trk[row].extd), &pos);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(extra),
				GTK_SIGNAL_FUNC(edit_track_extra), clist);

    if(gtk_events_pending())
	gtk_main_iteration();
    if(event && event->type == GDK_2BUTTON_PRESS)
	tcd_playtracks(&cd, row, -1, prefs.only_use_trkind);

    return;
}

void edit_window(GtkWidget *widget, gpointer data)
{
    char *titles[] = {N_("Trk"),N_("Time"),N_("Title")};
    char tmp[64];
    int i;

    GtkWidget *disc_entry, *swin;
    GtkWidget *label, *disc_frame, *button_box;
    GtkWidget *main_box, *disc_vbox, *button;

    GtkWidget *track_list, *track_frame;
    GtkWidget *track_vbox, *track_entry;
    GtkWidget *entry_box;
    GtkWidget *hbox, *vbox;

    GtkWidget *disc_extra, *track_extra;
	
    if( trwin )
    {
	destroy_window(trwin, 1);
	return;
    }
    
    trwin = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_container_border_width(GTK_CONTAINER(trwin), 5);
    gtk_window_set_title(GTK_WINDOW(trwin), _("Track Editor"));
    gtk_window_set_wmclass(GTK_WINDOW(trwin), "track_editor","gtcd");
    gtk_widget_set_usize(trwin, 500, 300);

    gtk_signal_connect(GTK_OBJECT(trwin), "delete_event",
		       GTK_SIGNAL_FUNC(destroy_window), trwin);
	
    main_box = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);

    hbox = gtk_hbox_new(TRUE, GNOME_PAD_SMALL);
    gtk_box_pack_start(GTK_BOX(main_box), hbox, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    
    /* Disc area */
    g_snprintf(tmp, 63, _("Disc Information (%02u:%02u minutes)"),
	       cd.trk[cd.last_t+1].toc.cdte_addr.msf.minute,
	       cd.trk[cd.last_t+1].toc.cdte_addr.msf.second);

    disc_frame = gtk_frame_new(tmp);
    disc_vbox  = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
    gtk_container_set_border_width(GTK_CONTAINER(disc_vbox), GNOME_PAD_SMALL);
    gtk_container_add(GTK_CONTAINER(disc_frame), disc_vbox);
	              
    label 	   = gtk_label_new(_("Artist / Title"));
    disc_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(disc_entry), cd.dtitle);

    disc_extra = gtk_text_new(FALSE, FALSE);
    gtk_text_set_word_wrap(GTK_TEXT(disc_extra), TRUE);
    gtk_editable_set_editable(GTK_EDITABLE(disc_extra), TRUE);
    i = 0;
    gtk_editable_insert_text(GTK_EDITABLE(disc_extra), cd.extd,
			     strlen(cd.extd), &i);
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(swin), disc_extra);

    gtk_box_pack_start(GTK_BOX(disc_vbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(disc_vbox), disc_entry, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(disc_vbox), swin, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), disc_frame, TRUE, TRUE, 0);
    /* END Disc area */
	
    /* Track area */
    track_frame = gtk_frame_new(_("Track Information"));
    track_vbox  = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
    gtk_container_set_border_width(GTK_CONTAINER(track_vbox), GNOME_PAD_SMALL);
    gtk_container_add(GTK_CONTAINER(track_frame), track_vbox);

    entry_box   = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
    track_entry = gtk_entry_new();

    track_extra = gtk_text_new(FALSE, FALSE);
    gtk_text_set_word_wrap(GTK_TEXT(track_extra), TRUE);
    gtk_editable_set_editable(GTK_EDITABLE(track_extra), TRUE);
    /*gtk_editable_insert_text(GTK_EDITABLE(track_extra), cd.extd,
      strlen(cd.extd), NULL);*/
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(swin), track_extra);
    /* ... */

    gtk_box_pack_start(GTK_BOX(track_vbox), track_entry, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(track_vbox), swin, TRUE, TRUE, 0);
	
    gtk_box_pack_start(GTK_BOX(vbox), track_frame, TRUE, TRUE, 0);
    /* END Track area */

    track_list = gtk_clist_new(3);
    for (i=0 ; i < 3 ; i++)
	gtk_clist_set_column_title(GTK_CLIST(track_list), i, _(titles[i]));
    gtk_clist_column_titles_show(GTK_CLIST(track_list));
    gtk_clist_set_shadow_type(GTK_CLIST(track_list), GTK_SHADOW_NONE);
    /* FIXME: Setting the column size needs to be redone in a way that
       supports i18n */
    gtk_clist_set_column_width(GTK_CLIST(track_list), 0, 20);
    gtk_clist_set_column_width(GTK_CLIST(track_list), 1, 36);
    gtk_clist_set_selection_mode(GTK_CLIST(track_list), 
				 GTK_SELECTION_BROWSE);
    gtk_clist_column_titles_passive(GTK_CLIST(track_list));

    swin = gtk_scrolled_window_new(GTK_CLIST(track_list)->hadjustment,
				   GTK_CLIST(track_list)->vadjustment);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(swin), track_list);

    gtk_box_pack_start(GTK_BOX(hbox), swin, TRUE, TRUE, 0);

    gtk_object_set_data(GTK_OBJECT(trwin), "dtitle", disc_entry);
    gtk_object_set_data(GTK_OBJECT(trwin), "clist", track_list);
    gtk_object_set_data(GTK_OBJECT(trwin), "track", track_entry);
    gtk_object_set_data(GTK_OBJECT(trwin), "extd", disc_extra);
    gtk_object_set_data(GTK_OBJECT(trwin), "extt", track_extra);

    button_box = gtk_hbox_new(TRUE, GNOME_PAD_SMALL);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 0);

#if 0
    /* Clear button */
    button = gtk_button_new_with_label(_("Clear"));
    gtk_widget_set_sensitive(button, FALSE);
    gtk_box_pack_start_defaults(GTK_BOX(button_box), button);

    /* Playlist button */
    button = gtk_button_new_with_label(_("Playlist"));
    gtk_widget_set_sensitive(button, FALSE);
    gtk_box_pack_start_defaults(GTK_BOX(button_box), button);
#endif

    /* CDDB Status  button */
    button = gtk_button_new_with_label(_("CDDB Status"));
    gtk_widget_set_sensitive(button, TRUE);
    gtk_box_pack_start_defaults(GTK_BOX(button_box), button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(cddb_status_dialog), NULL);

    button = gtk_button_new_with_label(_("Submit"));
    gtk_box_pack_start_defaults(GTK_BOX(button_box), button);
    /* connect the signal.  Setting insensitive is temporary */
    gtk_widget_set_sensitive(button, FALSE);

    button = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
    gtk_box_pack_start_defaults(GTK_BOX(button_box), button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(destroy_window), (gpointer)TRUE);
    button = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
    gtk_box_pack_start_defaults(GTK_BOX(button_box), button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(destroy_window), (gpointer)FALSE);

    gtk_signal_connect(GTK_OBJECT(track_list), "select_row",
		       GTK_SIGNAL_FUNC(select_row_cb), trwin);
    gtk_signal_connect(GTK_OBJECT(disc_entry), "changed",
		       GTK_SIGNAL_FUNC(dtitle_changed), NULL);
    signal_id = gtk_signal_connect(GTK_OBJECT(track_entry), "changed",
		       GTK_SIGNAL_FUNC(activate_entry), track_list);
    gtk_signal_connect(GTK_OBJECT(track_entry), "activate",
		       GTK_SIGNAL_FUNC(next_entry), track_list);

    gtk_signal_connect(GTK_OBJECT(disc_extra), "changed",
		       GTK_SIGNAL_FUNC(edit_disc_extra), NULL);
    gtk_signal_connect(GTK_OBJECT(track_extra), "changed",
		       GTK_SIGNAL_FUNC(edit_track_extra), track_list);

    gtk_container_add(GTK_CONTAINER(trwin), main_box);

    fill_list(track_list);

    gtk_widget_show_all(trwin);
    return;
}
