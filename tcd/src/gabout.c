/* This file is part of TCD 2.0.
   gabout.c - About dialog for gtcd.
   
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
#include <linux/cdrom.h>

#include "gabout.h"
#include "../icons/default/aboutbox.xpm"
#include "../icons/default/credit.h"

GtkWidget *win, *td_a, *notebook;

void destroy_about_window (GtkWidget *widget, GtkWidget **data)
{
	gtk_widget_destroy( win );
	win = NULL;
	gtk_widget_set_sensitive( GTK_WIDGET(td_a), TRUE );
	td_a=NULL;
}

void add_line( gchar *text, GtkWidget *box, gint spacing )
{
	GtkWidget *label = gtk_label_new(text);
	gtk_widget_show(label);
	gtk_box_pack_start( GTK_BOX(box), label, TRUE, TRUE, spacing );
}

GtkWidget *create_about_box( void )
{
	char tmp[128];
	GtkWidget *box;
	GtkWidget *pixmapwid;
	GdkPixmap *pixmap;
	GtkStyle *style;
	GdkBitmap *mask;
	
	style = gtk_widget_get_style( win );
	pixmap = gdk_pixmap_create_from_xpm_d( win->window,  &mask,
			&style->bg[GTK_STATE_NORMAL],
			(gchar **)aboutbox_xpm );
	pixmapwid = gtk_pixmap_new( pixmap, mask );
	gtk_widget_show( pixmapwid );

	box = gtk_vbox_new( FALSE,4 );

	gtk_box_pack_start( GTK_BOX(box), pixmapwid, TRUE, TRUE, 0 );
	add_line( PACKAGE" "VERSION " - Copyright (C) 1997-98", box, 0 );
	add_line( "Written by Tim P. Gerla", box, 0 );
	sprintf( tmp,"Iconset by %s", CREDIT );
	add_line( tmp, box, 0 );

	gtk_widget_show( box );
	return box;
}

GtkWidget *create_thanks_box( void )
{
	GtkWidget *box;
	
	box = gtk_vbox_new( FALSE, 4 );
	add_line( "Thanks goes to all of TCD's users, ", box, 0 );
	add_line( "especially those who sent me an email", box, 0 );
	add_line( "about TCD.", box, 0 );
	add_line( "", box, 0 );
	add_line( "Special thanks goes to the folks in", box, 0 );
	add_line( "#LinuxOS, especially Magnwa, NumbLock,", box, 0 );
	add_line( "and Kha0S.", box, 0 );
	add_line( "", box, 0 );
	add_line( "Thanks goes to Ti Kan and Steve Scherf", box, 0 );
	add_line( "for the CDDB database format.", box, 0 );
	gtk_widget_show( box );
	return box;
}	

void about( GtkWidget *button_td )
{
	GtkWidget *button, *wbox;
	GtkWidget *label;

	td_a = button_td;
	gtk_widget_set_sensitive( button_td, FALSE );
	win = gtk_window_new( GTK_WINDOW_DIALOG );
	gtk_window_set_title( GTK_WINDOW(win), "About " PACKAGE " "VERSION );
        gtk_window_set_wmclass( GTK_WINDOW(win), "about","gtcd" );
	gtk_container_border_width (GTK_CONTAINER (win), 5);          
	gtk_widget_realize( win );

	notebook = gtk_notebook_new();

	gtk_signal_connect (GTK_OBJECT (win), "delete_event",
		GTK_SIGNAL_FUNC (destroy_about_window), &button_td);
	
	wbox = gtk_vbox_new( FALSE,4 );
	
	button = gtk_button_new_with_label( "OK" );
	gtk_widget_show( button );

	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (destroy_about_window), &button_td);

	label = gtk_label_new("About");
	gtk_widget_show(label);
	gtk_notebook_append_page( GTK_NOTEBOOK(notebook), create_about_box(), label ); 
	label = gtk_label_new("Thanks");
	gtk_widget_show(label);
	gtk_notebook_append_page( GTK_NOTEBOOK(notebook), create_thanks_box(), label ); 

	gtk_box_pack_start( GTK_BOX(wbox), notebook, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(wbox), button, TRUE, TRUE, 0 );

	gtk_container_add( GTK_CONTAINER(win), wbox );

	gtk_widget_show( wbox );
	gtk_widget_show( notebook );
	gtk_widget_show( win ); 
}
