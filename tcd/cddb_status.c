#include <gnome.h>
#include "gtcd_public.h"
#include "cddb.h"

char *status_string[] = 
{
    "No status.",
    "Connecting.",
    "Querying server.",
    "Waiting for reply.",
    "Reading reply.",
    "Disconnecting.",
    "Error connecting.",
    "Error querying.",
    "Error reading reply.",
    "No exact match found.",
    "No status.",
};

GtkWidget *csw;			/* cddb status window */
guint timer=0;

static void destroy_window (GtkWidget *widget, gpointer data);

static void destroy_window (GtkWidget *widget, gpointer data)
{
    gtk_timeout_remove(timer);
    gtk_widget_destroy(csw);
    csw = NULL;
}

static char *read_status(void)
{
    int status;
    FILE *fp;
    
    fp = fopen("/tmp/.cddbstatus", "r");
    if(!fp)
	return status_string[0x000];
    fscanf(fp, "%03d\n", &status);
    fclose(fp);

    return status_string[status];
}

static int status_timer(GtkWidget *label)
{
    gtk_label_set(GTK_LABEL(label), read_status());
    return TRUE;
}

static void call_slave(GtkWidget *widget, gpointer data)
{
    tcd_call_cddb_slave(&cd, "TCD", "3.0");
}

void cddb_status_dialog(GtkWidget *widget, gpointer data)
{
    GtkWidget *main_box;
    GtkWidget *button;
    GtkWidget *label;

    if(csw)
	return;
    
    csw = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_container_border_width(GTK_CONTAINER(csw), GNOME_PAD_SMALL);
    gtk_window_set_title(GTK_WINDOW(csw), _("CDDB Status"));
    gtk_window_set_wmclass(GTK_WINDOW(csw), "cddb_status","gtcd");
    
    gtk_signal_connect(GTK_OBJECT(csw), "delete_event",
		       GTK_SIGNAL_FUNC(destroy_window), NULL);
 
    main_box = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
    gtk_container_add(GTK_CONTAINER(csw), main_box);
		  
    /* status line */
    label = gtk_label_new("No status.");
    gtk_box_pack_start_defaults(GTK_BOX(main_box), label);

    /* grab button */
    button = gtk_button_new_with_label("Get CDDB Now");
    gtk_box_pack_start_defaults(GTK_BOX(main_box), button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(call_slave), NULL);

    /* close button */
    button = gnome_stock_button(GNOME_STOCK_BUTTON_CLOSE);
    gtk_box_pack_start_defaults(GTK_BOX(main_box), button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(destroy_window), NULL);

    timer = gtk_timeout_add(500, (GtkFunction)status_timer, label);
    gtk_widget_show_all(csw);
}
