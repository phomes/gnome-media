/* This file is part of TCD 2.0.
   
   Copyright (C) 1997-98 Tim P. Gerla <timg@rrv.net>
   
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
   timg@rrv.net
*/

#include <config.h>
#include <gnome.h>
#include <string.h>
#include <ctype.h>

#include "gtcd_public.h"
#include "keybindings.h"
#include "prefs.h"
#include "cddb_props.h"

static GtkWidget *pref_window;
static tcd_prefs *oldprefs;

/* private functions */
static void color_set_cb(GnomeColorPicker *cp, guint pr, guint pg, guint pb);
static void start_toggle_cb(GtkWidget *widget, gpointer data);
static void check_changed_cb(GtkWidget *widget, gboolean *data);
static GtkWidget *create_start_frame(void);
static void exit_toggle_cb(GtkWidget *widget, gpointer data);
static GtkWidget *create_exit_frame(void);
static void dev_entry_changed_cb(GtkWidget *widget, gpointer data);
static void font_ok_clicked_cb(GtkWidget *widget, GtkWidget *fs);
static void font_cancel_clicked_cb(GtkWidget *widget, GtkWidget *fs);
static void font_button_cb(GtkWidget *widget, gpointer *data);
static GtkWidget *create_general_frame(void);
static GtkWidget *create_page(void);
static void apply_cb(GtkWidget *widget, void *data);
static GtkWidget *key_page(void);
static int entry_changed(GtkWidget *widget, GdkEvent *ev, KeyBinding *kb);

/* public functions */
void load_prefs(tcd_prefs *prop);
void save_prefs(tcd_prefs *prop);
void changed_cb(GtkWidget *widget, void *data);
void preferences(GtkWidget *widget, void *data);

void load_prefs(tcd_prefs *prop)
{
	char *s;
#if defined(sun) || defined(__sun__)
#if defined(SVR4) || defined(__svr4__)
	prop->cddev=gnome_config_get_string    ("/gtcd/cdrom/device=/vol/dev/aliases/cdrom0");
#else
	prop->cddev=gnome_config_get_string    ("/gtcd/cdrom/device=/dev/rcd0");
#endif
#else
	prop->cddev=gnome_config_get_string    ("/gtcd/cdrom/device=/dev/cdrom");
#endif
	prop->handle=gnome_config_get_bool     ("/gtcd/ui/handle=false");
	prop->tooltip=gnome_config_get_bool    ("/gtcd/ui/tooltip=true");
	prop->mixer_cmd=gnome_config_get_string    ("/gtcd/ui/mixer=gmix");

	s = g_strconcat("/gtcd/ui/trackfont=", _("-adobe-helvetica-medium-r-normal-*-*-120-*-*-p-*-*-*"), NULL);
	prop->trackfont=gnome_config_get_string(s);
	g_free(s);

	prop->trackcolor_r=gnome_config_get_int("/gtcd/ui/trackcolor_r=255" );
	prop->trackcolor_g=gnome_config_get_int("/gtcd/ui/trackcolor_g=0" );
	prop->trackcolor_b=gnome_config_get_int("/gtcd/ui/trackcolor_b=0" );
    
	prop->exit_action=gnome_config_get_int         ("/gtcd/general/exit_action=0");
	prop->start_action=gnome_config_get_int        ("/gtcd/general/start_action=0");
	prop->close_tray_on_start=gnome_config_get_bool("/gtcd/general/close_tray_on_start=false");

	prop->quit.key =    gnome_config_get_int("/gtcd/keybindings/quit=81");
	prop->play.key =    gnome_config_get_int("/gtcd/keybindings/play=80");
	prop->stop.key =    gnome_config_get_int("/gtcd/keybindings/stop=83");
	prop->tracked.key = gnome_config_get_int("/gtcd/keybindings/tracked=84");
	prop->mixer.key =   gnome_config_get_int("/gtcd/keybindings/mixer=109");
	prop->eject.key =   gnome_config_get_int("/gtcd/keybindings/eject=69");
	prop->back.key =    gnome_config_get_int("/gtcd/keybindings/back=45");
	prop->forward.key = gnome_config_get_int("/gtcd/keybindings/forward=43");

	prop->only_use_trkind = gnome_config_get_bool("/gtcd/general/only_use_trkind=0");

/* cddb stuff, used by cddbslave. */
	prop->cddb_server = gnome_config_get_string("/cddbslave/server/address=freedb.freedb.org");
	prop->cddb_port   = gnome_config_get_int("/cddbslave/server/port=888");
	prop->cddb_http   = gnome_config_get_bool("/cddbslave/server/use_http=false");
	prop->cddb_httpproxy_need_auth = gnome_config_get_bool("/cddbslave/server/need_http_proxy_auth=false");
	prop->cddb_httpproxy_auth_name = gnome_config_private_get_string("/cddbslave/server/http_proxy_auth_name=");
	prop->cddb_httpproxy_auth_passwd = gnome_config_private_get_string("/cddbslave/server/http_proxy_auth_passwd=");
	prop->cddb_httpproxy = gnome_config_get_string("/cddbslave/server/http_proxy=");
}

void save_prefs(tcd_prefs *prop)
{
	gnome_config_set_string("/gtcd/cdrom/device", prop->cddev);
	gnome_config_set_string("/gtcd/ui/mixer", prop->mixer_cmd);
	gnome_config_set_bool  ("/gtcd/ui/handle", prop->handle);
	gnome_config_set_bool  ("/gtcd/ui/tooltip", prop->tooltip);
	gnome_config_set_string("/gtcd/ui/trackfont", prop->trackfont);

	gnome_config_set_int("/gtcd/ui/trackcolor_r", prop->trackcolor_r);
	gnome_config_set_int("/gtcd/ui/trackcolor_g", prop->trackcolor_g);
	gnome_config_set_int("/gtcd/ui/trackcolor_b", prop->trackcolor_b);

	gnome_config_set_int ("/gtcd/general/exit_action", prop->exit_action);
	gnome_config_set_int ("/gtcd/general/start_action", prop->start_action);
	gnome_config_set_bool("/gtcd/general/close_tray_on_start", prop->close_tray_on_start);

	gnome_config_set_int("/gtcd/keybindings/quit", prop->quit.key);
	gnome_config_set_int("/gtcd/keybindings/play", prop->play.key);
	gnome_config_set_int("/gtcd/keybindings/stop", prop->stop.key);
	gnome_config_set_int("/gtcd/keybindings/tracked", prop->tracked.key);
	gnome_config_set_int("/gtcd/keybindings/mixer", prop->mixer.key);
	gnome_config_set_int("/gtcd/keybindings/eject", prop->eject.key);
	gnome_config_set_int("/gtcd/keybindings/back", prop->back.key);
	gnome_config_set_int("/gtcd/keybindings/forward", prop->forward.key);

	gnome_config_set_string("/cddbslave/server/address", prop->cddb_server);
	gnome_config_set_int("/cddbslave/server/port=8880", prop->cddb_port);
	gnome_config_set_bool("/cddbslave/server/use_http", prop->cddb_http);
	gnome_config_set_string("/cddbslave/server/http_proxy", prop->cddb_httpproxy);
	gnome_config_set_bool("/cddbslave/server/need_http_proxy_auth", prop->cddb_httpproxy_need_auth);
	gnome_config_private_set_string("/cddbslave/server/http_proxy_auth_name", prop->cddb_httpproxy_auth_name);
	gnome_config_private_set_string("/cddbslave/server/http_proxy_auth_passwd", prop->cddb_httpproxy_auth_passwd);

	gnome_config_set_bool("/gtcd/general/only_use_trkind", prop->only_use_trkind);
        
	gnome_config_sync();
}
        
void changed_cb(GtkWidget *widget, void *data)
{
	gnome_property_box_changed(GNOME_PROPERTY_BOX(pref_window));
}

static void color_set_cb(GnomeColorPicker *cp, guint pr, guint pg, guint pb)
{
	prefs->trackcolor_r = pr / 256;
	prefs->trackcolor_g = pg / 256;
	prefs->trackcolor_b = pb / 256; 

	changed_cb(NULL, NULL);
}

static void start_toggle_cb(GtkWidget *widget, gpointer data)
{
	prefs->start_action = GPOINTER_TO_INT(data);
	changed_cb(NULL, NULL);
}

static void check_changed_cb(GtkWidget *widget, gboolean *data)
{
	if( *data )
		*data = FALSE;
	else        
		*data = TRUE;
	changed_cb(NULL, NULL);
}

static GtkWidget *create_start_frame()
{
	GtkWidget *start_playing;
	GtkWidget *stop_playing;
	GtkWidget *close_tray;
	GtkWidget *do_nothing;
	GtkWidget *vbox;
    
	vbox = gtk_vbox_new(TRUE, 0);

	/* do nothing */
	do_nothing = gtk_radio_button_new_with_label(NULL, _("Do nothing"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(do_nothing), (prefs->start_action==DoNothing)?1:0);

	/* start playing */
	start_playing = gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(do_nothing)),
		_("Start playing"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(start_playing), (prefs->start_action==StartPlaying)?1:0);
    
	/* stop playing */
	stop_playing = gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(do_nothing)),
		_("Stop playing"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stop_playing), (prefs->start_action==StopPlaying)?1:0);
	
	/* close tray */
	close_tray = gtk_check_button_new_with_label(_("Close tray"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(close_tray), prefs->close_tray_on_start);

	gtk_signal_connect(GTK_OBJECT(close_tray), "clicked",
			   GTK_SIGNAL_FUNC(check_changed_cb), &prefs->close_tray_on_start);
   
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

static void exit_toggle_cb(GtkWidget *widget, gpointer data)
{
	prefs->exit_action = GPOINTER_TO_INT(data);
	changed_cb(NULL, NULL);
}

static GtkWidget *create_exit_frame()
{
	GtkWidget *stop_playing;
	GtkWidget *open_tray;
	GtkWidget *close_tray;
	GtkWidget *do_nothing;
	GtkWidget *vbox;
    
	vbox = gtk_vbox_new(TRUE, 0);

	/* do nothing */
	do_nothing = gtk_radio_button_new_with_label(NULL, _("Do nothing"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(do_nothing), (prefs->exit_action==DoNothing)?1:0);
    
	/* stop playing */
	stop_playing = gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(do_nothing)),
		_("Stop playing"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stop_playing), (prefs->exit_action==StopPlaying)?1:0);

	/* open tray */
	open_tray = gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(do_nothing)),
		_("Open tray"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(open_tray), (prefs->exit_action==OpenTray)?1:0);
        
	/* close tray */
	close_tray = gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(do_nothing)),
		_("Close tray"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(close_tray), (prefs->exit_action==CloseTray)?1:0);

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

static void dev_entry_changed_cb(GtkWidget *widget, gpointer data)
{
	prefs->cddev = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
	changed_cb(NULL, NULL);
}

static void font_ok_clicked_cb(GtkWidget *widget, GtkWidget *fs)
{
        prefs->trackfont = g_strdup(gtk_font_selection_dialog_get_font_name(
                GTK_FONT_SELECTION_DIALOG(fs)));
        gtk_widget_destroy(fs);
	changed_cb(NULL, NULL);
}

static void font_cancel_clicked_cb(GtkWidget *widget, GtkWidget *fs)
{
        gtk_widget_destroy(fs);
}       
        
static void font_button_cb(GtkWidget *widget, gpointer *data)
{
        GtkWidget *fs;
        
        fs = gtk_font_selection_dialog_new("Font");
        gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(fs), prefs->trackfont);

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
	GtkWidget *handles, *tooltips, *trkind;
    
	left_box = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	right_box = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
    
	gtk_box_pack_start_defaults(GTK_BOX(hbox), left_box);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), right_box);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);
    
	/* device entry */
	label = gtk_label_new(_("CDROM device"));
	dev_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(dev_entry), prefs->cddev);
	gtk_signal_connect(GTK_OBJECT(dev_entry), "changed",
			   GTK_SIGNAL_FUNC(dev_entry_changed_cb), NULL);
	gtk_box_pack_start_defaults(GTK_BOX(left_box), label);
	gtk_box_pack_start_defaults(GTK_BOX(right_box), dev_entry);
    
	/* Color picker */
	label = gtk_label_new(_("Track/title color"));
	cp = gnome_color_picker_new();
	gnome_color_picker_set_i8(GNOME_COLOR_PICKER(cp), 
				  prefs->trackcolor_r, 
				  prefs->trackcolor_g, 
				  prefs->trackcolor_b, 0);
	gtk_signal_connect(GTK_OBJECT(cp), "color_set",
			   GTK_SIGNAL_FUNC(color_set_cb), NULL);
	gtk_box_pack_start_defaults(GTK_BOX(left_box), label);
	gtk_box_pack_start(GTK_BOX(right_box), cp, TRUE, FALSE, 0);
    
	/* font picker */
	label = gtk_label_new(_("Track/title font"));
	fs = gtk_button_new_with_label(_("Change font"));
	gtk_signal_connect(GTK_OBJECT(fs), "clicked",
			   GTK_SIGNAL_FUNC(font_button_cb), NULL);
	gtk_box_pack_start_defaults(GTK_BOX(left_box), label);
	gtk_box_pack_start(GTK_BOX(right_box), fs, TRUE, FALSE, 0);
    
	/* show handles */
	handles = gtk_check_button_new_with_label(_("Show handles (restart required)"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(handles), prefs->handle);
	gtk_signal_connect(GTK_OBJECT(handles), "clicked",
			   GTK_SIGNAL_FUNC(check_changed_cb), &prefs->handle);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), handles);
    
	/* show tooltips */
	tooltips = gtk_check_button_new_with_label(_("Show tooltips"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tooltips), prefs->tooltip);
	gtk_signal_connect(GTK_OBJECT(tooltips), "clicked",
			   GTK_SIGNAL_FUNC(check_changed_cb), &prefs->tooltip);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), tooltips);

	/* use alternate play ioctl */
	trkind = gtk_check_button_new_with_label(_("Use alternate method to play CD"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(trkind), prefs->only_use_trkind);
	gtk_signal_connect(GTK_OBJECT(trkind), "clicked",
			   GTK_SIGNAL_FUNC(check_changed_cb), &prefs->only_use_trkind);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), trkind);
    
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget *create_page()
{
	GtkWidget *table;
	GtkWidget *start_frame;
	GtkWidget *exit_frame;
	GtkWidget *general_frame;
    
	table = gtk_table_new(2, 2, FALSE);
    
	/* start frame */
	start_frame = gtk_frame_new(_("On startup"));
	gtk_container_border_width(GTK_CONTAINER(start_frame), GNOME_PAD_SMALL);
	gtk_container_add(GTK_CONTAINER(start_frame), create_start_frame());
	gtk_table_attach_defaults(GTK_TABLE(table), start_frame, 0, 1, 0, 1);
    
	/* exit frame */
	exit_frame = gtk_frame_new(_("On exit"));
	gtk_container_border_width(GTK_CONTAINER(exit_frame), GNOME_PAD_SMALL);
	gtk_container_add(GTK_CONTAINER(exit_frame), create_exit_frame());
	gtk_table_attach_defaults(GTK_TABLE(table), exit_frame, 0, 1, 1, 2);
    
	/* general frame */
	general_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(general_frame), GTK_SHADOW_NONE);
	gtk_container_border_width(GTK_CONTAINER(general_frame), GNOME_PAD_SMALL);
	gtk_container_add(GTK_CONTAINER(general_frame), create_general_frame());
	gtk_table_attach_defaults(GTK_TABLE(table), general_frame, 1, 2, 0, 2);
    
	gtk_widget_show_all(table);
	return table;
}

static void fill_list(KeyBinding *kb, GtkWidget *clist)
{
	char *tmp[2];

	tmp[0] = g_malloc(64);	/* key */
	tmp[1] = g_malloc(256);	/* desc */

	g_snprintf(tmp[0], 63, "%c", toupper(kb->key->key));
	g_snprintf(tmp[1], 255, "%s", kb->desc);
    
	gtk_clist_append(GTK_CLIST(clist), tmp);
	gtk_object_set_data(GTK_OBJECT(clist), tmp[0], kb);

	g_free(tmp[0]);
	g_free(tmp[1]);
}	

static int entry_changed(GtkWidget *widget, GdkEvent *ev, KeyBinding *kb)
{
	char tmp[64];
	if(ev->type != GDK_KEY_PRESS)
		return 0;
	g_snprintf(tmp, 63, "%c", toupper(kb->key->key));
	gtk_object_remove_data(GTK_OBJECT(kb->data), tmp);
	kb->key->key = *((GdkEventKey*)ev)->string;

	g_snprintf(tmp, 63, "%c", toupper(kb->key->key));
	gtk_object_set_data(GTK_OBJECT(kb->data), tmp, kb);
	gtk_clist_set_text(GTK_CLIST(kb->data), kb->data2, 0, tmp);
	changed_cb(NULL, NULL);

	return 1;
}

static void select_row_cb(GtkCList *clist,
			  gint row,
			  gint column,
			  GdkEventButton *event,
			  gpointer data)
{
	static int entry_cb=0, alt_cb=0, shift_cb=0, ctrl_cb=0;
	GtkWidget *entry, *alt_check, *shift_check, *ctrl_check;
	KeyBinding *kb;
	gchar *text;

	gtk_clist_get_text(clist, row, 0, &text);
	kb = gtk_object_get_data(GTK_OBJECT(clist), text);
	entry = gtk_object_get_data(GTK_OBJECT(clist), "entry");
	if(!kb) /* FIXME this is broken...? */
		return;
	ctrl_check = gtk_object_get_data(GTK_OBJECT(clist), "ctrl_check");
	alt_check = gtk_object_get_data(GTK_OBJECT(clist), "alt_check");
	shift_check = gtk_object_get_data(GTK_OBJECT(clist), "shift_check");

	if(entry_cb > 0) gtk_signal_disconnect(GTK_OBJECT(entry), entry_cb);
	if(ctrl_cb > 0) gtk_signal_disconnect(GTK_OBJECT(ctrl_check), ctrl_cb);
	if(alt_cb > 0) gtk_signal_disconnect(GTK_OBJECT(alt_check), alt_cb);
	if(shift_cb > 0) gtk_signal_disconnect(GTK_OBJECT(shift_check), shift_cb);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctrl_check), kb->key->ctrl);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(alt_check), kb->key->alt);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(shift_check), kb->key->shift);
    
	kb->data = (GtkWidget*)clist;
	kb->data2 = row;
	entry_cb = gtk_signal_connect(GTK_OBJECT(entry), "event",
				      GTK_SIGNAL_FUNC(entry_changed), kb);
	ctrl_cb = gtk_signal_connect(GTK_OBJECT(ctrl_check), "clicked",
				     GTK_SIGNAL_FUNC(check_changed_cb), &kb->key->ctrl);
	alt_cb = gtk_signal_connect(GTK_OBJECT(alt_check), "clicked",
				    GTK_SIGNAL_FUNC(check_changed_cb), &kb->key->alt);
	shift_cb = gtk_signal_connect(GTK_OBJECT(shift_check), "clicked",
				      GTK_SIGNAL_FUNC(check_changed_cb), &kb->key->shift);
}

static GtkWidget *key_page(void)
{
	GtkWidget *clist, *frame, *box, *scrolled, *mod_box;
	GtkWidget *alt_check, *ctrl_check, *shift_check;
	GtkWidget *label, *entry;

	/* Scrolled Window */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	/* List */
	clist = gtk_clist_new(2);
	gtk_clist_set_column_width(GTK_CLIST(clist), 0, 30);
	gtk_clist_set_selection_mode(GTK_CLIST(clist),
				     GTK_SELECTION_BROWSE);
	gtk_clist_set_column_title(GTK_CLIST(clist), 0, _("Key"));
	gtk_clist_set_column_title(GTK_CLIST(clist), 1, _("Action"));
	gtk_clist_column_titles_show(GTK_CLIST(clist));
	gtk_clist_column_titles_passive(GTK_CLIST(clist));
	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);
	gtk_clist_set_shadow_type(GTK_CLIST(clist), GTK_SHADOW_NONE);
        
	g_list_foreach(keys, (GFunc)fill_list, clist);

	gtk_container_add(GTK_CONTAINER(scrolled), clist);
	gtk_signal_connect(GTK_OBJECT(clist), "select_row",
			   GTK_SIGNAL_FUNC(select_row_cb), NULL);
	/* Box */
	box = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);

	/* key box */
	mod_box = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);

	entry = gtk_entry_new_with_max_length(1);
	label = gtk_label_new(_("Click here to change (restart required):"));

	ctrl_check = gtk_check_button_new_with_label("Control");
	alt_check = gtk_check_button_new_with_label("Alt");
	shift_check = gtk_check_button_new_with_label("Shift");

	gtk_box_pack_start(GTK_BOX(mod_box), label, FALSE, FALSE, GNOME_PAD);
	gtk_box_pack_start(GTK_BOX(mod_box), entry, TRUE, TRUE, GNOME_PAD);
#if 0				/* Not yet implemented */
	gtk_box_pack_start_defaults(GTK_BOX(mod_box), ctrl_check);
	gtk_box_pack_start_defaults(GTK_BOX(mod_box), alt_check);
	gtk_box_pack_start_defaults(GTK_BOX(mod_box), shift_check);
#endif

	gtk_box_pack_start(GTK_BOX(box), mod_box, FALSE, FALSE, 0);

	gtk_object_set_data(GTK_OBJECT(clist), "entry", entry);
	gtk_object_set_data(GTK_OBJECT(clist), "ctrl_check", ctrl_check);
	gtk_object_set_data(GTK_OBJECT(clist), "alt_check", alt_check);
	gtk_object_set_data(GTK_OBJECT(clist), "shift_check", shift_check);

	/* Frame */
	frame = gtk_frame_new(_("Keybindings"));
	gtk_container_add(GTK_CONTAINER(frame), box);
	gtk_container_border_width(GTK_CONTAINER(frame), GNOME_PAD_SMALL);

	gtk_widget_show_all(frame);
	return frame;
}    

static void apply_cb(GtkWidget *widget, void *data)
{       
/* Do stuff here if needed */

	if(prefs->tooltip)
		gtk_tooltips_enable(tooltips);
	else
		gtk_tooltips_disable(tooltips);
	setup_colors();
	setup_fonts();
	adjust_status_size();
	save_prefs(prefs);
	load_prefs(oldprefs);
}

static void cancel_cb(void)
{
	prefs = oldprefs;
	pref_window = NULL;
}

void preferences(GtkWidget *widget, void *data)
{
	GtkWidget *label;

	if(pref_window)
	{
		gtk_widget_destroy(pref_window);
		cancel_cb();
		return;
	}
	/* store current settings */
	oldprefs = g_new0(tcd_prefs, 1);
	load_prefs(oldprefs);
	
	pref_window = gnome_property_box_new();
	gtk_widget_realize(pref_window);
	
	label = gtk_label_new(_("Preferences"));
	gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pref_window)->notebook),
				 create_page(), label);
    
	label = gtk_label_new(_("Keybindings"));
	gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pref_window)->notebook),
				 key_page(), label);

	label = gtk_label_new(_("CDDB Settings"));
	gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pref_window)->notebook),
				 create_cddb_page(), label);
	
	gtk_signal_connect(GTK_OBJECT(pref_window), "apply",
			   GTK_SIGNAL_FUNC(apply_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(pref_window), "help",
			   GTK_SIGNAL_FUNC(help), NULL);
	gtk_signal_connect(GTK_OBJECT(pref_window), "destroy",
			   GTK_SIGNAL_FUNC(cancel_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(pref_window), "delete_event",
			   GTK_SIGNAL_FUNC(cancel_cb), NULL);

	gtk_widget_show_all(pref_window);	
}    
