#define PACKAGE "TCD"
#define VERSION "2.0"

/* This file is part of TCD 2.0.
   gtcd.c - Main source file for GTK+ interface.
   
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
#include <gnome.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/cdrom.h>
#include <linux/soundcard.h>

#include "cdrom.h"
#include "tcd.h"
#include "tracked.h"
#include "cddb.h"

#include "gtracked.h"
#include "gabout.h"
#include "gcddb.h"
#include "properties.h"

#include "icons/default/play.xpm"
#include "icons/default/stop.xpm"
#include "icons/default/pause.xpm"
#include "icons/default/ff.xpm"
#include "icons/default/rw.xpm"
#include "icons/default/prev_t.xpm"
#include "icons/default/next_t.xpm"
#include "icons/default/power.xpm"
#include "icons/default/eject.xpm"
#include "icons/default/menu.xpm"
#include "icons/default/cdrom.xpm"
#include "icons/default/cddb.xpm"
#include "icons/default/edit.xpm"
#include "icons/default/goto.xpm"

#include "tooltips.h"


enum { PLAY=0, PAUSE, STOP, EJECT,
       FF, RW, NEXT_T, PREV_T,
       CDDB, TRACKLIST, GOTO, QUIT, ABOUT };

char *play_methods[] =
{
	"Repeat CD",
        "Repeat Trk",
        "Normal",
        "Random" // Not yet implemented
};
                                 

#define Connect( x,y ) gtk_signal_connect (GTK_OBJECT (x), "clicked", \
	        GTK_SIGNAL_FUNC (callback), (gpointer*)y);
      
/* Regular globals */
cd_struct cd;
int tracklabel_f = 0, titlelabel_f = 0;
int gotoi;

/* Gtk Globals */
GtkWidget *row, *vbox, *upper_box, *bottom_box;
GtkWidget *button_box, *row1, *row2, *row3;
GtkWidget *tracklabel, *titlelabel, *trackeditor;
GtkWidget *tracktime_label, *trackcur_label;
GtkWidget *cdtime_label, *changer_box;
GtkWidget *status_table, *status_area, *sep;
GtkWidget *volume, *window, *aboutbutton;
GtkWidget *gotomenu = NULL, *gotobutton, *lowerbox;
GdkPixmap *status_db;
GtkWidget **changer_buttons, *cddbbutton;
GtkObject *vol;
GdkColormap *colormap;
GdkFont *sfont, *tfont;
GdkColor darkgrey, green, blue;

GtkTooltips *tooltips;

int timeonly = FALSE, status_height, status_width;
int configured = FALSE;
tcd_properties props;

/* Prototypes */
void draw_status( void );
void delete_event (GtkWidget *widget, gpointer *data);

void callback (GtkWidget *widget, gpointer *data)
{
	if( cd.isplayable ) tcd_gettime(&cd);
	switch( (int)data )
	{
		case PLAY:
  			if( cd.sc.cdsc_audiostatus == CDROM_AUDIO_PAUSED )
	                	tcd_pausecd(&cd);
        	        else 
				tcd_playtracks( &cd,cd.first_t,cd.last_t );
			cd.repeat_track = cd.cur_t;
                	break;
		case PAUSE:
			tcd_pausecd(&cd);
			break;
		case STOP:
			tcd_stopcd(&cd);
			break;	                                                                                                        
		case EJECT:
			tcd_ejectcd(&cd);
		        cd.play_method = NORMAL;
		        cd.repeat_track = -1;
		        break;
		case FF:
			tcd_play_seconds(&cd, 4);
			cd.repeat_track = cd.cur_t;
			break;
		case RW:
			tcd_play_seconds(&cd, -4);
			cd.repeat_track = cd.cur_t;
			break;
		case NEXT_T:
			if( cd.cur_t < cd.last_t ) 
			{
				cd.cur_t++;
				tcd_playtracks( &cd,cd.cur_t, cd.last_t );
				if( cd.play_method==REPEAT_TRK )
					cd.repeat_track = cd.cur_t;
			}
			break;
		case PREV_T:
                	if( cd.cur_t > cd.first_t ) 
                	{
				if( (cd.t_sec+(cd.t_min*60)) < 10 )
					cd.cur_t--;
	                       	tcd_playtracks( &cd,cd.cur_t, cd.last_t );
                        	
                        	if( cd.play_method==REPEAT_TRK )
                        		cd.repeat_track = cd.cur_t;
                        }
                        else
                        	tcd_playtracks( &cd, cd.cur_t, cd.last_t );
                        break;
		case TRACKLIST:
			gtracked(trackeditor);
			break;
		case QUIT:
			delete_event(NULL,NULL);
			break;		
		case ABOUT:
			about_cb(NULL,NULL);
			break;	
		case CDDB:
			gcddb( cddbbutton );
			break;
		default:
	}
	draw_status();
}
                                               
void delete_event (GtkWidget *widget, gpointer *data)
{
	gtk_main_quit();
}

GtkWidget* make_button( char *title, GtkWidget *box, int func, gchar *tooltip )
{
	GtkWidget *button;
	
	button = gtk_button_new_with_label (title);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);
	gtk_widget_show (button);
	Connect( button, func );

        if( props.tooltips )
        	gtk_tooltips_set_tip( tooltips, button, tooltip, "" );

	return button;
}	                        

GtkWidget* make_button_with_pixmap( char **pic, GtkWidget *box, int func,
	gint expand, gint fill, gchar *tooltip )
{
	GtkWidget *button;
	GtkWidget *pixmapwid;
	GdkPixmap *pixmap;
	GtkStyle *style;
	GdkBitmap *mask;
	
	style = gtk_widget_get_style( window );
        pixmap = gdk_pixmap_create_from_xpm_d( window->window,  &mask,
        	&style->bg[GTK_STATE_NORMAL],
                (gchar **)pic );
	pixmapwid = gtk_pixmap_new( pixmap, mask );
        gtk_widget_show( pixmapwid );
        
	button = gtk_button_new();
	gtk_container_add( GTK_CONTAINER(button), pixmapwid );
	gtk_box_pack_start( GTK_BOX (box), button, expand, fill, 0 );
	gtk_widget_show(button);
	gtk_signal_connect(GTK_OBJECT (button), "clicked", \
	        GTK_SIGNAL_FUNC (callback), (gpointer*)func );
	
	if( props.tooltips )
		gtk_tooltips_set_tip( tooltips, button, tooltip, "" );

	return button;
}	                        

void update_changer_buttons( void )
{
	int i;
	for( i=0; i < cd.nslots; i++ )
	{
		if( i == cd.cur_disc )
			gtk_widget_set_sensitive(GTK_WIDGET(changer_buttons[i]), FALSE );
		else 
			gtk_widget_set_sensitive(GTK_WIDGET(changer_buttons[i]), TRUE );
	}
}	

gint changer_callback( GtkWidget *widget, gpointer *data )
{
	tcd_close_disc(&cd);
	tcd_change_disc( &cd, (int)data );
	tcd_init_disc(&cd);
	
	cd.play_method = NORMAL;
	titlelabel_f = TRUE;
	tracklabel_f = -100;
	return 1;
}

GtkWidget* make_changer_buttons( void )
{
	GtkWidget *box;
	char tmp[5];
	int i;

	box = gtk_hbox_new( FALSE, 0 );
	changer_buttons = malloc( sizeof(GtkWidget)*cd.nslots );
		
	for( i=0; i < cd.nslots && i < 12; i++ )
	{
		sprintf( tmp, "%d", i+1 );
		changer_buttons[i] = gtk_button_new_with_label( tmp );
		gtk_widget_show( changer_buttons[i] );
		gtk_box_pack_start( GTK_BOX(box), changer_buttons[i], FALSE, FALSE, 0 );
	 	gtk_signal_connect( GTK_OBJECT(changer_buttons[i]), "clicked", \
		        GTK_SIGNAL_FUNC(changer_callback), (gpointer*)i );
	}
	return box;
}

static gint button_press (GtkWidget *widget, GdkEvent *event)
{
	if (event->type == GDK_BUTTON_PRESS) 
	{
        	GdkEventButton *bevent = (GdkEventButton *) event; 
        	gtk_menu_popup (GTK_MENU(widget), NULL, NULL, NULL, NULL,
	        	bevent->button, bevent->time);
	        /* Tell calling code that we have handled this event; the buck
	         * stops here. */
	        return TRUE;
        }
        /* Tell calling code that we have not handled this event; pass it on. */
        return FALSE;
}

GtkWidget* make_row1( void )
{
	GtkWidget *box, *handle;

	box = gtk_hbox_new( TRUE, 5 );
	handle = gtk_handle_box_new();

	make_button_with_pixmap( play_xpm, box, PLAY, TRUE, TRUE, TT_PLAY );
	make_button_with_pixmap( pause_xpm, box, PAUSE, TRUE, TRUE, TT_PAUSE );
	make_button_with_pixmap( stop_xpm, box, STOP, TRUE, TRUE, TT_STOP );
	make_button_with_pixmap( eject_xpm, box, EJECT, TRUE, TRUE, TT_EJECT );
	gtk_widget_show(box);
	gtk_container_add( GTK_CONTAINER(handle), box );

	return row1=handle;
}

GtkWidget* make_row2( void )
{
	GtkWidget *box, *handle;
	box = gtk_hbox_new (TRUE, 5);
	handle = gtk_handle_box_new();

	make_button_with_pixmap(prev_t_xpm, box, PREV_T, TRUE, TRUE, TT_PREV_TRACK );
	make_button_with_pixmap(rw_xpm, box, RW, TRUE, TRUE, TT_REWIND );
	make_button_with_pixmap(ff_xpm, box, FF, TRUE, TRUE, TT_FF );
	make_button_with_pixmap(next_t_xpm, box, NEXT_T, TRUE, TRUE, TT_NEXT_TRACK );
	gtk_widget_show(box);
	gtk_container_add(GTK_CONTAINER(handle), box );

	return row2=handle;
}
GtkWidget* make_row3( void )
{
	GtkWidget *box, *bbox;
	GtkWidget *button, *gotolabel;
	GtkWidget *pixmapwid, *handle;
	GdkPixmap *pixmap;
	GtkStyle *style;
	GdkBitmap *mask;
	
        box = gtk_hbox_new( TRUE, 5 );
	bbox = gtk_vbox_new( FALSE, 0 );
	handle = gtk_handle_box_new();

	cddbbutton = make_button_with_pixmap( cddb_xpm, box, CDDB, TRUE, TRUE,  TT_CDDB );
	trackeditor =make_button_with_pixmap( edit_xpm, box, TRACKLIST, TRUE, TRUE, TT_TRACKED );

	gotobutton = gtk_button_new();

	style = gtk_widget_get_style( window );
        pixmap = gdk_pixmap_create_from_xpm_d( window->window,  &mask,
        	&style->bg[GTK_STATE_NORMAL],
                (gchar **)goto_xpm );
	pixmapwid = gtk_pixmap_new( pixmap, mask );
        gtk_widget_show( pixmapwid );
	
	gtk_box_pack_start( GTK_BOX(bbox), pixmapwid, FALSE, FALSE, 0 );
        gtk_container_add( GTK_CONTAINER(gotobutton), bbox);
	gtk_box_pack_start( GTK_BOX(box), gotobutton, TRUE, TRUE, 0);
	
	gtk_widget_show(bbox);
	gtk_widget_show(gotobutton);
	button = make_button_with_pixmap(power_xpm, box, QUIT, TRUE, TRUE, "Quit" );
	gtk_widget_show(box);
	gtk_container_add(GTK_CONTAINER(handle), box);
	
	return row3=handle;
}

void setup_colors( void )
{
	GdkColormap *colormap;
	
	colormap = gtk_widget_get_colormap(status_area);
	
	gdk_color_parse("#353535", &darkgrey);
	gdk_color_alloc(colormap, &darkgrey);

	gdk_color_parse("green", &green);
	gdk_color_alloc (colormap, &green);

	gdk_color_parse("blue", &blue);
	gdk_color_alloc (colormap, &blue);
}

void draw_status( void )
{
	char tmp[30];
	GdkGC *gc;

	if( !configured )
		return;

	gc = gdk_gc_new( window->window );
	gdk_gc_copy( gc, status_area->style->white_gc );

/* Erase Rectangle */
	gdk_draw_rectangle( status_db, 
			    status_area->style->black_gc,
			    TRUE, 0,0,
			    status_area->allocation.width,
			    status_area->allocation.height );
/* Done */

	gdk_gc_set_foreground( gc, &darkgrey );
	
	gdk_draw_line( status_db,gc,0,28,status_area->allocation.width,28 );
	gdk_draw_line( status_db,gc,48,0,48,28 );
	gdk_draw_rectangle( status_db, 
			    gc,FALSE, 0,0,
			    status_area->allocation.width-1,
			    status_area->allocation.height-1 );


	gdk_gc_set_foreground(gc, &green);

	sprintf( tmp, "%2d/%2d", cd.cur_t, cd.last_t );
	gdk_draw_text( status_db,tfont,gc,8,20,tmp,strlen(tmp) );

	sprintf( tmp, "Trk %2d:%02d/%d:%02d", cd.t_min, cd.t_sec,
						cd.trk[C(cd.cur_t)].tot_min,
						cd.trk[C(cd.cur_t)].tot_sec );
	gdk_draw_text( status_db,sfont,gc,52,12,tmp,strlen(tmp) );
	sprintf( tmp, " CD %2d:%02d/%d:%02d", cd.cd_min, cd.cd_sec,
		cd.trk[C(cd.last_t+1)].toc.cdte_addr.msf.minute,
		cd.trk[C(cd.last_t+1)].toc.cdte_addr.msf.second  );
	gdk_draw_text( status_db,sfont,gc,52,24,tmp,strlen(tmp) );

	sprintf( tmp, " Vol: %d%%", (int)ceil(cd.volume*0.390625) );
	gdk_draw_text( status_db,sfont, gc,88,54,tmp,strlen(tmp) );
	
	if( !cd.err )
	{
		switch( cd.sc.cdsc_audiostatus )
		{
			case CDROM_AUDIO_INVALID:
				strcpy( tmp,"No Audio" );
				break;
			case CDROM_AUDIO_PLAY:
				strcpy( tmp,"Playing" );
				break;
			case CDROM_AUDIO_PAUSED:
				strcpy( tmp,"Paused" );
				break;
			case CDROM_AUDIO_COMPLETED:
				strcpy( tmp,"Stopped" );
				break;
			case CDROM_AUDIO_ERROR:
				strcpy( tmp,"Error" );
				break;
			case CDROM_AUDIO_NO_STATUS:
				strcpy( tmp,"Stopped" );
				break;
			default:
				strcpy( tmp,"" );
		}
	}
	else strcpy( tmp, cd.errmsg );

	gdk_gc_set_foreground(gc, &blue);
	gdk_draw_text( status_db,sfont,gc,4,40,tmp,strlen(tmp) );
	
	gdk_draw_text( status_db,sfont,gc,4,54, play_methods[cd.play_method] ,
		strlen( play_methods[cd.play_method] ) );
	
	/* Finally, update the display */
	gdk_draw_pixmap(status_area->window,
        	status_area->style->fg_gc[GTK_WIDGET_STATE(status_area)],
                status_db,
                0, 0,
                0, 0,
                status_area->allocation.width,
                status_area->allocation.height);
	gdk_gc_destroy(gc);
}

gint slow_timer( gpointer *data )
{
	if( cd.err )
        {
		tcd_readtoc(&cd);
		tcd_readdiskinfo(&cd);		
		if( cd.err )
		{
			cd.cur_t = 0;
		        gtk_window_set_title (GTK_WINDOW (window),"(scanning)" );
			gtk_label_set( GTK_LABEL(titlelabel), "(scanning)" );
			gtk_label_set( GTK_LABEL(tracklabel), "(scanning)" );
	        	tracklabel_f = titlelabel_f = FALSE;
		}
		else
		{
	        	tracklabel_f = titlelabel_f = TRUE;
        	}
        }
	if( cd.sc.cdsc_audiostatus != CDROM_AUDIO_PLAY &&
		cd.sc.cdsc_audiostatus != CDROM_AUDIO_PAUSED )
	{
		if( cd.play_method == REPEAT_CD )
		tcd_playtracks( &cd, cd.first_t, cd.last_t );
	}				                                                        
 
	tcd_gettime(&cd);
	draw_status();
	return 1;
}

gint volume_changed( GtkWidget *widget, gpointer *data )
{
	if( !data )
	{
		cd.volume = (int)floor(GTK_ADJUSTMENT(vol)->value);
		if( cd.isplayable ) tcd_gettime(&cd);
	}
	draw_status();
	return 1;
}

gint gototrack( GtkWidget *widget, gpointer *data )
{
	tcd_playtracks( &cd, (int)data, cd.last_t );
	cd.repeat_track = (int)data;
	return 1;
}

void make_gotomenu()
{
	char buf[128];
	int i;
	GtkWidget *item;
	gotomenu = NULL;
	gotomenu = gtk_menu_new();

	for( i=1; i <= cd.last_t; i++ )
	{
		sprintf( buf, "%2d - %s", i, cd.trk[C(i)].name );

		item = gtk_menu_item_new_with_label(buf);
		gtk_menu_append(GTK_MENU (gotomenu), item);
		gtk_signal_connect_object(GTK_OBJECT(item), "activate",
			GTK_SIGNAL_FUNC(gototrack), (gpointer)i );
		gtk_widget_show(item);
	}
	if( gotoi ) gtk_signal_disconnect( GTK_OBJECT(gotobutton), gotoi );
	gotoi = gtk_signal_connect_object(GTK_OBJECT(gotobutton), "event",
		GTK_SIGNAL_FUNC (button_press), GTK_OBJECT(gotomenu));

}
#if 0
GtkWidget* add_rootmenu( GtkWidget *menu, GtkSignalFunc func, int data, char *label )
{
	GtkWidget *item;
	item = gtk_menu_item_new_with_label(label);
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_signal_connect_object(GTK_OBJECT(item), "activate",
		func, (gpointer)data );
	gtk_widget_show(item);
	return(item);
}

void make_rootmenu()
{
	GtkWidget *item;
	rootmenu = NULL;
	rootmenu = gtk_menu_new();

	add_rootmenu( rootmenu, GTK_SIGNAL_FUNC(callback), QUIT, "Quit" );
}
#endif

gint fast_timer( gpointer *data )
{
	char buf[128];
	if( tracklabel_f != (-cd.cur_t) )
	{
		tracklabel_f = -cd.cur_t;
		gtk_label_set( GTK_LABEL(tracklabel), cd.trk[C(cd.cur_t)].name );
		sprintf( buf, "%s", cd.trk[C(cd.cur_t)].name );
	        gtk_window_set_title (GTK_WINDOW (window),buf );
		draw_status();
	}
	if( titlelabel_f )
	{
		titlelabel_f = FALSE;
		gtk_label_set( GTK_LABEL(titlelabel), cd.dtitle );
		make_gotomenu();
		draw_status();
	}
	tcd_gettime(&cd);
	if( (cd.play_method==REPEAT_TRK) && (cd.cur_t != cd.repeat_track) )
		tcd_playtracks( &cd, cd.repeat_track, cd.last_t );
	return 1;
}

static gint status_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
	static int first=TRUE;
	configured = TRUE;	
	if( status_db )
		status_db = NULL;
		
	status_db = gdk_pixmap_new( widget->window, 
				    widget->allocation.width,
				    widget->allocation.height,
				    -1 );
	gdk_draw_rectangle( status_db, 
			    widget->style->black_gc,
			    TRUE, 0,0,
			    widget->allocation.width,
			    widget->allocation.height );
	gdk_draw_pixmap(status_area->window,
        	status_area->style->fg_gc[GTK_WIDGET_STATE(status_area)],
                status_db,
                0, 0,
                0, 0,
                status_area->allocation.width,
                status_area->allocation.height);

	if( first )
	{
		status_width = status_area->allocation.width;
		status_height = status_area->allocation.height;
		first=FALSE;
	}
        GTK_ADJUSTMENT(vol)->value = (double)cd.volume;
	gtk_signal_emit_by_name(GTK_OBJECT(vol),"value_changed", "no_update");
        return TRUE;
}
static gint status_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	gdk_draw_pixmap(widget->window,
        	widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                status_db,
                event->area.x, event->area.y,
                event->area.x, event->area.y,
                event->area.width, event->area.height);
	return FALSE;
}

void no_buttons( GtkWidget *widget, gpointer *data )
{
/*	GtkWidget *win;
	if( timeonly==FALSE )
	{
	        gtk_container_border_width (GTK_CONTAINER (window), 0);
		gtk_widget_set_usize( status_area, status_width, status_height );
		gtk_widget_hide( button_box );
		gtk_widget_hide( lowerbox );
		gtk_widget_hide( bottom_box );
		gtk_widget_hide( sep );
	}
	else
	{
	        gtk_container_border_width (GTK_CONTAINER (window), 5);
		gtk_widget_show( button_box );
		gtk_widget_show( lowerbox );
		gtk_widget_show( bottom_box );
		gtk_widget_show( sep );
	}
	timeonly = ~timeonly;*/
	properties_cb(NULL, NULL);
}

static gint status_click_event (GtkWidget *widget, GdkEventButton *event)
{
	if( event->type = GDK_BUTTON_PRESS )
	{
	
		if( event->x < (status_width/4) &&
			event->y < (status_height/2) )
		{
			switch( event->button )
			{
			case 1:
				if( cd.cur_t < cd.last_t )
				{
					cd.cur_t++;
					tcd_playtracks( &cd, cd.cur_t, cd.last_t );
				}
				break;
			case 2:
			case 3:
				if( cd.cur_t > cd.first_t )
				{
					cd.cur_t--;
					tcd_playtracks( &cd, cd.cur_t, cd.last_t );
				}
				break;
			} 
		}				
		else
		{
			switch( event->button )
			{
			case 1:
				cd.play_method++;
				if( cd.play_method > NORMAL )
					cd.play_method = 0;
				if( cd.play_method == REPEAT_TRK )
					cd.repeat_track = cd.cur_t;
#ifdef DEBUG
				g_print( "cd.play_method = %d\n", cd.play_method );
#endif
				draw_status();
				break;
			case 2:
			case 3:
				no_buttons(NULL, NULL);
				break;
			}
		}
	}
	
	return FALSE;
}

void setup_time_display( void )
{
	GtkWidget *handle1;
	
	lowerbox = gtk_hbox_new( FALSE, 5 );
	gtk_widget_show( lowerbox );
	
	vol = gtk_adjustment_new (0.0, 0.0, 256.0, 0.1, 1.0, 1.0);
	volume = gtk_hscale_new(GTK_ADJUSTMENT(vol));
	gtk_range_set_update_policy( GTK_RANGE(volume), GTK_UPDATE_CONTINUOUS );
        gtk_scale_set_draw_value( GTK_SCALE(volume), FALSE );
	gtk_signal_connect( GTK_OBJECT(vol), "value_changed",
        	(GtkSignalFunc)volume_changed, NULL);
        gtk_widget_show(volume);

#ifdef TCD_CHANGER_ENABLED
	changer_box = make_changer_buttons();
	gtk_widget_show( changer_box );
#endif

	status_area = gtk_drawing_area_new();
	gtk_signal_connect( GTK_OBJECT (status_area), "expose_event",
        	(GtkSignalFunc)status_expose_event, NULL);
        gtk_signal_connect( GTK_OBJECT(status_area),"configure_event",
        	(GtkSignalFunc)status_configure_event, NULL);
	gtk_signal_connect( GTK_OBJECT(status_area),"button_press_event",
        	(GtkSignalFunc)status_click_event, NULL);
	gtk_widget_set_usize( status_area, 175, 59 );

        if( props.tooltips )
        	gtk_tooltips_set_tip( tooltips, status_area, TT_TIME, "" );
        
        gtk_widget_set_events (status_area, GDK_EXPOSURE_MASK
 					   | GDK_LEAVE_NOTIFY_MASK
                                           | GDK_BUTTON_PRESS_MASK
                                           | GDK_POINTER_MOTION_MASK
                                           | GDK_POINTER_MOTION_HINT_MASK);	

	status_table = gtk_vbox_new( FALSE, 4 );
	gtk_box_pack_start( GTK_BOX(upper_box), status_table, TRUE, TRUE, 2 );

#ifdef TCD_CHANGER_ENABLED
	gtk_box_pack_start( GTK_BOX(lowerbox), changer_box, FALSE, FALSE, 0 );
#endif
	gtk_box_pack_end( GTK_BOX(lowerbox), volume, TRUE, TRUE, 5 );

	handle1 = gtk_handle_box_new();
	gtk_container_add(GTK_CONTAINER(handle1), status_area );
	gtk_widget_show(handle1);
	gtk_box_pack_start( GTK_BOX(status_table), handle1, TRUE, TRUE, 4 );
	gtk_box_pack_start( GTK_BOX(status_table), lowerbox, FALSE, FALSE, 4 );
	gtk_widget_show(status_table);
	gtk_widget_show(status_area);
	return;
}

void setup_rows( void )
{
	GtkWidget *ttbox = gtk_vbox_new( FALSE, 5 );
	sep = gtk_hseparator_new();

	bottom_box = gtk_hbox_new( FALSE, 5 );
	vbox = gtk_vbox_new( FALSE, 5 );
	upper_box = gtk_hbox_new( TRUE, 5 );
	button_box = gtk_vbox_new( TRUE, 5 );

        row = make_row1();
	gtk_box_pack_start( GTK_BOX(button_box), row, TRUE, TRUE, 0 );
	gtk_widget_show(row);
	
        row = make_row2();
	gtk_box_pack_start( GTK_BOX(button_box), row, TRUE, TRUE, 0 );
	gtk_widget_show(row);

        row = make_row3();
	gtk_box_pack_start( GTK_BOX(button_box), row, TRUE, TRUE, 0 );
	gtk_widget_show(row);

	gtk_box_pack_start( GTK_BOX(upper_box), button_box, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(vbox), upper_box, TRUE, TRUE, 0 );

        gtk_box_pack_start(GTK_BOX(vbox), sep, TRUE, FALSE, 0);

	tracklabel = gtk_label_new("--");
	gtk_widget_show(tracklabel);
        gtk_box_pack_start(GTK_BOX(ttbox), tracklabel, TRUE, FALSE, 0);
	
	titlelabel = gtk_label_new("-");
	gtk_widget_show(titlelabel);
	titlelabel_f = tracklabel_f = TRUE;

        gtk_box_pack_start(GTK_BOX(ttbox), titlelabel, TRUE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(bottom_box), ttbox, TRUE, FALSE, 0);
	aboutbutton = make_button_with_pixmap( cdrom_xpm, bottom_box, ABOUT, FALSE, FALSE, TT_ABOUT );
        gtk_box_pack_start(GTK_BOX(vbox), bottom_box, TRUE, FALSE, 0);

        gtk_container_add (GTK_CONTAINER (window), vbox);

	gtk_widget_show(sep);
	gtk_widget_show(ttbox);
	gtk_widget_show(bottom_box);
	gtk_widget_show(vbox);
	gtk_widget_show(upper_box);
	gtk_widget_show(button_box);

	return;
}

void init_window(void)
{
        sfont = gdk_font_load( "-misc-fixed-*-*-*-*-12-*-*-*-*-*-*-*" );
        tfont = gdk_font_load( "-adobe-helvetica-*-r-*-*-14-*-*-*-*-*-*-*" );

        window = gnome_app_new( "gtcd", "TCD 2.0" );
        gtk_window_set_title( GTK_WINDOW(window), PACKAGE" "VERSION" " );
        gtk_window_set_wmclass( GTK_WINDOW(window), "main_window","gtcd" );
        gtk_window_set_policy( GTK_WINDOW(window),FALSE,FALSE,TRUE);

        gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                GTK_SIGNAL_FUNC (delete_event), NULL);

        gtk_container_border_width (GTK_CONTAINER (window), 5);
        gtk_widget_realize(window);

        if( props.tooltips )
	        tooltips = gtk_tooltips_new();
}

int main (int argc, char *argv[])
{
	char *homedir;
	char rcfile[64];

        argp_program_version = VERSION;
 	gnome_init( "gtcd", NULL, argc, argv, 0, NULL );
        
        homedir = getenv("HOME");
        sprintf( rcfile, "%s/.tcd/gtcdrc", homedir );
	gtk_rc_parse( rcfile );

	cd.play_method = NORMAL;        

	load_properties(&props);
	cd.cdpath = props.cddev;
	
        tcd_init_disc(&cd);
        
	init_window();
	setup_rows();
	make_gotomenu();
	setup_time_display();
	setup_colors();

	/* Initialize some timers */
	if( cd.isplayable ) tcd_gettime(&cd);

	gtk_timeout_add(1000, (GtkFunction)slow_timer, NULL);
	gtk_timeout_add(500, (GtkFunction)fast_timer, NULL);
	titlelabel_f = TRUE;
        gnome_app_set_contents( GNOME_APP(window), vbox);
        gtk_widget_show(window); /* Make sure window is shown last */
        gtk_main ();
	gnome_config_sync();
        return 0;
}
