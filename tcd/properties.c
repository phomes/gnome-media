#define PACKAGE "TCD"                                             
#define VERSION "2.0"                                             

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

#include <gnome.h>
#include "properties.h"

tcd_properties props;
GtkWidget *propbox;
extern GtkTooltips *tooltips;

/* from gtcd.c */
void setup_colors(void);
void setup_fonts(void);

struct font_str
{
	GtkWidget *fs;
	char **font;
};


void load_properties( tcd_properties *prop )
{
	prop->cddev 	= gnome_config_get_string("/gtcd/cdrom/device=/dev/cdrom");
	prop->cddb  	= gnome_config_get_string("/gtcd/cddb/server=cddb.cddb.com");
	prop->cddbport  = gnome_config_get_int(   "/gtcd/cddb/port=888");
	prop->handle    = gnome_config_get_bool(  "/gtcd/ui/handle=1");
	prop->tooltip   = gnome_config_get_bool(  "/gtcd/ui/tooltip=1");
	prop->trackfont  = gnome_config_get_string(
		"/gtcd/ui/trackfont=-misc-fixed-*-*-*-*-12-*-*-*-*-*-*-*" );
	prop->statusfont = gnome_config_get_string(
		"/gtcd/ui/statusfont=-adobe-helvetica-*-r-*-*-14-*-*-*-*-*-*-*" );
	prop->trackcolor = gnome_config_get_string( "/gtcd/ui/trackcolor=#FF0000" );
	prop->statuscolor = gnome_config_get_string( "/gtcd/ui/statuscolor=#FFFF00" );
}

void save_properties( tcd_properties *prop )
{
	gnome_config_set_string("/gtcd/cdrom/device", prop->cddev);
	gnome_config_set_string("/gtcd/cddb/server", prop->cddb);
	gnome_config_set_int(   "/gtcd/cddb/port", prop->cddbport );
	gnome_config_set_bool(  "/gtcd/ui/handle", prop->handle);
	gnome_config_set_bool(  "/gtcd/ui/tooltip", prop->tooltip);
	gnome_config_set_string("/gtcd/ui/trackfont", prop->trackfont );
	gnome_config_set_string("/gtcd/ui/statusfont",prop->statusfont );
	gnome_config_set_string("/gtcd/ui/trackcolor", prop->trackcolor );
	gnome_config_set_string("/gtcd/ui/statuscolor",prop->statuscolor );
	gnome_config_sync();
}

void changed_cb( GtkWidget *widget, void *data )
{
	gnome_property_box_changed(GNOME_PROPERTY_BOX(propbox));
}

void port_changed_cb( GtkWidget *widget, GtkWidget *spin )
{
	props.cddbport = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
	gnome_property_box_changed(GNOME_PROPERTY_BOX(propbox));
}
	
GtkWidget *create_cddb_frame( GtkWidget *box )
{
	GtkWidget *cddb_r_box, *cddb_l_box;
	GtkWidget *cddb_frame, *cddb_box;
	GtkWidget *cddb_i, *cddb_l;
	GtkWidget *port_i, *port_l;
	GtkObject *adj;

	adj = gtk_adjustment_new( props.cddbport, 1, 9999, 1, 10, 10 );
	port_i  = gtk_spin_button_new( GTK_ADJUSTMENT(adj), 1,0 );
	gtk_signal_connect( GTK_OBJECT(adj),"value_changed",
        	GTK_SIGNAL_FUNC(port_changed_cb),port_i );
        gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON(port_i),
        	GTK_UPDATE_ALWAYS );
        
        cddb_i  = gtk_entry_new();

	cddb_r_box = gtk_vbox_new( FALSE, 2 );
	cddb_l_box = gtk_vbox_new( FALSE, 2 );
	cddb_box   = gtk_hbox_new( FALSE, 2 );
	
	gtk_entry_set_text( GTK_ENTRY(cddb_i), props.cddb );

	props.cddb = gtk_entry_get_text(GTK_ENTRY(cddb_i));

	cddb_l = gtk_label_new("Server");
	port_l = gtk_label_new("Port");
	gtk_label_set_justify( GTK_LABEL(cddb_l), GTK_JUSTIFY_RIGHT );
	gtk_label_set_justify( GTK_LABEL(port_l), GTK_JUSTIFY_RIGHT );

	cddb_frame = gtk_frame_new("CDDB Access");
	gtk_container_border_width( GTK_CONTAINER(cddb_frame), 5 );

	gtk_signal_connect( GTK_OBJECT(cddb_i), "changed",
		GTK_SIGNAL_FUNC(changed_cb), NULL );
	gtk_signal_connect( GTK_OBJECT(port_i), "changed",
		GTK_SIGNAL_FUNC(changed_cb), NULL );

	gtk_box_pack_start( GTK_BOX(cddb_l_box), cddb_l,TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(cddb_r_box), cddb_i,TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(cddb_l_box), port_l,TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(cddb_r_box), port_i,TRUE, TRUE, 0 );

	gtk_box_pack_start( GTK_BOX(cddb_box), cddb_l_box, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(cddb_box), cddb_r_box, TRUE, TRUE, 0 );

	gtk_container_add( GTK_CONTAINER(cddb_frame), cddb_box );

	gtk_widget_show_all(cddb_frame);

	return cddb_frame;
}

GtkWidget *create_cdrom_frame( GtkWidget *box )
{
	GtkWidget *cdrom_r_box;
	GtkWidget *cdrom_l_box;
	GtkWidget *cdrom_frame;
	GtkWidget *cdrom_box;
	GtkWidget *cddev_i, *cddev_l;

	cdrom_r_box= gtk_vbox_new( FALSE, 2 );
	cdrom_l_box= gtk_vbox_new( FALSE, 2 );
	cdrom_box  = gtk_hbox_new( FALSE, 2 );

	cddev_l = gtk_label_new("Device");
	gtk_label_set_justify( GTK_LABEL(cddev_l), GTK_JUSTIFY_RIGHT );
	cddev_i = gtk_entry_new();

	gtk_entry_set_text( GTK_ENTRY(cddev_i), props.cddev );
	props.cddev = gtk_entry_get_text(GTK_ENTRY(cddev_i));

	cdrom_frame = gtk_frame_new("CDROM Drive");
	gtk_container_border_width( GTK_CONTAINER(cdrom_frame), 5 );

	gtk_signal_connect( GTK_OBJECT(cddev_i), "changed",
		GTK_SIGNAL_FUNC(changed_cb), NULL );

	gtk_box_pack_start( GTK_BOX(cdrom_l_box), cddev_l, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(cdrom_r_box), cddev_i, TRUE, TRUE, 0 );

	gtk_box_pack_start( GTK_BOX(cdrom_box), cdrom_l_box, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(cdrom_box), cdrom_r_box, TRUE, TRUE, 0 );
	
	gtk_container_add( GTK_CONTAINER(cdrom_frame), cdrom_box );

	gtk_widget_show_all(cdrom_frame);
	
	return cdrom_frame;
}

void check_changed_cb( GtkWidget *widget, gboolean *data )
{
	if( *data )
		*data = FALSE;
	else	
		*data = TRUE;
	gnome_property_box_changed(GNOME_PROPERTY_BOX(propbox));
}

void color_changed_cb( GnomeColorSelector *widget, gchar **color )
{
	char *tmp;
	int r,g,b;

	tmp = malloc(24);
	if( !tmp )
	{
		g_warning( "Can't allocate memory for color\n" );
		return;
	}
	gnome_color_selector_get_color_int( 
		widget, &r, &g, &b, 255 );
	
	sprintf( tmp, "#%02x%02x%02x", r, g, b );
	*color = tmp;
// 	free(color);
	gnome_property_box_changed(GNOME_PROPERTY_BOX(propbox));
}

void font_changed_cb( GtkWidget *widget, struct font_str *str )
{
	*str->font = g_strdup(gnome_font_selector_get_selected(GNOME_FONT_SELECTOR(str->fs)));
	gnome_property_box_changed(GNOME_PROPERTY_BOX(propbox));
//	free(str);
}

void font_cb( GtkWidget *widget, gchar **font )
{
	GtkWidget *fs;
	struct font_str *str;

	if( !(str = malloc(sizeof(struct font_str))))
		g_warning("cannot allocate %d bytes..things will die quickly\n", sizeof(struct font_str));

	fs = gnome_font_selector_new();
	str->fs = fs;
	str->font = font;

	gtk_widget_show(fs);
        gtk_signal_connect( GTK_OBJECT(GNOME_FONT_SELECTOR(fs)->ok_button), "clicked",
		GTK_SIGNAL_FUNC(font_changed_cb), str );
}	

GtkWidget *create_status_frame( GtkWidget *box )
{
	GtkWidget *trackbutton_c, *statusbutton_c;
	GtkWidget *trackbutton_f, *statusbutton_f;
	GtkWidget *track_l, *status_l;
	GnomeColorSelector *track_gcs, *status_gcs;
	int tr,tg,tb, rr,rg,rb;
	
	GtkWidget *frame, *status_table;

	sscanf( props.trackcolor, "#%02x%02x%02x", &tr,&tg,&tb );
	sscanf( props.statuscolor, "#%02x%02x%02x", &rr,&rg,&rb );

	status_table = gtk_table_new( 4, 2, FALSE );

	track_gcs  = gnome_color_selector_new( (SetColorFunc)color_changed_cb, 
					      &props.trackcolor );
	status_gcs = gnome_color_selector_new( (SetColorFunc)color_changed_cb, 
					      &props.statuscolor );
	trackbutton_f  = gtk_button_new_with_label( "Time Font" );
	statusbutton_f = gtk_button_new_with_label( "Track Font" );
	
	gtk_signal_connect( GTK_OBJECT(trackbutton_f), "clicked",
		GTK_SIGNAL_FUNC(font_cb), &props.trackfont );
	gtk_signal_connect( GTK_OBJECT(statusbutton_f), "clicked",
		GTK_SIGNAL_FUNC(font_cb), &props.statusfont );

	gnome_color_selector_set_color_int( track_gcs, tr, tg, tb, 255 );
	gnome_color_selector_set_color_int( status_gcs, rr, rg, rb, 255 );

	trackbutton_c = gnome_color_selector_get_button( track_gcs );
	statusbutton_c = gnome_color_selector_get_button( status_gcs );

	track_l  = gtk_label_new("Track Display");
	status_l = gtk_label_new("Status Display");
	
	frame = gtk_frame_new("Fonts & Colors");

	gtk_table_attach_defaults( GTK_TABLE(status_table),
	                                track_l, 0,2,0,1 );
	gtk_table_attach_defaults( GTK_TABLE(status_table),
	                                status_l, 0,2,1,2 );

	gtk_table_attach_defaults( GTK_TABLE(status_table),
	                                trackbutton_c, 2,3,0,1 );
	gtk_table_attach_defaults( GTK_TABLE(status_table),
	                                trackbutton_f, 3,4,0,1 );
	gtk_table_attach_defaults( GTK_TABLE(status_table),
	                                statusbutton_c, 2,3,1,2 );
	gtk_table_attach_defaults( GTK_TABLE(status_table),
	                                statusbutton_f, 3,4,1,2 );

	gtk_container_add( GTK_CONTAINER(frame), status_table );

	gtk_widget_show_all(frame);
	
	return frame;
}

GtkWidget *create_ui_frame( GtkWidget *box )
{
	GtkWidget *ui_r_box;
	GtkWidget *ui_l_box;
	GtkWidget *ui_frame;
	GtkWidget *ui_box;
	
	GtkWidget *handle_i;
	GtkWidget *tooltips_i;

	ui_r_box= gtk_vbox_new( FALSE, 2 );
	ui_l_box= gtk_vbox_new( FALSE, 2 );
	ui_box  = gtk_hbox_new( TRUE, 2 );

	handle_i = gtk_check_button_new_with_label("Show Handles (Restart of TCD required)");
	tooltips_i = gtk_check_button_new_with_label("Show Tooltips");

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(handle_i), props.handle );
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tooltips_i), props.tooltip );

	ui_frame = gtk_frame_new("General");
	gtk_container_border_width( GTK_CONTAINER(ui_frame), 5 );

	gtk_signal_connect( GTK_OBJECT(handle_i), "clicked",
		GTK_SIGNAL_FUNC(check_changed_cb), &props.handle );
	gtk_signal_connect( GTK_OBJECT(tooltips_i), "clicked",
		GTK_SIGNAL_FUNC(check_changed_cb), &props.tooltip );

	gtk_box_pack_start( GTK_BOX(ui_l_box), handle_i, FALSE, FALSE, 4 );
	gtk_box_pack_start( GTK_BOX(ui_l_box), tooltips_i, FALSE, FALSE, 4 );

	gtk_box_pack_start( GTK_BOX(ui_box), ui_l_box, FALSE, FALSE, 0 );
	gtk_box_pack_start( GTK_BOX(ui_box), ui_r_box, FALSE, FALSE, 0 );
	
	gtk_container_add( GTK_CONTAINER(ui_frame), ui_box );

	gtk_widget_show_all(ui_frame);
	
	return ui_frame;
}

GtkWidget *create_page2()
{
	GtkWidget *box;
	GtkWidget *ui_frame, *status_frame;
	GtkWidget *sep;

	box = gtk_vbox_new( FALSE,4 );	

	ui_frame = create_ui_frame(box);
	status_frame = create_status_frame(box);
	
	gtk_box_pack_start( GTK_BOX(box), ui_frame,TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(box), status_frame,TRUE, TRUE, 0 );

	return box;
}

GtkWidget *create_page1()
{
	GtkWidget *box;
	GtkWidget *cdrom_frame, *cddb_frame;
	GtkWidget *sep;

	box = gtk_vbox_new( FALSE,4 );	

	cddb_frame = create_cddb_frame( box );
	cdrom_frame= create_cdrom_frame( box );

	gtk_box_pack_start( GTK_BOX(box), cddb_frame,TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(box), cdrom_frame,TRUE, TRUE, 0 );

	return box;
}

void apply_cb( GtkWidget *widget, void *data )
{	
/* Do stuff here if needed */
	if( props.tooltip )
		gtk_tooltips_enable(tooltips);
	else
		gtk_tooltips_disable(tooltips);
	setup_colors();		
	setup_fonts();
	save_properties(&props);
}

void properties_cb( GtkWidget *widget, void *data )
{
	GtkWidget *page1, *page2, *label;

	if( propbox )
		return;

	load_properties(&props);
		
	propbox = gnome_property_box_new();
	gtk_window_set_title( GTK_WINDOW(&GNOME_PROPERTY_BOX(propbox)->dialog.window), "TCD Settings" );

	page1	= create_page1();
	label   = gtk_label_new("General");
	gtk_widget_show(page1);
	gnome_property_box_append_page( GNOME_PROPERTY_BOX(propbox),
		page1, label );

	page2 	= create_page2();
        label   = gtk_label_new("Interface");
	gtk_widget_show(page2);
	gnome_property_box_append_page( GNOME_PROPERTY_BOX(propbox),
		page2, label );

	gtk_signal_connect( GTK_OBJECT(propbox), 
		"apply", GTK_SIGNAL_FUNC(apply_cb), NULL );

	gtk_widget_show_all(propbox);
	return;
}
