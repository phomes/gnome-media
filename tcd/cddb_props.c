/* Author: Tim P. Gerla <timg@means.net> */ 
#include <config.h>
#include <gnome.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>

#include "cddb_props.h"

/* prototypes */
static void edit_cb(GtkWidget *widget, GtkWidget *clist);

static GtkWidget *create_local_db(void);
static gchar *get_file_name(gchar *append);
static gchar *get_dtitle(const gchar *filename);
static void select_row_cb(GtkCList *clist,
			  gint row,
			  gint column,
			  GdkEventButton *event,
			  GtkWidget *editbutton);
static void fill_list(GtkWidget *clist);
static void edit_destroy_cb(GtkWidget *widget, EditWindow *w);
static EditWindow *create_edit_window(GtkWidget *clist);
static void refresh_cb(GtkWidget *widget, GtkWidget *clist);
static void remove_cb(GtkWidget *widget, GtkWidget *clist);
static void msg_callback(gint reply, GtkCList *clist);

/* code */
static void select_row_cb(GtkCList *clist,
			  gint row,
			  gint column,
			  GdkEventButton *event,
			  GtkWidget *editbutton)
{
    static gchar *filename=NULL;
    gchar *tmp;

    gtk_widget_set_sensitive(editbutton, TRUE);

    if(filename)
	g_free(filename);

    gtk_clist_get_text(clist, row, 0, &tmp);
    filename = get_file_name(tmp);
    gtk_object_set_data(GTK_OBJECT(clist), "filename", filename);
    gtk_object_set_data(GTK_OBJECT(clist), "row", GINT_TO_POINTER(row));
}

static GtkWidget *create_local_db(void)
{
    const gchar *titles[] = {N_("Disc ID"), N_("Disc Title")};
    GtkWidget *clist, *refresh_button, *edit_button, *hsep;
    GtkWidget *hbox, *bbox, *scw, *remove_button;

    hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
    
    /* buttons */
    bbox = gtk_vbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_START);

    edit_button = gtk_button_new_with_label("Edit");
    gtk_widget_set_sensitive(edit_button, FALSE);
    gtk_box_pack_start_defaults(GTK_BOX(bbox), edit_button);

    remove_button = gtk_button_new_with_label("Remove");
    gtk_box_pack_start_defaults(GTK_BOX(bbox), remove_button);

    hsep = gtk_hseparator_new();
    gtk_box_pack_start_defaults(GTK_BOX(bbox), hsep);

    refresh_button = gtk_button_new_with_label("Refresh List");
    gtk_box_pack_start_defaults(GTK_BOX(bbox), refresh_button);

    gtk_box_pack_end(GTK_BOX(hbox), bbox, FALSE, FALSE, GNOME_PAD_SMALL);

    /* clist */
    clist = gtk_clist_new_with_titles(2, titles);
    gtk_clist_column_titles_show(GTK_CLIST(clist));
    gtk_clist_set_selection_mode(GTK_CLIST(clist), 
                                 GTK_SELECTION_BROWSE); 
    gtk_clist_column_titles_passive(GTK_CLIST(clist));
    gtk_clist_set_sort_column(GTK_CLIST(clist), 1);
    gtk_clist_set_auto_sort(GTK_CLIST(clist), TRUE);

    scw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scw),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_clist_set_column_width(GTK_CLIST(clist), 0, 64);

    gtk_container_add(GTK_CONTAINER(scw), clist);
    gtk_box_pack_start(GTK_BOX(hbox), scw, TRUE, TRUE, GNOME_PAD_SMALL);
    gtk_signal_connect(GTK_OBJECT(clist), "select_row",
		       GTK_SIGNAL_FUNC(select_row_cb), edit_button);
    fill_list(clist);
    
    /* now that we have a clist to pass... */
    gtk_signal_connect(GTK_OBJECT(edit_button), "clicked",
		       GTK_SIGNAL_FUNC(edit_cb), clist);
    gtk_signal_connect(GTK_OBJECT(refresh_button), "clicked",
		       GTK_SIGNAL_FUNC(refresh_cb), clist);
    gtk_signal_connect(GTK_OBJECT(remove_button), "clicked",
		       GTK_SIGNAL_FUNC(remove_cb), clist);

    gtk_widget_show_all(hbox);
    return hbox;
}

GtkWidget *create_cddb_page(void)
{
    GtkWidget *vbox, *frame, *label;
    GtkWidget *table, *entry;
    GtkObject *adj;

    vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);

    /* server settings */
    frame = gtk_frame_new("Server Settings");
    gtk_container_border_width(GTK_CONTAINER(frame), GNOME_PAD_SMALL);

    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

    table = gtk_table_new(2, 4, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), GNOME_PAD_SMALL);
    gtk_table_set_col_spacings(GTK_TABLE(table), GNOME_PAD_SMALL);

    label = gtk_label_new("Address");
    entry = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 0, 1);

    adj = gtk_adjustment_new(8880, 0, 65536, 1, 100, 10);
    label = gtk_label_new("Port");
    entry = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 3, 4, 0, 1);

    gtk_container_add(GTK_CONTAINER(frame), table);

    /* local db */
    frame = gtk_frame_new("Local Database");
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_border_width(GTK_CONTAINER(frame), GNOME_PAD_SMALL);

    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(frame), create_local_db());

    return vbox;
}

/* EDIT STUFF */
static void edit_destroy_cb(GtkWidget *widget, EditWindow *w)
{
    /* FIXME implement saving here */
    if(w->fp)
	fclose(w->fp);

    gtk_widget_destroy(w->window);
    g_free(w);
}

static void edit_cb(GtkWidget *widget, GtkWidget *clist)
{
    EditWindow *w;

    w = create_edit_window(clist);
}

static EditWindow *create_edit_window(GtkWidget *clist)
{
    EditWindow *w;
    GtkWidget *text, *frame;
    GtkWidget *button;
    GtkWidget *vbox, *bbox;
    GtkWidget *table, *vscrollbar, *hscrollbar;

    gchar *data;
    guint len;
    gchar *filename;

    w = g_new0(EditWindow, 1);

    /* vertical box */
    vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
    gtk_container_border_width(GTK_CONTAINER(vbox), GNOME_PAD_SMALL);
    
    /* frame */
    frame = gtk_frame_new("CDDB Data");
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    /* TEXT WINDOW */
    /* table */
    table = gtk_table_new(2, 2, FALSE);
 
    /* text widget */
    text = gtk_text_new(NULL, NULL);
    gtk_text_set_editable(GTK_TEXT(text), TRUE);
    gtk_text_set_word_wrap(GTK_TEXT(text), FALSE);
    gtk_table_attach(GTK_TABLE(table), text, 0, 1, 0, 1,
		     GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		     GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
    gtk_widget_grab_focus(text);
    gtk_widget_set_usize(text, 300, 400);
    
    /* scrollbars */
    hscrollbar = gtk_hscrollbar_new(GTK_TEXT(text)->hadj);
    gtk_table_attach(GTK_TABLE(table), hscrollbar, 0, 1, 1, 2,
		     GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL, 0, 0);
    gtk_widget_show(hscrollbar);
    
    vscrollbar = gtk_vscrollbar_new(GTK_TEXT(text)->vadj);
    gtk_table_attach(GTK_TABLE(table), vscrollbar, 1, 2, 0, 1,
		      GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
    gtk_widget_show(vscrollbar);
    

    gtk_container_add(GTK_CONTAINER(frame), table);

    /* BUTTONS */
    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);

    button = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
    gtk_box_pack_start_defaults(GTK_BOX(bbox), button);
/*    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(destroy_window), (gpointer)TRUE);*/
    button = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
    gtk_box_pack_start_defaults(GTK_BOX(bbox), button);
/*    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(destroy_window), (gpointer)FALSE);*/
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    /* open file and send to text widget */
    filename = gtk_object_get_data(GTK_OBJECT(clist), "filename");
    if(!(w->fp = fopen(filename, "rw")))
    {
	/* FIXME create dialog here warning user of error */
	g_print("can't open %s\n", filename);
	gtk_widget_destroy(vbox);
	g_free(w);
	return NULL;
    }
    fseek(w->fp, 0, SEEK_END);	/* find file length */
    len = ftell(w->fp);
    fseek(w->fp, 0, SEEK_SET);

    data = g_malloc(len);
    if(!fread(data, 1, len, w->fp))
    {
	/* FIXME create dialog here */
	gtk_widget_destroy(vbox);
	fclose(w->fp);
	g_free(w);
	return NULL;
    }

    gtk_text_freeze(GTK_TEXT(text));
    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL,
		    data, len);
    gtk_text_thaw(GTK_TEXT(text));

    /* main window */
    w->window = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_container_border_width(GTK_CONTAINER(w->window), GNOME_PAD_SMALL);
    gtk_window_set_title(GTK_WINDOW(w->window), _("CDDB Editor"));
    gtk_window_set_wmclass(GTK_WINDOW(w->window), "cddb_editor","gtcd");   
    gtk_signal_connect(GTK_OBJECT(w->window), "destroy_event",
		       GTK_SIGNAL_FUNC(edit_destroy_cb), w);

    gtk_container_add(GTK_CONTAINER(w->window), vbox);
    gtk_widget_show_all(w->window);

    return w;
}

/* fill a clist with cddb entries */
static void fill_list(GtkWidget *clist)
{
    const gchar *error_item[] = {"0", N_("Error reading $HOME/.cddbslave.")};
    char *dname;
    gchar *tmp[2];
    DIR *d;
    struct dirent *de;

    dname = get_file_name("");
    if(!dname)
	goto error;

    d = opendir(dname);
    if(!d)
	goto error;

    gtk_clist_freeze(GTK_CLIST(clist));
    while((de = readdir(d)))
    {
	gchar *filename;

	if(strlen(de->d_name) != 8) /* erm, sort of ugly hack but it should work just fine :) */
	    continue;

        filename = get_file_name(de->d_name);
	tmp[0] = de->d_name;
	tmp[1] = get_dtitle(filename);

	gtk_clist_append(GTK_CLIST(clist), (const gchar**)tmp);

	g_free(filename);
	g_free(tmp[1]);
    }
    gtk_clist_thaw(GTK_CLIST(clist));
    
    closedir(d);
    g_free(dname);
    return;
 error:;
    gtk_clist_append(GTK_CLIST(clist), error_item);
    g_free(dname);
    return;
}

/* return a path formatted like: $HOME/.cddbslave/append */
static gchar *get_file_name(gchar *append)
{
    char *fname;
    char *homedir=NULL;
    struct passwd *pw=NULL;

    homedir = getenv("HOME");

    if(homedir == NULL)
    {
        pw = getpwuid(getuid());

        if(pw != NULL)
            homedir = pw->pw_dir;
        else
            homedir = "/";
    }

    fname = g_malloc(512);

    g_snprintf(fname, 511, "%s/.cddbslave/%s", homedir, append);
    return fname;
}

/* open a cddb file in specified dir, and parse out the dtitle. */
static gchar *get_dtitle(const gchar *filename)
{
    FILE *fp;
    char string[256];

    if(!(fp = fopen(filename, "r")))
    {
	perror("fopen");
	return g_strdup("Cannot open file");
    }
    while(fgets(string, 255, fp)!=NULL)
    {
	string[strlen(string)-1] = 0;
	
	if( strncmp( string, "DTITLE", 6 ) == 0)
	{
	    fclose(fp);
	    return g_strdup(string+7);
	}
    }
    fclose(fp);
    return g_strdup("Invalid CDDB File");  
}

static void refresh_cb(GtkWidget *widget, GtkWidget *clist)
{
    gtk_clist_clear(GTK_CLIST(clist));

    fill_list(clist);
}

static void msg_callback(gint reply, GtkCList *clist)
{
    gchar *filename;
    gint row;

    filename = gtk_object_get_data(GTK_OBJECT(clist), "filename");
    row = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(clist), "row"));

    if(reply == 0)
    {
	if(remove(filename))
	{
	    gchar tmp[256];
	    g_snprintf(tmp, 255, "Couldn't remove file: %s\n", strerror(errno));
	    gtk_widget_show(gnome_error_dialog(tmp));
	}
	else
	    gtk_clist_remove(clist, row);
    }
}

static void remove_cb(GtkWidget *widget, GtkWidget *clist)
{
    GtkWidget *msg;

    msg = gnome_question_dialog("Delete CDDB Entry?",
				(GnomeReplyCallback)msg_callback,
				clist);
    gtk_widget_show(msg);
}
