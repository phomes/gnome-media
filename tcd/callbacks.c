/* Various callbacks for gtcd. */

#include <config.h>
#include <gnome.h>
#include <sys/types.h>
#include <linux/cdrom.h>

#include "linux-cdrom.h"
#include "gtcd_public.h"
#include "callbacks.h"

void play_cb(GtkWidget *widget, gpointer data)
{
    if(cd.sc.cdsc_audiostatus==CDROM_AUDIO_PAUSED)
	tcd_pausecd(&cd);
    else
	tcd_playtracks(&cd, cd.first_t, cd.last_t);
    cd.repeat_track = cd.cur_t;

    draw_status();
    return;
}

void pause_cb(GtkWidget *widget, gpointer data)
{
    tcd_pausecd(&cd);
    
    draw_status();
    return;
}

void stop_cb(GtkWidget *widget, gpointer data)
{
    tcd_stopcd(&cd);

    draw_status();
    return;
}

void eject_cb(GtkWidget *widget, gpointer data)
{
    tcd_ejectcd(&cd);
    cd.play_method = NORMAL;
    cd.repeat_track = -1;
    /* SDH: Make sure play/pause state change is noticed */
    cd.sc.cdsc_audiostatus = -1;

    draw_status();
    return;
}

void about_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *about;
    gchar *authors[] = {
	"Tim P. Gerla",
	NULL
    };  
    
    about = gnome_about_new ( 
	_("TCD - The Gnome CD Player"), 
	"2.2 (CVS)",
	"(C) 1997-98 Tim P. Gerla",
	(const gchar**)authors,
	_("Gnome CD player application with CDDB support."
	  " Please see the \'Thanks\' file included with the"
	  " distribution for more credits."),
	NULL);
    gtk_widget_show(about);
    
    return;
}

gint goto_track_cb( GtkWidget *widget, gpointer data )
{
    tcd_playtracks(&cd, GPOINTER_TO_INT(data), cd.last_t);
    cd.repeat_track = GPOINTER_TO_INT(data);
    return 1;
}

void quit_cb(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

void changer_cb(GtkWidget *widget, gpointer data)
{
    tcd_close_disc(&cd);
    tcd_change_disc(&cd, GPOINTER_TO_INT(data));
    tcd_init_disc(&cd,create_warning);
    cd.play_method = NORMAL;

    return;
}

