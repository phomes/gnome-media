/* This file is part of TCD.
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

#include <config.h>

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

#include <gnome.h>
#include <gdk/gdkkeysyms.h>

#include "linux-cdrom.h"

#include "gtracked.h"
#include "prefs.h"
#include "led.h"
#include "callbacks.h"
#include "gtcd_public.h"
#include "keybindings.h"

/* time display types */
enum { TIME_FIRST=-1, TRACK_E, TRACK_R, DISC_E, DISC_R, TIME_LAST };

char *display_types[] = 
{
    N_("trk-e"),
    N_("trk-r"),
    N_("dsc-e"),
    N_("dsc-r")
};

char *play_types[] = 
{
    N_("loop-cd"),
    N_("loop-t"),
    N_("normal"),
    N_("random")
};

/* globals */
cd_struct cd;
int titlelabel_f = 0;
int gotoi;

GtkWidget *row, *upper_box;
GtkWidget *button_box;
GtkWidget *tracktime_label, *trackcur_label;
GtkWidget *cdtime_label, *changer_box, *playbutton;
GtkWidget *status_table, *status_area;
GtkWidget *volume, *window;
GtkWidget *gotomenu = NULL, *gotobutton, *main_box;
GdkPixmap *status_db = NULL;
GtkWidget **changer_buttons;
GtkObject *vol;
GdkColormap *colormap;
GdkFont *sfont, *tfont;
GdkColor track_color, darkgrey, timecolor, blue;
GdkGC *gc;
GtkAccelGroup *accel;

GtkTooltips *tooltips;

int timeonly = FALSE, status_height, status_width, playid=-1;
int configured = FALSE, old_status=-1, max,tfont_height;
unsigned int cur_goto_id, release_t=0, roll_t=0;
tcd_prefs prefs;
int cddb=0, max_title_width;
       
/* prototypes */
void status_changed(void);
gint slow_timer(gpointer *data);
gint roll_timer(gpointer *data);
gint release_timer(gpointer *data);
int skip_cb(GtkWidget *widget, GdkEvent *event, gpointer *data);
gint changer_callback(GtkWidget *widget, gpointer *data);
GtkWidget* make_changer_buttons(void);
GtkWidget* make_small_buttons(void);
GtkWidget* create_buttons(void);
void setup_colors(void);
void draw_time_playing(void);
void draw_titles(void);
void draw_time_scanning(void);
gint volume_changed(GtkWidget *widget, gpointer *data);
gint launch_gmix(GtkWidget *widget, GdkEvent *event, gpointer data);
gint fast_timer(gpointer *data);
void setup_time_display(GtkWidget *table);
void setup_fonts(void);
void init_window(void);
GtkWidget* make_button_with_pixmap(char *pic, GtkSignalFunc func,
				   Shortcut *key,  gchar *tooltip);
GtkWidget* make_button_stock(char *stock, GtkSignalFunc func,
			     Shortcut *key, gchar *tooltip);
void reload_info(int signal);
void exit_action(void);
void start_action(void);
void setup_keys(void);
void adjust_status_size(void);

/* functions */
gint roll_timer(gpointer *data)
{
    tcd_play_seconds(&cd, GPOINTER_TO_INT(data));
    tcd_gettime(&cd);
    draw_status();
    return TRUE;
}

gint release_timer(gpointer *data)
{
    roll_t = gtk_timeout_add(35, (GtkFunction)roll_timer, data);
    release_t = 0;
    return FALSE;
}

int skip_cb(GtkWidget *widget, GdkEvent *event, gpointer *data)
{
    if(event->type == GDK_BUTTON_PRESS)
    {
	release_t = gtk_timeout_add(750, (GtkFunction)release_timer, data);
    }
    else if(event->type == GDK_BUTTON_RELEASE)
    {
	if(release_t)
	{
	    gtk_timeout_remove(release_t);
	    if(GPOINTER_TO_INT(data) > 0)
	    {
		if(cd.cur_t < cd.last_t)
		{   
		    cd.cur_t++;
		    tcd_playtracks(&cd,cd.cur_t, cd.last_t, prefs.only_use_trkind);
		    if(cd.play_method==REPEAT_TRK)
			cd.repeat_track = cd.cur_t;
		}
	    }
	    else
	    {
		if( cd.cur_t > cd.first_t )
		{
		    if( (cd.t_sec+(cd.t_min*60)) < 10 )
			cd.cur_t--;
		    tcd_playtracks( &cd,cd.cur_t, cd.last_t, prefs.only_use_trkind);
				                                                             
		    if( cd.play_method==REPEAT_TRK )
			cd.repeat_track = cd.cur_t;
		}
	    }
	}   
	if(roll_t)
	    gtk_timeout_remove(roll_t);
			
	release_t = 0;
	roll_t = 0;
    }
	
    return FALSE;	
}

GtkWidget* make_button_with_pixmap(char *pic, GtkSignalFunc func,
				   Shortcut *key,  gchar *tooltip)
{
    GtkWidget *button;
    GtkWidget *pixmap;
    char tmp[256];
    char *name;
    
    g_snprintf( tmp, 255, "tcd/%s.xpm", pic);
    name = gnome_pixmap_file(tmp);
#ifdef DEBUG
    g_print( "loading: %s\n", name );
#endif
    pixmap = gnome_pixmap_new_from_file(name);
    g_free(name);
    
    button = gtk_button_new();
    gtk_container_add( GTK_CONTAINER(button), pixmap );

    if(key)
	add_key_binding(button, "clicked", tooltip, key);

    if(func)
	gtk_signal_connect(GTK_OBJECT (button), "clicked",
			   GTK_SIGNAL_FUNC(func), NULL);
    
    gtk_tooltips_set_tip(tooltips, button, tooltip, "");
    
    return button;
}	                        

GtkWidget* make_button_stock(char *stock, GtkSignalFunc func,
			     Shortcut *key, gchar *tooltip)
{
    GtkWidget *button;
    GtkWidget *pixmap;
    
    pixmap = gnome_stock_pixmap_widget_new(window,stock);
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), pixmap);

    if(key)
	add_key_binding(button, "clicked", tooltip, key);

    if(func)
	gtk_signal_connect(GTK_OBJECT (button), "clicked",
			   GTK_SIGNAL_FUNC(func), NULL);
	
    gtk_tooltips_set_tip( tooltips, button, tooltip, "" );
	
    return button;
}	                        

GtkWidget* make_changer_buttons( void )
{
    GtkWidget *box;
    GtkStyle *style;
    char tmp[5];
    int i;
    
    box = gtk_hbox_new(FALSE, 0);
    changer_buttons = g_new(GtkWidget *, cd.nslots);
    
    for(i=0; i < cd.nslots && i < 12; i++)
    {
	g_snprintf( tmp, 4, "%d", i+1 );
	changer_buttons[i] = gtk_button_new_with_label(tmp);

	style = gtk_widget_get_style(GTK_BUTTON(changer_buttons[i])->child);
	gdk_font_unref(style->font);
	style->font = gdk_font_load("-misc-fixed-*-*-*-*-10-*-*-*-*-*-*-*");
	gdk_font_ref(style->font);
	gtk_widget_set_style(GTK_BUTTON(changer_buttons[i])->child, style);
	gdk_font_unref(style->font);
	gtk_style_unref(style);

	gtk_box_pack_start(GTK_BOX(box), changer_buttons[i], TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(changer_buttons[i]), "clicked", \
			   GTK_SIGNAL_FUNC(changer_cb), GINT_TO_POINTER(i));
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

GtkWidget *make_small_buttons(void)
{
    GtkWidget *b1, *b2, *b3;
    GtkWidget *table;
    
    table = gtk_table_new(TRUE, 6,1);

    b1 = make_button_stock(GNOME_STOCK_PIXMAP_PROPERTIES, edit_window, &prefs.tracked, _("Open track editor"));
    b2 = make_button_stock(GNOME_STOCK_PIXMAP_PREFERENCES, preferences, NULL, _("Preferences"));
    b3 = make_button_stock(GNOME_STOCK_PIXMAP_QUIT, quit_cb, &prefs.quit, _("Quit"));

    gtk_table_attach_defaults(GTK_TABLE(table), b1, 0, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), b2, 2, 4, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), b3, 4, 6, 0, 1);

    return table;
}

GtkWidget* create_buttons(void)
{
    GtkWidget *table;
    GtkWidget *b1, *b2, *b3;
    GtkWidget *pixmap;
    GtkWidget *rw, *ff;
    char *name;

    table = gtk_table_new(TRUE, 3, 4);

/* TOP ROW */ 
     
    playbutton = make_button_with_pixmap("play", NULL, &prefs.play, _("Play/Pause"));
    status_changed();
    b1 = playbutton;
    
    b2 = make_button_with_pixmap( "stop", stop_cb, &prefs.stop, _("Stop") );
    b3 = make_button_with_pixmap( "eject", eject_cb, &prefs.eject, _("Eject") );
    
    gtk_table_attach_defaults(GTK_TABLE(table), b1, 0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), b2, 1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), b3, 2, 3, 0, 1);

/* MIDDLE ROW */
    rw = make_button_with_pixmap("rw", NULL, NULL, _("Skip backwards"));
    ff = make_button_with_pixmap("ff", NULL, NULL, _("Skip forwards"));

    gtk_widget_set_events(rw, GDK_BUTTON_PRESS_MASK
			  | GDK_BUTTON_RELEASE_MASK);
    gtk_widget_set_events(ff, GDK_BUTTON_PRESS_MASK
			  | GDK_BUTTON_RELEASE_MASK);

    add_key_binding(rw, "clicked", _("Skip backwards"), &prefs.back);
    add_key_binding(rw, "clicked", _("Skip forwards"), &prefs.forward);

    gtk_signal_connect(GTK_OBJECT(ff), "event",
		       GTK_SIGNAL_FUNC(skip_cb), GINT_TO_POINTER(2));
    gtk_signal_connect(GTK_OBJECT(rw), "event",
		       GTK_SIGNAL_FUNC(skip_cb), GINT_TO_POINTER(-2));

/* Create goto button  */
    gotobutton = gtk_button_new();
    name = gnome_pixmap_file("tcd/goto.xpm");
    pixmap = gnome_pixmap_new_from_file(name);
    g_free(name);
    gtk_container_add(GTK_CONTAINER(gotobutton), pixmap);

    gtk_table_attach_defaults(GTK_TABLE(table), rw, 0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), ff, 1, 2, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), gotobutton, 2, 3, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), make_small_buttons(), 0, 3, 2, 3);

    return table;
}

void setup_colors( void )
{
    char tmp[16];
    GdkColormap *colormap;
    
    colormap = gtk_widget_get_colormap(status_area);
    
    gdk_color_parse("#888888", &darkgrey);
    gdk_color_alloc(colormap, &darkgrey);

    gdk_color_parse("#0000ff", &blue);
    gdk_color_alloc(colormap, &blue);

    g_snprintf(tmp, 15, "#%02X%02X%02X", 
	       prefs.trackcolor_r,
	       prefs.trackcolor_g,
	       prefs.trackcolor_b);

    gdk_color_parse(tmp, &track_color);
    gdk_color_alloc(colormap, &track_color);
    draw_status();
}

void draw_time_playing(void)
{
    int pos, end, cur, min, sec;
    switch( prefs.time_display )
    {
    case TRACK_E: /* track time ascending */
	led_draw_time(status_db, status_area,
		      48,4, cd.t_min, cd.t_sec);
	break;
    case TRACK_R: /* track time decending */
	cur = cd.cur_pos_rel;
	end = (cd.trk[cd.cur_t].tot_min*60)+
	    cd.trk[cd.cur_t].tot_sec;
	pos = end-cur;
	min = pos/60;
	sec = pos-(pos/60)*60;
	led_draw_time(status_db, status_area,
		      48,4, min, sec);
	break;
    case DISC_E: /* disc time ascending */
	led_draw_time(status_db, status_area,
		      48,4, cd.cd_min, cd.cd_sec);
	break;
    case DISC_R:
	cur = cd.cur_pos_abs;
	end = (cd.trk[cd.last_t+1].toc.cdte_addr.msf.minute
	       *60)+cd.trk[cd.last_t+1].toc.cdte_addr.msf.second;
	pos = end-cur;
	min = pos/60;
	sec = pos-(pos/60)*60;
	led_draw_time(status_db, status_area,
		      48, 4, min, sec);
	break;
    default:
	break;
    }		
}

void draw_titles(void)
{
    int inc;

    gdk_gc_set_foreground( gc, &track_color );
    inc = (tfont->ascent+tfont->descent)-2;

    gdk_draw_text(status_db,tfont,gc,4,39, cd.artist,
		  strlen(cd.artist));
    gdk_draw_text(status_db,tfont,gc,4,39+inc, cd.album, 
		  strlen(cd.album));
    
    /* make sure the title fits */
    {
	char tmp[512], tmp2[512], tmp3[512];
	int width;

	width = strlen(cd.trk[cd.cur_t].name);
	
	if(width > 25)
	{
	    strncpy(tmp, cd.trk[cd.cur_t].name, 11);
	    tmp[11] = 0;
	    strncpy(tmp2, cd.trk[cd.cur_t].name+(width-11), 11);
	    tmp2[11] = 0;
	    
	    g_snprintf(tmp3, 511, "%s...%s", tmp, tmp2);

	    gdk_draw_text(status_db,tfont,gc,4,39+inc+inc,
			  tmp3, strlen(tmp3));
	}
	else
	{
	    gdk_draw_text(status_db,tfont,gc,4,39+inc+inc, 
			  cd.trk[cd.cur_t].name,
			  strlen(cd.trk[cd.cur_t].name));
	}
    }
    if (cd.cur_t == 0)
      gtk_window_set_title(GTK_WINDOW(window), cd.dtitle);
    else
      gtk_window_set_title(GTK_WINDOW(window), cd.trk[cd.cur_t].name);
}

void draw_time_scanning(void)
{
    gdk_gc_set_foreground( gc, &track_color );
    gdk_draw_text(status_db,tfont,gc,4,39+(tfont->ascent+tfont->descent)-2, 
                  _("(Scanning)"),
                  strlen( _("(Scanning)")));
    gtk_window_set_title( GTK_WINDOW(window), _("(Scanning)") );
}

void draw_status(void)
{
    char tmp[128];

    if(!configured)
	return;

    /* Erase Rectangle */
    gdk_draw_rectangle( status_db, 
			status_area->style->black_gc,
			TRUE, 0,0,
			status_area->allocation.width,
			status_area->allocation.height );

    if( !cd.err )
    {
	switch( cd.sc.cdsc_audiostatus )
	{
	case CDROM_AUDIO_INVALID:
	    strcpy(tmp, _("No Disc"));
	    gdk_draw_text(status_db,tfont,gc,4,39,tmp, strlen(tmp));
	    draw_time_scanning();
	    break;
	case CDROM_AUDIO_PLAY:
	    if( cd.isplayable ) /* we can't be playing if we can't play */
	    {
		draw_time_playing();
		draw_titles();
		led_draw_track(status_db, status_area, 
			       4,4, cd.cur_t);
	    }
	    else
		draw_time_scanning();
	    break;
	case CDROM_AUDIO_PAUSED:
	    draw_titles();
	    break;
	case CDROM_AUDIO_COMPLETED:
	case CDROM_AUDIO_NO_STATUS:
	    led_stop_time(status_db, status_area, 34,4 );
	    led_stop_track(status_db, status_area, 4,4 );
	    if(cd.isplayable)
		draw_titles();
	    else
		draw_time_scanning();
	    break;
	case CDROM_AUDIO_ERROR:
	    strcpy( tmp, _("Error") );
	    gdk_draw_text(status_db,tfont,gc,4,39,tmp, strlen(tmp));
	    draw_time_scanning();
	    break;
	default:
	    break;
	}
	if(cd.isplayable)
	{		
	    char *dtmsg = gettext(display_types[prefs.time_display]);
	    char *pmsg =  gettext(play_types[cd.play_method]);
	    gdk_gc_set_foreground(gc, &darkgrey);
	    gdk_draw_text( status_db,sfont,gc,48,26, 
	    		   dtmsg, 
			   strlen(dtmsg));
	    gdk_draw_text( status_db,sfont,gc,2,26, 
			   pmsg,
			   strlen(pmsg));
	}
    }
    else
    {
	strncpy(tmp, cd.errmsg, sizeof(tmp));
	gdk_draw_text(status_db,tfont,gc,
		      4, 39, tmp, strlen(tmp));
	gtk_window_set_title(GTK_WINDOW(window), "TCD "VERSION" ");
    }
    if(cddb)
    {
	gdk_gc_set_foreground(gc, &blue);
	gdk_draw_text(status_db,sfont,gc,
		      82, 26, "CDDB", 4);
    }

    /* Finally, update the display */
    gdk_draw_pixmap(status_area->window,
		    status_area->style->fg_gc[GTK_WIDGET_STATE(status_area)],
		    status_db,
		    0, 0,
		    0, 0,
		    status_area->allocation.width,
		    status_area->allocation.height);
}

gint slow_timer( gpointer *data )
{
    static char *lock_file = NULL;
    
    tcd_post_init(&cd);

    /* see if we need to make a new menu */
    if(cd.cddb_id != cur_goto_id)
    {
	make_goto_menu();
	update_editor();
    }

    /* see if we need to repeat */
    if( cd.sc.cdsc_audiostatus != CDROM_AUDIO_PLAY &&
	cd.sc.cdsc_audiostatus != CDROM_AUDIO_PAUSED )
    {
	if( cd.play_method == REPEAT_CD )
	    tcd_playtracks( &cd, cd.first_t, cd.last_t, prefs.only_use_trkind);
    }

    /* is a cddb operation going on? */
    if (!lock_file)
      lock_file = gnome_util_home_file(".cddbstatus_lock");
    cddb = g_file_test(lock_file, G_FILE_TEST_EXISTS);
    
    draw_status(); 
    return 1;
}

void status_changed(void)
{
    if(old_status != cd.sc.cdsc_audiostatus)
    {
	GtkWidget *pixmap;
	GtkSignalFunc func;
	char tmp[256];
	char *name;
	
	old_status = cd.sc.cdsc_audiostatus;
	g_snprintf(tmp, 255, "tcd/%s.xpm", 
		    (old_status==CDROM_AUDIO_PLAY)?"pause":"play");

	gtk_widget_destroy(GTK_BUTTON(playbutton)->child);
	GTK_BUTTON(playbutton)->child = NULL;

	name = gnome_pixmap_file(tmp);
	pixmap = gnome_pixmap_new_from_file(name);
	g_free (name);
	gtk_widget_show(pixmap);
	gtk_container_add( GTK_CONTAINER(playbutton), pixmap );

	if(old_status == CDROM_AUDIO_PLAY)
	    func = GTK_SIGNAL_FUNC(pause_cb);
	else
	    func = GTK_SIGNAL_FUNC(play_cb);
	
	if(playid > 0)
	    gtk_signal_disconnect(GTK_OBJECT(playbutton), playid);

	playid = gtk_signal_connect(GTK_OBJECT(playbutton), "clicked",
				    func, NULL);
    }
}

gint volume_changed( GtkWidget *widget, gpointer *data )
{
    if(!data)
    {
	tcd_set_volume(&cd, (int)floor(GTK_ADJUSTMENT(vol)->value));
    }
    draw_status();
    return 1;
}

gint launch_gmix( GtkWidget *widget, GdkEvent *event, gpointer data ) 
{
    if(event->type==GDK_2BUTTON_PRESS)
        if (fork()==0)
            execlp("gmix","",NULL); 

    return FALSE;
}

void make_goto_menu()
{
    char buf[TRK_NAME_LEN];
    int i;
    GtkWidget *item;

    gotomenu = gtk_menu_new();

    for(i=1; i <= cd.last_t; i++)
    {
	g_snprintf(buf, TRK_NAME_LEN-1, "%2d - %s", i, cd.trk[C(i)].name);
	item = gtk_menu_item_new_with_label(buf);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(gotomenu), item);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(goto_track_cb), GINT_TO_POINTER(i));
    }
    if(gotoi) gtk_signal_disconnect(GTK_OBJECT(gotobutton), gotoi);
    gotoi = gtk_signal_connect_object(GTK_OBJECT(gotobutton), "event",
				      GTK_SIGNAL_FUNC(button_press), GTK_OBJECT(gotomenu));

    cur_goto_id = cd.cddb_id;
    adjust_status_size();
}

void adjust_status_size(void)
{
    int t;
    max_title_width = 0;

    t = gdk_string_width(tfont, cd.artist);
    if(max_title_width < t)
	max_title_width = t;
    t = gdk_string_width(tfont, cd.album);
    if(max_title_width < t)
	max_title_width = t;
    t = gdk_string_width(tfont, " ") * 25;
    if(max_title_width < t) max_title_width = t;
    if(max_title_width < 100) max_title_width = 100; /* make sure we can fit time */
    gtk_widget_set_usize(status_area, max_title_width+8, tfont_height+27);
}

gint fast_timer( gpointer *data )
{
    tcd_gettime(&cd);

    status_changed();
    if((cd.play_method==REPEAT_TRK) && (cd.cur_t != cd.repeat_track))
	tcd_playtracks(&cd, cd.repeat_track, -1, prefs.only_use_trkind);
    return 1;
}

static gint status_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
    static int first=TRUE;
    configured = TRUE;	
    if( status_db )
	gdk_pixmap_unref(status_db);
		
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
    GTK_ADJUSTMENT(vol)->value = (double)tcd_get_volume(&cd);
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
	    return FALSE;

	if( x > 2 &&
	    y > 0 &&
	    x < 48 &&
	    y < 32 )
	{
	    cd.play_method++;
	    if( cd.play_method > NORMAL )
		cd.play_method = 0;
	    if( cd.play_method == REPEAT_TRK )
		cd.repeat_track = cd.cur_t;

	    draw_status();
	    return TRUE;
	}
	if( x > 48 &&
	    y > 0 &&
	    x < 100 &&
	    y < 32 )
	{
	    prefs.time_display++;
	    if( prefs.time_display >= TIME_LAST )
		prefs.time_display = TIME_FIRST+1;
	    draw_status();
	    return TRUE;
	}
    }
	
    return FALSE;
}

void setup_time_display(GtkWidget *table)
{
    GtkWidget *handle1, *frame;
    GtkWidget *box;

    vol = gtk_adjustment_new (0.0, 0.0, 256.0, 0.1, 1.0, 1.0);
    volume = gtk_hscale_new(GTK_ADJUSTMENT(vol));
    gtk_tooltips_set_tip(tooltips, volume, _("Volume"), "");
    gtk_range_set_update_policy( GTK_RANGE(volume), GTK_UPDATE_CONTINUOUS );
    gtk_scale_set_draw_value( GTK_SCALE(volume), FALSE );
    gtk_signal_connect( GTK_OBJECT(vol), "value_changed",
			(GtkSignalFunc)volume_changed, NULL);
    gtk_signal_connect( GTK_OBJECT(volume), "button_press_event",
			(GtkSignalFunc)launch_gmix, NULL);

    status_area = gtk_drawing_area_new();
    gtk_signal_connect( GTK_OBJECT (status_area), "expose_event",
			(GtkSignalFunc)status_expose_event, NULL);
    gtk_signal_connect( GTK_OBJECT(status_area),"configure_event",
			(GtkSignalFunc)status_configure_event, NULL);
    gtk_signal_connect( GTK_OBJECT(status_area),"button_press_event",
			(GtkSignalFunc)status_click_event, NULL);
    gtk_widget_set_usize( status_area, 102, 60 );

    gtk_widget_set_events (status_area, gtk_widget_get_events(status_area)
			   | GDK_EXPOSURE_MASK
			   | GDK_LEAVE_NOTIFY_MASK
			   | GDK_BUTTON_PRESS_MASK
			   | GDK_POINTER_MOTION_MASK
			   | GDK_POINTER_MOTION_HINT_MASK);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_IN );
    gtk_container_add(GTK_CONTAINER(frame), status_area);
    status_table = gtk_vbox_new(FALSE, 2);

    box = gtk_hbox_new(FALSE, 2);
#ifdef TCD_CHANGER_ENABLED
    gtk_box_pack_start(GTK_BOX(box), make_changer_buttons(), FALSE, FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(box), volume, TRUE, TRUE, 0);

    gtk_box_pack_end(GTK_BOX(status_table), box, FALSE, FALSE, 0);

    if( prefs.handle )
    {
	handle1 = gtk_handle_box_new();
	gtk_container_add(GTK_CONTAINER(handle1), frame );
	gtk_box_pack_start( GTK_BOX(status_table), handle1, TRUE, TRUE, 2 );
    }
    else
	gtk_box_pack_start( GTK_BOX(status_table), frame, TRUE, TRUE, 0);
		                
    gtk_box_pack_start(GTK_BOX(main_box), status_table, FALSE, FALSE, 2);
    return;
}

void setup_fonts(void)
{
    if(tfont)
	gdk_font_unref(tfont);

    if(prefs.trackfont)
	tfont = gdk_font_load( prefs.trackfont );
    else
	tfont = NULL;

    if(sfont)
	gdk_font_unref(sfont);
    
    sfont = gdk_font_load("-misc-fixed-*-*-*-*-12-*-*-*-*-*-*-*");
    
    tfont_height = (tfont->ascent+tfont->descent)*3;
}

void init_window(void)
{
    setup_fonts();
    /* Window */
    window = gnome_app_new("gtcd", "TCD 2.0");
    gtk_window_set_title(GTK_WINDOW(window), PACKAGE" "VERSION" ");
    gtk_window_set_wmclass( GTK_WINDOW(window), "main_window","gtcd");
    gtk_window_set_policy(GTK_WINDOW(window), TRUE, FALSE, TRUE);

    gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		       GTK_SIGNAL_FUNC(quit_cb), NULL);

    gtk_container_border_width(GTK_CONTAINER(window), GNOME_PAD_SMALL);

    gtk_widget_realize(window);

    /* a gc */
    gc = gdk_gc_new(window->window);
    gdk_gc_copy(gc, window->style->white_gc);

    main_box = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);

    tooltips = gtk_tooltips_new();
    if( prefs.tooltip )
	gtk_tooltips_enable(tooltips);
    else
	gtk_tooltips_disable(tooltips);
}

void create_warning(char *message_text, char *type)
{
    gtk_widget_show(gnome_message_box_new(message_text, type,
					  GNOME_STOCK_BUTTON_OK, NULL));
}

void reload_info(int signal)
{
    tcd_post_init(&cd);
    tcd_readdiskinfo(&cd);
    if(cd.isplayable)
    {
	make_goto_menu();
	update_editor();
    }
    draw_status();
}


void exit_action(void)
{
    switch(prefs.exit_action)
    {
    case StopPlaying:
	tcd_stopcd(&cd);
	break;
    case OpenTray:
	tcd_ejectcd(&cd);
	break;
    case CloseTray:
	tcd_ejectcd(&cd);
	if(cd.isplayable)
	{
	    make_goto_menu();
	    update_editor();
	}
	break;
    case DoNothing:
    default:
	break;
    }
}	

void start_action(void)
{
    if(prefs.close_tray_on_start) {
	tcd_ejectcd(&cd);
	if(cd.isplayable)
	{
	    make_goto_menu();
	    update_editor();
	}
    }
    switch(prefs.start_action)
    {
    case StartPlaying:
	tcd_playtracks(&cd, cd.first_t, cd.last_t, prefs.only_use_trkind);
	if(cd.isplayable)
	{
	    make_goto_menu();
	    update_editor();
	}
	break;
    case StopPlaying:
	tcd_stopcd(&cd);
	break;
    case DoNothing:
    default:
	break;
    }
}	

void setup_keys()
{
    accel = gtk_accel_group_get_default();
}    

static char *CD_device = NULL;
poptContext ctx;

const struct poptOption gtcd_popt_options [] = {
	{ "device", '\0', POPT_ARG_STRING, &CD_device, 0,
	  N_("Filename of the CD device"),   N_("FILE") },
	{ NULL, '\0', 0, NULL, 0 }
};



int main (int argc, char *argv[])
{
    GtkWidget *table;
    GdkColor black;

    bindtextdomain(PACKAGE, GNOMELOCALEDIR);
    textdomain(PACKAGE);

    gnome_init_with_popt_table("gtcd", VERSION, argc, argv, gtcd_popt_options, 0, &ctx);

    cd.play_method = NORMAL;

    load_prefs(&prefs);
    if(CD_device)
	cd.cdpath = CD_device;
    else
    	cd.cdpath = prefs.cddev;
    		
    tcd_init_disc(&cd, (WarnFunc)create_warning);

    start_action();
    
    setup_keys();
    init_window();

    table = create_buttons();
    gtk_box_pack_start(GTK_BOX(main_box), table, FALSE, FALSE, 0);
    
    setup_time_display(table);
    setup_colors();
    led_init(window);
    make_goto_menu();

    setup_popup_menu(status_area, &cd);
    
    /* Initialize some timers */
    if(cd.isplayable) tcd_gettime(&cd);
    
    gtk_timeout_add(1000, (GtkFunction)slow_timer, NULL);
    gtk_timeout_add(500, (GtkFunction)fast_timer, NULL);
    titlelabel_f = TRUE;
	
    gnome_app_set_contents(GNOME_APP(window), main_box);
    adjust_status_size();
    gtk_widget_show_all(window);

    gdk_color_black( gtk_widget_get_colormap(status_area), &black);
    gdk_window_set_background( status_area->window, &black);
    
    signal(SIGUSR2, reload_info);
    
    gtk_main();
    save_prefs(&prefs);
    gnome_config_sync();
    gdk_gc_destroy(gc);
    exit_action();
    tcd_close_disc(&cd);

    return 0;
}
