/* This file is part of TCD 2.0.
   gtracked.c - Track editor window for GTK+ interface.
   
   Copyright (C) 1997-98 Tim P. Gerla <timg@means.net>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
               
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
                           
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
                                    
   Tim P. Gerla
   RR 1, Box 40
   Climax, MN  56523
   timg@means.net
*/
                                                
#include <gtk/gtk.h>
#include <sys/types.h>

#ifdef linux
# include <linux/cdrom.h>
# include "linux-cdrom.h"
#else
# error TCD currently only builds under Linux systems.
#endif

#include "gtracked.h"

GtkWidget *trwin;
GtkWidget *box, *namebox;
GtkWidget *entry[50], *title;
extern cd_struct cd;
extern int tracklabel_f, titlelabel_f;

void destroy_window (GtkWidget *widget, GtkWidget **window)
{
	int i;
	for( i=1; i <= cd.last_t; i++ )
	{
		strcpy( cd.trk[i].name, (char*)gtk_entry_get_text(GTK_ENTRY(entry[i])) );
	}
	strcpy( cd.dtitle, (char*)gtk_entry_get_text(GTK_ENTRY(title)));
	tcd_writediskinfo(&cd);
	gtk_widget_destroy( trwin );
	trwin = NULL;
	tracklabel_f = titlelabel_f = -100; /* Invalidate the labels */
}


void gtracked()
{
	int i;
	char buf[64];
	/* FIXME don't hardcode number of entries! */
	GtkWidget *label, *tmp, *button, *table;

	if( trwin )
		return;

	trwin = gtk_window_new( GTK_WINDOW_DIALOG );
	gtk_window_set_title( GTK_WINDOW(trwin), "TCD 2.0 - Track Editor" );
        gtk_window_set_wmclass( GTK_WINDOW(trwin), "track_editor","gtcd" );
         		
	gtk_signal_connect (GTK_OBJECT (trwin), "delete_event",
		GTK_SIGNAL_FUNC (destroy_window), &trwin);

	/* sets the border width of the window. */
	gtk_container_border_width (GTK_CONTAINER (trwin), 5);          

	box = gtk_vbox_new( TRUE,4 );		
	table = gtk_table_new( 1, 2, FALSE );

	label = gtk_label_new("Disc Title:   ");
	gtk_widget_show(label);
		
	title = gtk_entry_new();
	gtk_entry_set_text( GTK_ENTRY(title), cd.dtitle );
	gtk_widget_show(title);
	gtk_table_attach_defaults( GTK_TABLE(table), label, 0,1,0,1 );
	gtk_table_attach_defaults( GTK_TABLE(table), title, 1,2,0,1 );
	gtk_widget_show( table );

	gtk_box_pack_start( GTK_BOX(box), table, TRUE, TRUE, 0 );
	
	tmp = gtk_hseparator_new();
	gtk_widget_show( tmp );
	gtk_box_pack_start( GTK_BOX(box), tmp, TRUE, TRUE, 0 );
	
	for( i=1; i <= cd.last_t; i++ )
	{
		table = gtk_table_new( 1, 2, FALSE );
		sprintf( buf, "Track %2d:   ", i );
		
		label = gtk_label_new( buf );
		gtk_widget_show(label);
		
		entry[i] = gtk_entry_new();
		gtk_entry_set_text( GTK_ENTRY(entry[i]), cd.trk[i].name );
		gtk_widget_show(entry[i]);
		gtk_table_attach_defaults( GTK_TABLE(table), label, 0,1,0,1 );
		gtk_table_attach_defaults( GTK_TABLE(table), entry[i], 1,2,0,1 );

		gtk_widget_show( table );
		
		gtk_box_pack_start( GTK_BOX(box), table, TRUE, TRUE, 0 );
	}
	tmp = gtk_hseparator_new();
	gtk_widget_show( tmp );
	gtk_box_pack_start( GTK_BOX(box), tmp, TRUE, TRUE, 0 );

	button = gtk_button_new_with_label( "Done" );
	gtk_widget_show( button );
	gtk_box_pack_start( GTK_BOX(box), button, TRUE, TRUE, 0 );

	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (destroy_window), &trwin);

	gtk_container_add( GTK_CONTAINER(trwin), box );
	gtk_widget_show( box );
	gtk_widget_show( trwin ); 
}
