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
#include <sys/socket.h>
#include <unistd.h>

#ifdef linux
# include <linux/cdrom.h>
# include "linux-cdrom.h"
#else
# error TCD currently only builds under Linux systems.
#endif

#include "gcddb.h"
#include "socket.h"
#include "cddb.h"
#include "properties.h"

GtkWidget *win, *label;
GtkWidget *server_e, *port;
extern int tracklabel_f, titlelabel_f;
extern cd_struct cd;
GtkWidget *box;
GtkWidget *pb;
int val;
int cancel_id, start_id, timeout;
GtkWidget *cancelbutton, *startbutton;

extern tcd_properties props;
void create_warn(void);

void close_cddb(GtkWidget *widget, GtkWidget **window)
{
	gtk_widget_destroy(win);
}

void cancel_cddb( GtkWidget *widget, gpointer data )
{
	gtk_signal_disconnect( GTK_OBJECT(cancelbutton), cancel_id );
	cancel_id = gtk_signal_connect (GTK_OBJECT(cancelbutton), "clicked",
        	GTK_SIGNAL_FUNC(close_cddb), NULL);
	
}

void gcddb()
{
	GtkWidget *table, *tmp;
	char ctmp[8];

	if( win )
		return;
	
	box = gtk_vbox_new( FALSE, 5 );
	table = gtk_table_new( 2,4,TRUE );

	win = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_container_border_width (GTK_CONTAINER (win), 5);

	tmp = gtk_label_new( "Server:" );
	gtk_widget_show( tmp );
	gtk_table_attach_defaults( GTK_TABLE(table), tmp, 0,3,0,1 );
	tmp = gtk_label_new( "Port:" );
	gtk_widget_show( tmp );
	gtk_table_attach_defaults( GTK_TABLE(table), tmp, 3,4,0,1 );
	
	server_e = gtk_entry_new();
	port = gtk_entry_new();
	gtk_widget_show( server_e );
	gtk_widget_show( port );
	gtk_table_attach_defaults( GTK_TABLE(table), server_e, 0,3,1,2 );
	gtk_table_attach_defaults( GTK_TABLE(table), port, 3,4,1,2 );
	gtk_entry_set_text( GTK_ENTRY(server_e), props.cddb );
	sprintf( ctmp, "%d", props.cddbport );
	gtk_entry_set_text( GTK_ENTRY(port), ctmp );
	
	gtk_widget_set_usize( table, 200, 50 );
	gtk_widget_show(table);
	
	gtk_window_set_title( GTK_WINDOW(win), "CDDB Remote" );
	gtk_window_set_wmclass( GTK_WINDOW(win), "cddb","gtcd" );
	
	cancelbutton = gtk_button_new_with_label( "Close" );
	gtk_widget_show( cancelbutton );
	cancel_id = gtk_signal_connect (GTK_OBJECT (cancelbutton), "clicked",
		GTK_SIGNAL_FUNC(close_cddb), NULL);

        label = gtk_label_new("CDDB Remote Controller");
	gtk_widget_show(label);

	startbutton = gtk_button_new_with_label( "Go" );
	gtk_widget_show( startbutton);
	start_id = gtk_signal_connect (GTK_OBJECT (startbutton), "clicked",
		GTK_SIGNAL_FUNC(do_cddb), NULL);

	tmp = gtk_hseparator_new();
	gtk_widget_show( tmp );

	pb = gtk_progress_bar_new();
	gtk_widget_show( pb );

        gtk_box_pack_start( GTK_BOX(box), label, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(box), tmp, TRUE, TRUE, 0 );
        gtk_box_pack_start( GTK_BOX(box), table, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(box), startbutton, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(box), cancelbutton, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(box), pb, TRUE, TRUE, 0 );
                        
        gtk_signal_connect (GTK_OBJECT (win), "delete_event",
                        GTK_SIGNAL_FUNC(close_cddb), NULL);

	gtk_container_add( GTK_CONTAINER(win), box );
	gtk_widget_show( box );
	gtk_widget_show( win );                                                
}

int do_cddb( GtkWidget *widget, gpointer data )
{
	int result;
	cddb_server server;
	int i,r;
	char s[80], qs[200];
	char tmp[60];
	FILE *outfile;
	cddb_query_str query;

	strcpy( server.hostname, (char*)gtk_entry_get_text(GTK_ENTRY(server_e)));
	server.port = atoi( gtk_entry_get_text(GTK_ENTRY(port)) );
	gtk_label_set( GTK_LABEL(label), "Connecting..." );
	while(gtk_events_pending()) gtk_main_iteration();

	if( tcd_open_cddb( &server ) != 0 )
	{
		sprintf( tmp, "Error: %s", server.error );
		gtk_label_set( GTK_LABEL(label), tmp );
		return 0;
	}
	gtk_label_set( GTK_LABEL(label), "Connected!" );
	gtk_progress_bar_update( GTK_PROGRESS_BAR(pb), 0.2);
	while(gtk_events_pending()) gtk_main_iteration();

	tcd_formatquery( &cd, qs , sizeof(qs));
	gtk_label_set( GTK_LABEL(label), "Sending query..." );
	r = send( server.socket, qs, strlen(qs), 0 );
#ifdef DEBUG
	g_print( "-> %s\n", qs );
#endif
	gtk_progress_bar_update( GTK_PROGRESS_BAR(pb), 0.4);
	while(gtk_events_pending()) gtk_main_iteration();

	gtk_label_set( GTK_LABEL(label), "Reading results..." );
	tcd_getquery( &server, &query );
	gtk_progress_bar_update( GTK_PROGRESS_BAR(pb), 0.6);
	while(gtk_events_pending()) gtk_main_iteration();

	sprintf( qs, "cddb read %s %s\n", query.categ, query.discid );
	gtk_label_set( GTK_LABEL(label), "Downloading data..." );
	send( server.socket, qs, strlen(qs), 0 );
#ifdef DEBUG
	g_print( "-> %s\n", qs );
#endif
	gtk_progress_bar_update( GTK_PROGRESS_BAR(pb), 1.0);
	while(gtk_events_pending()) gtk_main_iteration();

	sprintf( qs, "%s/.tcd/%s", getenv("HOME"),query.discid );
	outfile = fopen( qs, "w" );

	fgetsock( s, 80, server.socket );
	sscanf( s, "%d ", &result );
#ifdef DEBUG
	g_print( "%d\n", result );
#endif
/* Urk. cddb.howto is broken. */
	if( result != 200 && result != 210 )
	{
		gtk_label_set( GTK_LABEL(label), "Exact match not found." );
		fclose(outfile);
		close(server.socket);
		return;
	}	

	i=0;
	do
	{
		fgetsock( s, 80, server.socket );
#ifdef DEBUG
		g_print( "<- %s\n", s );
#endif
		fprintf( outfile, "%s\n", s );
		sprintf( tmp, "Downloading line %3d...",i++ );
		gtk_label_set( GTK_LABEL(label), tmp );
		while(gtk_events_pending()) gtk_main_iteration();
	} while( strcmp( ".", s ) );

	fclose(outfile);
	close(server.socket);
	gtk_label_set( GTK_LABEL(label), "Done!" );

	tcd_close_disc(&cd);
        tcd_init_disc(&cd,(WarnFunc)create_warn);

	titlelabel_f = tracklabel_f = TRUE;
	gtk_progress_bar_update( GTK_PROGRESS_BAR(pb), 0);
	while(gtk_events_pending()) gtk_main_iteration();

	gtk_signal_disconnect( GTK_OBJECT(cancelbutton), cancel_id );
	cancel_id = gtk_signal_connect (GTK_OBJECT(cancelbutton), "clicked",
        	GTK_SIGNAL_FUNC(close_cddb), NULL);
}        	                                                                          
