#include <config.h>
#include <gnome.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

#include "linux-cdrom.h"
#include "gtcd_public.h"
#include "callbacks.h"

static GtkWidget *mw;

static void destroy_window(GtkWidget *widget, gpointer p);

static void destroy_window(GtkWidget *widget, gpointer p)
{
    gtk_widget_destroy(mw);
    mw = NULL;
    return;
}

void track_buttons(GtkWidget *w, gpointer p)
{
	int row, col, i;
	GtkWidget *frame, *button, *table;

	if(mw) 
	{
		destroy_window(mw, NULL);
		return;
	}

	mw = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_container_border_width(GTK_CONTAINER(mw), GNOME_PAD_SMALL);
	gtk_window_set_title(GTK_WINDOW(mw), _("Quick Track Acess"));
	gtk_window_set_wmclass(GTK_WINDOW(mw), "track_access","gtcd");
	gtk_signal_connect(GTK_OBJECT(mw), "delete_event",
			   GTK_SIGNAL_FUNC(destroy_window), NULL);

	table = gtk_table_new(ceil(cd.last_t/10), 10, TRUE);
	frame = gtk_frame_new(cd.dtitle);

	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_add(GTK_CONTAINER(mw), frame);

	row = 0;
	col = 0;
	for(i = 1; i < cd.last_t; i++)
	{
		GtkWidget *b;

		b = gtk_button_new_with_label(
			g_strdup_printf("%d", i));
		gtk_table_attach_defaults(GTK_TABLE(table), b,
					  col, col+1, row, row+1);
		gtk_tooltips_set_tip(tooltips, b, cd.trk[i].name, "");
		gtk_signal_connect(GTK_OBJECT(b), "clicked",
				  GTK_SIGNAL_FUNC(goto_track_cb),
				  GINT_TO_POINTER(i));
		if(col == 9)
		{
			col = 0;
			row++;
		}
		else col++;
	}
	gtk_widget_show_all(mw);
	return;
}

	
	
