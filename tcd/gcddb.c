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
#include <linux/cdrom.h>

#include "linux-cdrom.h"
#include "gcddb.h"
#include "socket.h"
#include "cddb.h"
#include "properties.h"

GtkWidget *win, *label, *pb;
GtkWidget *cancelbutton, *startbutton;
        
extern int tracklabel_f, titlelabel_f;
extern cd_struct cd;
extern tcd_properties props;

int val;
int cancel_id, start_id, timeout;

void create_warn( char *message_text, char *type );

void close_cddb(GtkWidget *widget, GtkWidget **window)
{
	gtk_widget_destroy(win);
	win = NULL;
}

void cancel_cddb( GtkWidget *widget, gpointer data )
{
	gtk_signal_disconnect( GTK_OBJECT(cancelbutton), cancel_id );
	cancel_id = gtk_signal_connect (GTK_OBJECT(cancelbutton), "clicked",
        	GTK_SIGNAL_FUNC(close_cddb), NULL);
}

void gcddb()
{
	GtkWidget *box, *tmplabel, *infoframe, *infobox;
	char tmp[255];
	char pt[24];

	if( win )
		return;
	
	label = gtk_label_new("");
	box = gtk_vbox_new( FALSE, 5 );
	infobox = gtk_vbox_new( FALSE, 0 );
	infoframe = gtk_frame_new("Configuration:");
	gtk_frame_set_shadow_type(GTK_FRAME(infoframe),GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(infoframe), infobox);

	win = gtk_window_new( GTK_WINDOW_DIALOG );
	gtk_container_border_width (GTK_CONTAINER (win), 5);
	gtk_window_set_title( GTK_WINDOW(win), "CDDB Remote" );
	gtk_window_set_wmclass( GTK_WINDOW(win), "cddb","gtcd" );
	
	/* cancel button */
	cancelbutton = gtk_button_new_with_label( "Close" );
	cancel_id = gtk_signal_connect (GTK_OBJECT (cancelbutton), "clicked",
		GTK_SIGNAL_FUNC(close_cddb), NULL);

	/* go button */
	startbutton = gtk_button_new_with_label( "Go" );
	start_id = gtk_signal_connect (GTK_OBJECT (startbutton), "clicked",
		GTK_SIGNAL_FUNC(do_cddb), NULL);

	snprintf( tmp, 256, "Server: %s:%d\n", props.cddb, props.cddbport );
        tmplabel = gtk_label_new(tmp);
	gtk_box_pack_start( GTK_BOX(infobox), tmplabel, FALSE, TRUE, 0 );
	if( props.use_http )
	{
		snprintf( tmp, 256, "HTTP %s Enabled\n", 
			props.use_proxy?"and Proxy":"" );
	        tmplabel = gtk_label_new(tmp);
		gtk_box_pack_start( GTK_BOX(infobox), tmplabel, FALSE, TRUE, 0 );
	}
	pb = gtk_progress_bar_new();
	
	gtk_box_pack_start( GTK_BOX(box), infoframe, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(box), label, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(box), startbutton, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(box), cancelbutton, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(box), pb, TRUE, TRUE, 0 );

        gtk_signal_connect (GTK_OBJECT (win), "delete_event",
                        GTK_SIGNAL_FUNC(close_cddb), NULL);

	gtk_container_add( GTK_CONTAINER(win), box );
	gtk_widget_show_all(win);
}

int do_cddb( GtkWidget *widget, gpointer data )
{
	int result;
	cddb_server server;
	int i,r;
	char s[128], qs[800];
	char tmp[60];
	FILE *outfile;
	cddb_query_str query;

	strcpy( server.hostname, props.cddb );
	server.port = props.cddbport;
	server.http = props.use_http;
	server.proxy = props.use_proxy;

#ifdef DEBUG
	printf("use_http==%d  use_proxy==%d\n",use_http,use_proxy);
#endif
	if(server.proxy) {
		strcpy(server.proxy_server, props.proxy_server);
		server.proxy_port = props.proxy_port;
		strcpy(server.remote_path, props.remote_path);
	}
	
	gtk_label_set( GTK_LABEL(label), "Connecting..." );
	while(gtk_events_pending()) gtk_main_iteration();

	if (server.http) {
		if(tcd_open_cddb_http(&server)) {
			sprintf(tmp,"Eror: %s", server.error);
			gtk_label_set(GTK_LABEL(label),tmp);
			return(0);
		}
	} else {
		if( tcd_open_cddb( &server ) != 0 )
		{
			sprintf( tmp, "Error: %s", server.error );
			gtk_label_set( GTK_LABEL(label), tmp );
			return 0;
		}
	}
	gtk_label_set( GTK_LABEL(label), "Connected!" );
	gtk_progress_bar_update( GTK_PROGRESS_BAR(pb), 0.2);
	while(gtk_events_pending()) gtk_main_iteration();

	if (server.http) {
		tcd_formatquery_http(&cd,qs,sizeof(qs),server.hostname,server.port,server.remote_path);
	} else {
		tcd_formatquery( &cd, qs, sizeof(qs) );
	}
	gtk_label_set( GTK_LABEL(label), "Sending query..." );
	r = send( server.socket, qs, strlen(qs), 0 );
#ifdef DEBUG
	g_print( "-> %s\n", qs );
#endif
	gtk_progress_bar_update( GTK_PROGRESS_BAR(pb), 0.4);
	while(gtk_events_pending()) gtk_main_iteration();

	gtk_label_set( GTK_LABEL(label), "Reading results..." );
	if (server.http) {
		if (tcd_getquery_http(&server,&query)) {
			gtk_label_set(GTK_LABEL(label),"Error: Unable to open cddb read socket\n");
			return(0);
		}
	} else {
		tcd_getquery( &server, &query );
	}
	gtk_progress_bar_update( GTK_PROGRESS_BAR(pb), 0.6);
	while(gtk_events_pending()) gtk_main_iteration();

	gtk_label_set( GTK_LABEL(label), "Downloading data..." );
	if (server.http) {
		tcd_formatread_http(&cd,qs,sizeof(qs),server.hostname,server.port,server.remote_path,query.categ,query.discid); 
		send( server.socket, qs, strlen(qs), 0 );
	} else {
		sprintf( qs, "cddb read %s %s\n", query.categ, query.discid );
		send( server.socket, qs, strlen(qs), 0 );
	}
#ifdef DEBUG
	g_print( "-> %s\n", qs );
#endif
	gtk_progress_bar_update( GTK_PROGRESS_BAR(pb), 1.0);
	while(gtk_events_pending()) gtk_main_iteration();

	sprintf( qs, "%s/.tcd/%s", getenv("HOME"),query.discid );
	outfile = fopen( qs, "w" );

	if (server.http) {
		do {
			fgetsock(s,127,server.socket);
		} while (strncmp(s,"Content-Type:",13));
		fgetsock(s,127,server.socket);
	}

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
        tcd_init_disc(&cd, create_warn);

	titlelabel_f = tracklabel_f = TRUE;
	gtk_progress_bar_update( GTK_PROGRESS_BAR(pb), 0);
	while(gtk_events_pending()) gtk_main_iteration();

	gtk_signal_disconnect( GTK_OBJECT(cancelbutton), cancel_id );
	cancel_id = gtk_signal_connect (GTK_OBJECT(cancelbutton), "clicked",
        	GTK_SIGNAL_FUNC(close_cddb), NULL);
}        	                                                                          
