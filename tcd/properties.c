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

void load_properties( tcd_properties *prop )
{
	prop->cddev 	= gnome_config_get_string("/gtcd/cdrom/device=/dev/cdrom");
	prop->cddb  	= gnome_config_get_string("/gtcd/cddb/server=cddb.cddb.com");
	prop->cddbport  = gnome_config_get_int(   "/gtcd/cddb/port=888");
	prop->iconset  	= gnome_config_get_string("/gtcd/ui/iconset=BlackBevel");
	prop->handle    = gnome_config_get_bool(  "/gtcd/ui/handle=1");
}

void save_properties( tcd_properties *prop )
{
	gnome_config_set_string("/gtcd/cdrom/device", prop->cddev);
	gnome_config_set_string("/gtcd/cddb/server", prop->cddb);
	gnome_config_set_string("/gtcd/ui/iconset", prop->iconset);
	gnome_config_set_int(   "/gtcd/cddb/port", prop->cddbport);
	gnome_config_set_bool(  "/gtcd/ui/handle", prop->handle);
	gnome_config_sync();
}

void changed_cb( GtkWidget *widget, void *data )
{
	gnome_property_box_changed(GNOME_PROPERTY_BOX(propbox));
}

void port_changed_cb( GtkWidget *widget, GtkWidget *spin )
{
	int t;
	
	t = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
	props.cddbport = t;
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

	props.cddb     = gtk_entry_get_text(GTK_ENTRY(cddb_i));

	cddb_l     = gtk_label_new("Default CDDB Server");
	port_l     = gtk_label_new("Default CDDB Port");

	cddb_frame = gtk_frame_new("CDDB Access");
	gtk_container_border_width( GTK_CONTAINER(cddb_frame), 5 );

	gtk_widget_show(cddb_i);
	gtk_widget_show(cddb_l);
	gtk_widget_show(port_i);
	gtk_widget_show(port_l);

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

	gtk_widget_show(cddb_r_box);
	gtk_widget_show(cddb_l_box);
	gtk_widget_show(cddb_frame);
	gtk_widget_show(cddb_box);

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

	cddev_l = gtk_label_new("CDROM Drive Device");
	cddev_i = gtk_entry_new();

	gtk_entry_set_text( GTK_ENTRY(cddev_i), props.cddev );
	props.cddev = gtk_entry_get_text(GTK_ENTRY(cddev_i));

	cdrom_frame = gtk_frame_new("CDROM Drive");
	gtk_container_border_width( GTK_CONTAINER(cdrom_frame), 5 );

	gtk_widget_show(cddev_i);
	gtk_widget_show(cddev_l);

	gtk_signal_connect( GTK_OBJECT(cddev_i), "changed",
		GTK_SIGNAL_FUNC(changed_cb), NULL );

	gtk_box_pack_start( GTK_BOX(cdrom_l_box), cddev_l, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(cdrom_r_box), cddev_i, TRUE, TRUE, 0 );

	gtk_box_pack_start( GTK_BOX(cdrom_box), cdrom_l_box, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(cdrom_box), cdrom_r_box, TRUE, TRUE, 0 );
	
	gtk_container_add( GTK_CONTAINER(cdrom_frame), cdrom_box );

	gtk_widget_show(cdrom_r_box);
	gtk_widget_show(cdrom_l_box);
	gtk_widget_show(cdrom_frame);
	gtk_widget_show(cdrom_box);
	
	return cdrom_frame;
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

	handle_i = gtk_check_button_new_with_label("Show Handles");
	tooltips_i = gtk_check_button_new_with_label("Show Tooltips");

	ui_frame = gtk_frame_new("");
	gtk_container_border_width( GTK_CONTAINER(ui_frame), 5 );

	gtk_widget_show(handle_i);
	gtk_widget_show(tooltips_i);

/*	gtk_signal_connect( GTK_OBJECT(cddev_i), "changed",
		GTK_SIGNAL_FUNC(changed_cb), NULL );
*/
	gtk_box_pack_start( GTK_BOX(ui_l_box), handle_i, FALSE, FALSE, 4 );
	gtk_box_pack_start( GTK_BOX(ui_l_box), tooltips_i, FALSE, FALSE, 4 );

	gtk_box_pack_start( GTK_BOX(ui_box), ui_l_box, FALSE, FALSE, 0 );
	gtk_box_pack_start( GTK_BOX(ui_box), ui_r_box, FALSE, FALSE, 0 );
	
	gtk_container_add( GTK_CONTAINER(ui_frame), ui_box );

	gtk_widget_show(ui_r_box);
	gtk_widget_show(ui_l_box);
	gtk_widget_show(ui_frame);
	gtk_widget_show(ui_box);
	
	return ui_frame;
}

GtkWidget *create_page2()
{
	GtkWidget *box;
	GtkWidget *ui_frame;
	GtkWidget *sep;

	box = gtk_vbox_new( FALSE,4 );	

	ui_frame = create_ui_frame( box );

	gtk_box_pack_start( GTK_BOX(box), ui_frame,TRUE, TRUE, 0 );

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
	save_properties(&props);
}

void properties_cb( GtkWidget *widget, void *data )
{
	GtkWidget *page1, *page2, *label;

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

	gtk_widget_show(label);
	gtk_widget_show(page1);
	gtk_widget_show(propbox);
	return;
}
