/* This file is part of TCD 2.0.
   
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
#include <gnome.h>
#include <string.h>

#include "gtcd_public.h"
#include "prefs.h"

static GtkWidget *pref_window=NULL;

void load_prefs(tcd_prefs *prop);
void save_prefs(tcd_prefs *prop);
void changed_cb(GtkWidget *widget, void *data);
void color_set_cb(GnomeColorPicker *cp, int pr, int pg, int pb);
void start_toggle_cb(GtkWidget *widget, gpointer data);
void check_changed_cb(GtkWidget *widget, gboolean *data);
GtkWidget *create_start_frame(void);
void exit_toggle_cb(GtkWidget *widget, gpointer data);
GtkWidget *create_exit_frame(void);
void dev_entry_changed_cb(GtkWidget *widget, gpointer data);
void font_ok_clicked_cb(GtkWidget *widget, GtkWidget *fs);
void font_cancel_clicked_cb(GtkWidget *widget, GtkWidget *fs);
void font_button_cb(GtkWidget *widget, gpointer *data);
GtkWidget *create_general_frame(void);
GtkWidget *create_page(void);
void apply_cb(GtkWidget *widget, void *data);
void preferences(GtkWidget *widget, void *data);

void load_prefs(tcd_prefs *prop)
{
    prop->cddev=gnome_config_get_string    ("/gtcd/cdrom/device=/dev/cdrom");
    prop->handle=gnome_config_get_bool     ("/gtcd/ui/handle=false");
    prop->tooltip=gnome_config_get_bool    ("/gtcd/ui/tooltip=true");
    prop->time_display=gnome_config_get_int("/gtcd/ui/time_display=0");
    prop->trackfont=gnome_config_get_string("/gtcd/ui/trackfont=-misc-fixed-*-*-*-*-12-*-*-*-*-*-*-*" );

    prop->trackcolor_r=gnome_config_get_int("/gtcd/ui/trackcolor_r=255" );
    prop->trackcolor_g=gnome_config_get_int("/gtcd/ui/trackcolor_g=0" );
    prop->trackcolor_b=gnome_config_get_int("/gtcd/ui/trackcolor_b=0" );
    
    prop->exit_action=gnome_config_get_int         ("/gtcd/general/exit_action=0");
    prop->start_action=gnome_config_get_int        ("/gtcd/general/start_action=0");
    prop->close_tray_on_start=gnome_config_get_bool("/gtcd/general/close_tray_on_start=false");

}

void save_prefs(tcd_prefs *prop)
{
    gnome_config_set_string("/gtcd/cdrom/device", prop->cddev);
    gnome_config_set_bool  ("/gtcd/ui/handle", prop->handle);
    gnome_config_set_bool  ("/gtcd/ui/tooltip", prop->tooltip);
    gnome_config_set_int   ("/gtcd/ui/time_display", prop->time_display);
    gnome_config_set_string("/gtcd/ui/trackfont", prop->trackfont);

    gnome_config_set_int("/gtcd/ui/trackcolor_r", prop->trackcolor_r);
    gnome_config_set_int("/gtcd/ui/trackcolor_g", prop->trackcolor_g);
    gnome_config_set_int("/gtcd/ui/trackcolor_b", prop->trackcolor_b);

    gnome_config_set_int ("/gtcd/general/exit_action", prop->exit_action);
    gnome_config_set_int ("/gtcd/general/start_action", prop->start_action);
    gnome_config_set_bool("/gtcd/general/close_tray_on_start", prop->close_tray_on_start);
  
    gnome_config_sync();
}

void changed_cb(GtkWidget *widget, void *data)
{
    gnome_property_box_changed(GNOME_PROPERTY_BOX(pref_window));
}

void color_set_cb(GnomeColorPicker *cp, int pr, int pg, int pb)
{
    gnome_color_picker_get_i8(cp, 
			      &prefs.trackcolor_r, 
			      &prefs.trackcolor_g, 
			      &prefs.trackcolor_b,
			      NULL);
    changed_cb(NULL, NULL);
}

void start_toggle_cb(GtkWidget *widget, gpointer data)
{
    prefs.start_action = GPOINTER_TO_INT(data);
    changed_cb(NULL, NULL);
}

void check_changed_cb(GtkWidget *widget, gboolean *data)
{
    if( *data )
        *data = FALSE;
    else        
        *data = TRUE;
    changed_cb(NULL, NULL);
}

GtkWidget *create_start_frame()
{
    GtkWidget *start_playing;
    GtkWidget *stop_playing;
    GtkWidget *close_tray;
    GtkWidget *do_nothing;
    GtkWidget *vbox;
    
    vbox = gtk_vbox_new(TRUE, 0);

    /* do nothing */
    do_nothing = gtk_radio_button_new_with_label(NULL, _("Do Nothing"));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(do_nothing), (prefs.start_action==DoNothing)?1:0);

    /* start playing */
    start_playing = gtk_radio_button_new_with_label(
	gtk_radio_button_group(GTK_RADIO_BUTTON(do_nothing)),
	_("Start Playing"));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(start_playing), (prefs.start_action==StartPlaying)?1:0);
    
    /* stop playing */
    stop_playing = gtk_radio_button_new_with_label(
	gtk_radio_button_group(GTK_RADIO_BUTTON(do_nothing)),
	_("Stop Playing"));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(stop_playing), (prefs.start_action==StopPlaying)?1:0);
	
    /* close tray */
    close_tray = gtk_check_button_new_with_label(_("Close Tray"));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(close_tray), prefs.close_tray_on_start);

    gtk_signal_connect(GTK_OBJECT(close_tray), "clicked",
		       GTK_SIGNAL_FUNC(check_changed_cb), &prefs.close_tray_on_start);
   
    gtk_signal_connect(GTK_OBJECT(do_nothing), "clicked",
		       GTK_SIGNAL_FUNC(start_toggle_cb), GINT_TO_POINTER(DoNothing));
    gtk_signal_connect(GTK_OBJECT(start_playing), "clicked",
		       GTK_SIGNAL_FUNC(start_toggle_cb), GINT_TO_POINTER(StartPlaying));
    gtk_signal_connect(GTK_OBJECT(stop_playing), "clicked",
		       GTK_SIGNAL_FUNC(start_toggle_cb), GINT_TO_POINTER(StopPlaying));

    gtk_box_pack_start_defaults(GTK_BOX(vbox), do_nothing);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), start_playing);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), stop_playing);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), close_tray);

    gtk_widget_show_all(vbox);
    return vbox;
}

void exit_toggle_cb(GtkWidget *widget, gpointer data)
{
    prefs.exit_action = GPOINTER_TO_INT(data);
    changed_cb(NULL, NULL);
}

GtkWidget *create_exit_frame()
{
    GtkWidget *stop_playing;
    GtkWidget *open_tray;
    GtkWidget *close_tray;
    GtkWidget *do_nothing;
    GtkWidget *vbox;
    
    vbox = gtk_vbox_new(TRUE, 0);

    /* do nothing */
    do_nothing = gtk_radio_button_new_with_label(NULL, _("Do Nothing"));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(do_nothing), (prefs.exit_action==DoNothing)?1:0);
    
    /* stop playing */
    stop_playing = gtk_radio_button_new_with_label(
	gtk_radio_button_group(GTK_RADIO_BUTTON(do_nothing)),
	_("Stop Playing"));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(stop_playing), (prefs.exit_action==StopPlaying)?1:0);

    /* open tray */
    open_tray = gtk_radio_button_new_with_label(
	gtk_radio_button_group(GTK_RADIO_BUTTON(do_nothing)),
	_("Open Tray"));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(open_tray), (prefs.exit_action==OpenTray)?1:0);
        
    /* close tray */
    close_tray = gtk_radio_button_new_with_label(
	gtk_radio_button_group(GTK_RADIO_BUTTON(do_nothing)),
	_("Close Tray"));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(close_tray), (prefs.exit_action==CloseTray)?1:0);

    gtk_signal_connect(GTK_OBJECT(do_nothing), "clicked",
		       GTK_SIGNAL_FUNC(exit_toggle_cb), GINT_TO_POINTER(DoNothing));
    gtk_signal_connect(GTK_OBJECT(stop_playing), "clicked",
		       GTK_SIGNAL_FUNC(exit_toggle_cb), GINT_TO_POINTER(StopPlaying));
    gtk_signal_connect(GTK_OBJECT(open_tray), "clicked",
		       GTK_SIGNAL_FUNC(exit_toggle_cb), GINT_TO_POINTER(OpenTray));
    gtk_signal_connect(GTK_OBJECT(close_tray), "clicked",
		       GTK_SIGNAL_FUNC(exit_toggle_cb), GINT_TO_POINTER(CloseTray));

    gtk_box_pack_start_defaults(GTK_BOX(vbox), do_nothing);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), stop_playing);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), open_tray);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), close_tray);
    
    gtk_widget_show_all(vbox);
    return vbox;
}	

void dev_entry_changed_cb(GtkWidget *widget, gpointer data)
{
    prefs.cddev = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
    changed_cb(NULL, NULL);
}

void font_ok_clicked_cb(GtkWidget *widget, GtkWidget *fs)
{
        prefs.trackfont = g_strdup(gtk_font_selection_dialog_get_font_name(
                GTK_FONT_SELECTION_DIALOG(fs)));
        gtk_widget_destroy(fs);
	changed_cb(NULL, NULL);
}

void font_cancel_clicked_cb(GtkWidget *widget, GtkWidget *fs)
{
        gtk_widget_destroy(fs);
}       
        
void font_button_cb(GtkWidget *widget, gpointer *data)
{
        GtkWidget *fs;
        
        fs = gtk_font_selection_dialog_new("Font");
        gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(fs), prefs.trackfont);

        gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fs)->ok_button), "clicked",
                GTK_SIGNAL_FUNC(font_ok_clicked_cb), fs);
        gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fs)->cancel_button), "clicked",
                GTK_SIGNAL_FUNC(font_cancel_clicked_cb), fs);

        gtk_widget_show(fs);
}

GtkWidget *create_general_frame()
{
    GtkWidget *label;
    GtkWidget *dev_entry;
    GtkWidget *cp, *fs;
    GtkWidget *left_box, *right_box, *hbox, *vbox;
    GtkWidget *handles, *tooltips;
    
    left_box = gtk_vbox_new(FALSE, 2);
    right_box = gtk_vbox_new(FALSE, 2);
    hbox = gtk_hbox_new(FALSE, 2);
    vbox = gtk_vbox_new(FALSE, 2);
    
    gtk_box_pack_start_defaults(GTK_BOX(hbox), left_box);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), right_box);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);
    
    /* device entry */
    label = gtk_label_new(_("CDROM Device"));
    dev_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(dev_entry), prefs.cddev);
    gtk_signal_connect(GTK_OBJECT(dev_entry), "changed",
		       GTK_SIGNAL_FUNC(dev_entry_changed_cb), NULL);
    gtk_box_pack_start_defaults(GTK_BOX(left_box), label);
    gtk_box_pack_start_defaults(GTK_BOX(right_box), dev_entry);
    
    /* Color picker */
    label = gtk_label_new(_("Track/Title Color"));
    cp = gnome_color_picker_new();
    gnome_color_picker_set_i8(GNOME_COLOR_PICKER(cp), 
			      prefs.trackcolor_r, 
			      prefs.trackcolor_g, 
			      prefs.trackcolor_b, 0);
    gtk_signal_connect(GTK_OBJECT(cp), "color_set",
		       GTK_SIGNAL_FUNC(color_set_cb), NULL);
    gtk_box_pack_start_defaults(GTK_BOX(left_box), label);
    gtk_box_pack_start(GTK_BOX(right_box), cp, TRUE, FALSE, 0);
    
    /* font picker */
    label = gtk_label_new(_("Track/Title Font"));
    fs = gtk_button_new_with_label(_("Change Font"));
    gtk_signal_connect(GTK_OBJECT(fs), "clicked",
		       GTK_SIGNAL_FUNC(font_button_cb), NULL);
    gtk_box_pack_start_defaults(GTK_BOX(left_box), label);
    gtk_box_pack_start(GTK_BOX(right_box), fs, TRUE, FALSE, 0);
    
    /* show handles */
    handles = gtk_check_button_new_with_label(_("Show Handles (Restart of TCD required)"));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(handles), prefs.handle);
    gtk_signal_connect(GTK_OBJECT(handles), "clicked",
		       GTK_SIGNAL_FUNC(check_changed_cb), &prefs.handle);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), handles);
    
    /* show tooltips */
    tooltips = gtk_check_button_new_with_label(_("Show Tooltips"));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tooltips), prefs.tooltip);
    gtk_signal_connect(GTK_OBJECT(tooltips), "clicked",
		       GTK_SIGNAL_FUNC(check_changed_cb), &prefs.tooltip);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), tooltips);
    
    gtk_widget_show_all(vbox);
    return vbox;
}

GtkWidget *create_page()
{
    GtkWidget *table;
    GtkWidget *start_frame;
    GtkWidget *exit_frame;
    GtkWidget *general_frame;
    
    table = gtk_table_new(2, 2, FALSE);
    
    /* start frame */
    start_frame = gtk_frame_new(_("On Startup"));
    gtk_container_add(GTK_CONTAINER(start_frame), create_start_frame());
    gtk_table_attach_defaults(GTK_TABLE(table), start_frame, 0, 1, 0, 1);
    
    /* exit frame */
    exit_frame = gtk_frame_new(_("On Exit"));
    gtk_container_add(GTK_CONTAINER(exit_frame), create_exit_frame());
    gtk_table_attach_defaults(GTK_TABLE(table), exit_frame, 0, 1, 1, 2);
    
    /* general frame */
    general_frame = gtk_frame_new(_("General"));
    gtk_container_add(GTK_CONTAINER(general_frame), create_general_frame());
    gtk_table_attach_defaults(GTK_TABLE(table), general_frame, 1, 2, 0, 2);
    
    gtk_widget_show_all(table);
    return table;
}

void apply_cb( GtkWidget *widget, void *data )
{       
/* Do stuff here if needed */
    if( prefs.tooltip )
        gtk_tooltips_enable(tooltips);
    else
        gtk_tooltips_disable(tooltips);
    setup_colors();
    setup_fonts();
    save_prefs(&prefs);
}

void preferences(GtkWidget *widget, void *data)
{
    GtkWidget *label;
    
    pref_window = gnome_property_box_new();
    gtk_widget_realize(pref_window);

    label = gtk_label_new(_("Preferences"));
    gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pref_window)->notebook),
			     create_page(), label);
    
    gtk_signal_connect(GTK_OBJECT(pref_window),
		       "apply", GTK_SIGNAL_FUNC(apply_cb), NULL);

    gtk_widget_show_all(pref_window);	
}    
