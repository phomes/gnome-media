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

#ifdef linux
# include <linux/cdrom.h>
# include "linux-cdrom.h"
#else
# error TCD currently only builds under Linux systems.
#endif

#include "cddb.h"
#include "gtracked.h"
#include "gabout.h"
#include "gcddb.h"
#include "properties.h"
#include "tooltips.h"
#include "led.h"

/* callback commands */
enum { PLAY=0, PAUSE, STOP, EJECT,
       FF, RW, NEXT_T, PREV_T,
       CDDB, TRACKLIST, GOTO, QUIT, ABOUT, PROPS };

/* time display types */
enum { TRACK_ASC, TRACK_DEC, DISC_ASC };

char *play_methods[] =
{
	"repeat_cd.xpm",
        "repeat_track.xpm",
        "repeat_normal.xpm",
        "repeat_random.xpm" // Not yet implemented
};
GnomePixmap *play_method_pixmap[4];

#define Connect( x,y ) gtk_signal_connect (GTK_OBJECT (x), "clicked", \
	        GTK_SIGNAL_FUNC (callback), (gpointer*)y);
      
/* Regular globals */
cd_struct cd;
int titlelabel_f = 0;
int gotoi, time_display_type;

/* Gtk Globals */
GtkWidget *row, *vbox, *upper_box;
GtkWidget *button_box, *row1, *row2, *row3;
GtkWidget *trackeditor;
GtkWidget *tracktime_label, *trackcur_label;
GtkWidget *cdtime_label, *changer_box, *playbutton;
GtkWidget *status_table, *status_area, *sep;
GtkWidget *volume, *window, *aboutbutton, *propsbutton;
GtkWidget *gotomenu = NULL, *gotobutton, *lowerbox;
GdkPixmap *status_db;
GtkWidget **changer_buttons, *cddbbutton;
GtkObject *vol;
GdkColormap *colormap;
GdkFont *sfont, *tfont;
GdkColor red, darkgrey, timecolor, trackcolor;

GtkTooltips *tooltips;

int timeonly = FALSE, status_height, status_width, playid=-1;
int configured = FALSE, old_status=-1;
tcd_properties props;

/* Prototypes */
void draw_status( void );
void delete_event (GtkWidget *widget, gpointer *data);
void create_warn( char *, char * );

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
			cd.play_method = NORMAL;
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
			gtracked();
			break;
		case QUIT:
			delete_event(NULL,NULL);
			break;		
		case ABOUT:
			about_cb(NULL,NULL);
			break;	
		case CDDB:
			gcddb();
			break;
		case PROPS:
			properties_cb(NULL, NULL);
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

	gtk_tooltips_set_tip( tooltips, button, tooltip, "" );

	return button;
}	                        

GtkWidget* make_button_with_pixmap( char *pic, GtkWidget *box, int func,
	gint expand, gint fill, gchar *tooltip )
{
	GtkWidget *button;
	GtkWidget *pixmap;
	char tmp[256];
	
	sprintf( tmp, "tcd/%s.xpm", pic );
#ifdef DEBUG
	g_print( "loading: %s\n", gnome_pixmap_file(tmp) );
#endif
	pixmap = gnome_pixmap_new_from_file( gnome_pixmap_file(tmp) );

	button = gtk_button_new();
	gtk_container_add( GTK_CONTAINER(button), pixmap );
	gtk_box_pack_start( GTK_BOX (box), button, expand, fill, 0 );

	if( func != -1 )
		gtk_signal_connect(GTK_OBJECT (button), "clicked", \
		        GTK_SIGNAL_FUNC (callback), (gpointer*)func );
	
	gtk_tooltips_set_tip( tooltips, button, tooltip, "" );

	gtk_widget_show_all(button);
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
	tcd_init_disc(&cd,create_warn);
	
	cd.play_method = NORMAL;
	return 1;
}

GtkWidget* make_changer_buttons( void )
{
	GtkWidget *box;
	char tmp[5];
	int i;

	box = gtk_hbox_new( FALSE, 0 );
	changer_buttons = g_malloc( sizeof(GtkWidget)*cd.nslots );
		
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
	GtkWidget *box, *handle, *pixmap;
	
	box = gtk_hbox_new( TRUE, 0 );
	handle = gtk_handle_box_new();

	playbutton = make_button_with_pixmap( "play", box, -1, TRUE, TRUE, TT_PLAY );
	status_changed();

	make_button_with_pixmap( "stop", box, STOP, TRUE, TRUE, TT_STOP );
	make_button_with_pixmap( "eject", box, EJECT, TRUE, TRUE, TT_EJECT );

	propsbutton = gtk_button_new();
	pixmap = gnome_stock_pixmap_widget( window, GNOME_STOCK_PIXMAP_PREFERENCES );
	gtk_container_add(GTK_CONTAINER(propsbutton), pixmap );
	gtk_signal_connect(GTK_OBJECT(propsbutton), "clicked",
		GTK_SIGNAL_FUNC(callback), (gpointer*)PROPS );
        gtk_box_pack_start(GTK_BOX(box), propsbutton, TRUE, TRUE, 0);
        gtk_tooltips_set_tip( tooltips, propsbutton, TT_PROPS, "" );
	
	if( props.handle )
	{
		gtk_widget_show(box);
		gtk_container_add( GTK_CONTAINER(handle), box );
		row1=handle;
	}
	else
		row1=box;
	return row1;
}

GtkWidget* make_row2( void )
{
	GtkWidget *box, *handle;
	box = gtk_hbox_new( TRUE, 0 );
	handle = gtk_handle_box_new();

	make_button_with_pixmap("prev_t", box, PREV_T, TRUE, TRUE, TT_PREV_TRACK );
	make_button_with_pixmap("rw", box, RW, TRUE, TRUE, TT_REWIND );
	make_button_with_pixmap("ff", box, FF, TRUE, TRUE, TT_FF );
	make_button_with_pixmap("next_t", box, NEXT_T, TRUE, TRUE, TT_NEXT_TRACK );

	if( props.handle )
	{
		gtk_widget_show(box);
		gtk_container_add( GTK_CONTAINER(handle), box );
		row2=handle;
	}
	else
		row2=box;
	return row2;
}

GtkWidget* make_row3( void )
{
	char tmp[128];
	GtkWidget *box, *bbox, *pm;
	GtkWidget *button, *gotolabel;
	GtkWidget *pixmap, *handle;
	
        box = gtk_hbox_new( TRUE, 0 );
	bbox = gtk_vbox_new( FALSE, 0 );
	handle = gtk_handle_box_new();

	cddbbutton = make_button_with_pixmap( "cddb", box, CDDB, TRUE, TRUE,  TT_CDDB );
	trackeditor =make_button_with_pixmap( "edit", box, TRACKLIST, TRUE, TRUE, TT_TRACKED );

	gotobutton = gtk_button_new();
	pixmap = gnome_pixmap_new_from_file( gnome_pixmap_file("tcd/goto.xpm") );
	gtk_box_pack_start( GTK_BOX(bbox), pixmap, FALSE, FALSE, 0 );
        gtk_container_add( GTK_CONTAINER(gotobutton), bbox);
	gtk_box_pack_start( GTK_BOX(box), gotobutton, TRUE, TRUE, 0);
	
	make_button_with_pixmap( "power", box, QUIT, TRUE, TRUE, "Quit" );

	gtk_widget_show_all(box);
	
	if( props.handle )
	{
		gtk_widget_show(box);
		gtk_container_add( GTK_CONTAINER(handle), box );
		row3=handle;
	}
	else
		row3=box;
	return row3;
}

void setup_colors( void )
{
	GdkColormap *colormap;
	
	colormap = gtk_widget_get_colormap(status_area);
	
	gdk_color_parse("#353535", &darkgrey);
	gdk_color_alloc(colormap, &darkgrey);

	gdk_color_parse("#FF0000", &red);
	gdk_color_alloc(colormap, &red);
	draw_status();
}

void setup_pixmaps( void )
{
	gchar tmp[128];
	int i;
	for( i=0; i < PLAY_METHOD_END-1; i++ )
	{
		sprintf( tmp, "tcd/%s", play_methods[i] );
		play_method_pixmap[i] = 
			gnome_pixmap_new_from_file( gnome_pixmap_file(tmp) );
	}
}

void draw_time_playing( GdkGC *gc )
{
	int pos, end, cur, min, sec;
	switch( time_display_type )
	{
		case TRACK_ASC: /* track time ascending */
			led_draw_time(status_db, status_area,
				32,4, cd.t_min, cd.t_sec);
			break;
		case TRACK_DEC: /* track time decending */
			cur = cd.cur_pos_rel;
			end = (cd.trk[cd.cur_t].tot_min*60)+
			       cd.trk[cd.cur_t].tot_sec;
			pos = end-cur;
			min = pos/60;
			sec = pos-(pos/60)*60;
			led_draw_time(status_db, status_area,
				32,4, min, sec );
			break;
		case DISC_ASC: /* disc time ascending */
			led_draw_time(status_db, status_area,
				32,4, cd.cd_min, cd.cd_sec);
			break;
		default:
			break;
	}		
}

void draw_titles( GdkGC *gc )
{
	gdk_gc_set_foreground( gc, &red );
	gdk_draw_text(status_db,sfont,gc,4,32, cd.artist, 
		strlen(cd.artist));
	gdk_draw_text( status_db,sfont,gc,4,32+12, cd.album, 
		strlen(cd.album));
	gdk_draw_text( status_db,sfont,gc,4,32+24, cd.trk[cd.cur_t].name, 
		strlen(cd.trk[cd.cur_t].name));
        gtk_window_set_title( GTK_WINDOW(window), cd.trk[cd.cur_t].name );
}

void draw_time_scanning( GdkGC *gc )
{
	gdk_gc_set_foreground( gc, &red );
	gdk_draw_text(status_db,sfont,gc,4,32, "(Scanning)", 
		10);
        gtk_window_set_title( GTK_WINDOW(window), "(Scanning)" );
}

void draw_status( void )
{
	char tmp[128], *album;
	GtkWidget *pixmap;
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

	gdk_gc_set_foreground( gc, &darkgrey );

	gdk_draw_line( status_db,gc,0,22,status_area->allocation.width,22 );
	gdk_draw_line( status_db,gc,28,0,28,22 );
	gdk_draw_line( status_db,gc,79,0,79,22 );

	if( !cd.err )
	{
		switch( cd.sc.cdsc_audiostatus )
		{
			case CDROM_AUDIO_INVALID:
				strcpy( tmp,"No Disc" );
				gdk_draw_text( status_db,sfont,gc,4,32,tmp, strlen(tmp) );
				draw_time_scanning(gc);
				break;
			case CDROM_AUDIO_PLAY:
				if( cd.isplayable ) /* we can't be playing if we can't play */
				{
					draw_time_playing(gc);
					draw_titles(gc);
					led_draw_track(status_db, status_area, 
						4,4, cd.cur_t);
				}
				else
					draw_time_scanning(gc);
				break;
			case CDROM_AUDIO_PAUSED:
				draw_titles(gc);
				break;
			case CDROM_AUDIO_COMPLETED:
			case CDROM_AUDIO_NO_STATUS:
				led_stop_time(status_db, status_area, 34,4 );
				led_stop_track(status_db, status_area, 4,4 );
				if( cd.isplayable )
					draw_titles(gc);
				else
					draw_time_scanning(gc);
				break;
			case CDROM_AUDIO_ERROR:
				strcpy( tmp,"Error" );
				gdk_draw_text( status_db,sfont,gc,4,32,tmp, strlen(tmp) );
				draw_time_scanning(gc);
				break;
			default:
				break;
		}
	}
	else
	{
		led_stop_time(status_db, status_area, 34,4 );
		led_stop_track(status_db, status_area, 4,4 );
		strcpy( tmp, cd.errmsg );
		gdk_draw_text( status_db,sfont,gc,
			4, 32, tmp, strlen(tmp) );
		draw_time_scanning(gc);
	}
	
	gdk_draw_pixmap(status_db,
			status_area->style->fg_gc[GTK_WIDGET_STATE(status_area)],
		        GNOME_PIXMAP(play_method_pixmap[cd.play_method])->pixmap,
		        0, 0,
		        80, -2,
		        24, 24);

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
	if( cd.err || !cd.isdisk )
        {
		tcd_readtoc(&cd);
		tcd_readdiskinfo(&cd);
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

gint status_changed(void)
{
	if( old_status != cd.sc.cdsc_audiostatus )
	{
		GtkWidget *pixmap;
		char tmp[256];
	
		old_status = cd.sc.cdsc_audiostatus;
 		sprintf( tmp, "tcd/%s.xpm", 
 			(old_status==CDROM_AUDIO_PLAY)?"pause":"play" );
		
		gtk_widget_destroy(GTK_BUTTON(playbutton)->child);
		GTK_BUTTON(playbutton)->child = NULL;

		pixmap = gnome_pixmap_new_from_file( gnome_pixmap_file(tmp) );
		gtk_widget_show(pixmap);
		gtk_container_add( GTK_CONTAINER(playbutton), pixmap );
		
		if( playid > 0 )
			gtk_signal_disconnect(GTK_OBJECT(playbutton), playid);
		playid = gtk_signal_connect(GTK_OBJECT(playbutton), "clicked", \
		        GTK_SIGNAL_FUNC (callback),
		        (old_status==CDROM_AUDIO_PLAY)?(gpointer*)PAUSE:(gpointer*)PLAY );
		old_status = cd.sc.cdsc_audiostatus;
	}
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

gint fast_timer( gpointer *data )
{
	char buf[128];
	tcd_gettime(&cd);
	status_changed();
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
				    gtk_widget_get_visual(status_area)->depth );
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

static gint status_click_event(GtkWidget *widget, GdkEvent *event)
{
	GdkEventButton *e = (GdkEventButton *)event;
	int x, y;
	
	x = floor(e->x);
	y = floor(e->y);
		                
	if( event->type == GDK_BUTTON_PRESS )
	{
		if( e->button > 1 )
		{
			about_cb(NULL, NULL);
			return FALSE;
		}
		if( x > 80 &&
		    y > 0 &&
		    x < 104 &&
		    y < 24 )
		{
			cd.play_method++;
			if( cd.play_method > NORMAL )
				cd.play_method = 0;
			if( cd.play_method == REPEAT_TRK )
				cd.repeat_track = cd.cur_t;

			draw_status();
		}
	}
	
	return FALSE;
}

void setup_time_display( void )
{
	GtkWidget *handle1, *frame, *row;
	
	lowerbox = gtk_hbox_new( FALSE, 5 );
	
	vol = gtk_adjustment_new (0.0, 0.0, 256.0, 0.1, 1.0, 1.0);
	volume = gtk_hscale_new(GTK_ADJUSTMENT(vol));
	gtk_range_set_update_policy( GTK_RANGE(volume), GTK_UPDATE_CONTINUOUS );
        gtk_scale_set_draw_value( GTK_SCALE(volume), FALSE );
	gtk_signal_connect( GTK_OBJECT(vol), "value_changed",
        	(GtkSignalFunc)volume_changed, NULL);

#ifdef TCD_CHANGER_ENABLED
	changer_box = make_changer_buttons();
#endif

	status_area = gtk_drawing_area_new();
	gtk_signal_connect( GTK_OBJECT (status_area), "expose_event",
        	(GtkSignalFunc)status_expose_event, NULL);
        gtk_signal_connect( GTK_OBJECT(status_area),"configure_event",
        	(GtkSignalFunc)status_configure_event, NULL);
	gtk_signal_connect( GTK_OBJECT(status_area),"button_press_event",
        	(GtkSignalFunc)status_click_event, NULL);
	gtk_widget_set_usize( status_area, 75, 60 );

	gtk_tooltips_set_tip( tooltips, status_area, TT_TIME, "" );
        
        gtk_widget_set_events (status_area, GDK_EXPOSURE_MASK
 					   | GDK_LEAVE_NOTIFY_MASK
                                           | GDK_BUTTON_PRESS_MASK
                                           | GDK_POINTER_MOTION_MASK
                                           | GDK_POINTER_MOTION_HINT_MASK);	

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_IN );
	gtk_container_add(GTK_CONTAINER(frame), status_area);
	status_table = gtk_vbox_new( FALSE, 4 );
	gtk_box_pack_start( GTK_BOX(upper_box), status_table, TRUE, TRUE, 2 );

#ifdef TCD_CHANGER_ENABLED
	gtk_box_pack_start( GTK_BOX(lowerbox), changer_box, FALSE, FALSE, 0 );
#endif
/*        row = make_row3();
	gtk_box_pack_start( GTK_BOX(lowerbox), row, TRUE, TRUE, 0 );
	gtk_widget_show(row);
*/
	gtk_box_pack_end( GTK_BOX(lowerbox), volume, TRUE, TRUE, 5 );

	if( props.handle )
	{
		handle1 = gtk_handle_box_new();
		gtk_container_add(GTK_CONTAINER(handle1), frame );
		gtk_box_pack_start( GTK_BOX(status_table), handle1, TRUE, TRUE, 4 );
	}
	else
		gtk_box_pack_start( GTK_BOX(status_table), frame, TRUE, TRUE, 4);
		                
	gtk_box_pack_start( GTK_BOX(status_table), lowerbox, FALSE, FALSE, 4 );
	gtk_widget_show_all(status_table);
	return;
}

void setup_rows( void )
{
	GtkWidget *pixmap;
	GtkWidget *ttbox = gtk_vbox_new( FALSE, 1 );
	sep = gtk_hseparator_new();

	vbox = gtk_vbox_new( FALSE, 4 );
	upper_box = gtk_hbox_new( TRUE, 4 );
	button_box = gtk_vbox_new( TRUE, 0 );

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

	return;
}

void setup_fonts(void)
{
        if( tfont )
		gdk_font_unref(tfont);
        if( sfont )
		gdk_font_unref(sfont);

	if(props.statusfont)
	        sfont = gdk_font_load( props.statusfont );
	else
		sfont = NULL;

	if(props.trackfont)
		tfont = gdk_font_load( props.trackfont );
	else
		tfont = NULL;
}

void init_window(void)
{
	setup_fonts();

        window = gnome_app_new( "gtcd", "TCD 2.0" );
        gtk_window_set_title( GTK_WINDOW(window), PACKAGE" "VERSION" " );
        gtk_window_set_wmclass( GTK_WINDOW(window), "main_window","gtcd" );

        gtk_signal_connect( GTK_OBJECT(window), "delete_event",
                GTK_SIGNAL_FUNC(delete_event), NULL);

        gtk_container_border_width( GTK_CONTAINER(window), 4 );
        gtk_widget_realize(window);

	tooltips = gtk_tooltips_new();
	if( props.tooltip )
		gtk_tooltips_enable(tooltips);
	else
		gtk_tooltips_disable(tooltips);
}

void create_warn( char *message_text, char *type )
{
	gtk_widget_show(gnome_message_box_new(message_text, type,
		GNOME_STOCK_BUTTON_OK, NULL));
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
	
        tcd_init_disc(&cd, (WarnFunc)create_warn);
        
	init_window();
	setup_rows();
	make_gotomenu();
	setup_time_display();
	setup_colors();
	setup_pixmaps();
	led_init(window);
	
	time_display_type = TRACK_DEC;
	/* Initialize some timers */
	if( cd.isplayable ) tcd_gettime(&cd);

	gtk_timeout_add(1000, (GtkFunction)slow_timer, NULL);
	gtk_timeout_add(250, (GtkFunction)fast_timer, NULL);
	titlelabel_f = TRUE;
	
        gnome_app_set_contents( GNOME_APP(window), vbox );
	
        gtk_widget_show_all(window);
	
	gtk_main ();
	gnome_config_sync();
        return 0;
}

void change_orient(int id, int orient)
{
} 
        
void session_save(int id, const char *cfgpath, const char *globcfgpath)
{
}
