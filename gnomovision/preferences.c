#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <gdk/gdktypes.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gnome.h>
#include <gtk/gtkeventbox.h>

#include <linux/types.h>
#include <linux/videodev.h>

#include "channels.h"

#include "gnomovision.h"


GtkWidget *dialog;
GtkWidget *preferences;
GtkWidget *pref_colour;
GtkWidget *pref_country;
GtkWidget *pref_channel;

GtkWidget *make_label(char *x)
{
	GtkWidget *w=gtk_label_new(x);
	gtk_widget_show(w);
	return w;
}

static GtkWidget *colour_widget=NULL;
static GtkWidget *tuner_widget=NULL;

static gint colour_settings(GtkAdjustment *adj_data, void *ptr)
{
	int n=(int)ptr;
	
	if(n<5)
		adj_data->value+=0.5;
	
	switch(n)
	{
		case 0:
			vpic.brightness=adj_data->value;
			break;
		case 1:
			vpic.contrast=adj_data->value;
			break;
		case 2:
			vpic.whiteness=adj_data->value;
			break;
		case 3:
			vpic.colour=adj_data->value;
			break;
		case 4:
			vpic.hue=adj_data->value;
			break;
		case 5:
			vfrequency=vbase_freq+(adj_data->value-32768);
			set_tv_frequency(vfrequency);
			return TRUE;
		case 6:
			vchannel=adj_data->value;
			printf("Setting channel to %d\n", vchannel);
			vfrequency=vbase_freq=channel_compute(adj_data->value);
			set_tv_frequency(vfrequency);
			return TRUE;
	}
	set_tv_picture(&vpic);
	return TRUE;
}


static void slider_create(char *name, GtkWidget *vbox, int id, float value)
{
	GtkWidget *scale;
	GtkObject *slider;
	
	scale=gtk_label_new(name);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 2);
	gtk_widget_show(scale);

	if(id<5)
	{	
		slider = gtk_adjustment_new(value, 0.0, 65535.0, 1.0, 1.0, 0.0);
	}
	else if(id==6)
	{
		slider = gtk_adjustment_new(value, 0.0, 147.0, 1.0, 1.0, 0.0);
	}
	else
	{
		slider = gtk_adjustment_new(value, -256.0, 255.0, 1.0, 1.0, 0.0);
	}
	scale = gtk_hscale_new(GTK_ADJUSTMENT(slider));
	gtk_widget_set_usize(scale,200,0);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);
	
	gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
	gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
	gtk_scale_set_digits(GTK_SCALE(scale), 0);
	
	gtk_signal_connect(slider, "value_changed",
		(GtkSignalFunc)colour_settings, (void *)id); 

	gtk_widget_show(scale);
}

static void done_colour_settings(void)
{
	gtk_widget_hide(dialog);
}

static void done_tuner_settings(void)
{
	gtk_widget_hide(tuner_widget);
}


static void colour_setting(GtkWidget *colour_widget)
{
	GtkWidget *vbox, *button;
	
	vbox = gtk_vbox_new(FALSE,5);
	gtk_container_border_width(GTK_CONTAINER(vbox),3);
	gtk_container_add(GTK_CONTAINER(colour_widget), vbox);
	gtk_widget_show(vbox);
	
	slider_create("Brightness", vbox, 0, (float)vpic.brightness);
	slider_create("Contrast", vbox, 1, (float)vpic.contrast);
	
	if(vcap.type&VID_TYPE_MONOCHROME)
	{
		slider_create("Whiteness", vbox, 2, (float)vpic.whiteness);
	}
	else
	{
		slider_create("Colour", vbox, 3, (float)vpic.colour);
		slider_create("Hue", vbox, 4, (float)vpic.hue);
	}

	button = gtk_button_new_with_label("Dismiss");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)done_colour_settings, NULL);
	gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);
	gtk_widget_grab_default(button);
	
	gtk_widget_show(colour_widget);
}

void frequency_setting(void)
{
	GtkWidget *main_vbox, *vbox, *hbox, *button;
	
	main_vbox = gtk_vbox_new(FALSE, 5);
	
	vbox = gtk_vbox_new(FALSE,5);
	gtk_container_border_width(GTK_CONTAINER(vbox),3);
	gtk_box_pack_start(GTK_BOX(main_vbox), vbox, TRUE, TRUE, 0);
	gtk_widget_show(vbox);
	
	slider_create("Channel", vbox, 6, (float)vchannel);
	slider_create("Fine Tune", vbox, 5, 0);
	
	button = gtk_button_new_with_label("Dismiss");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)done_tuner_settings, NULL);
	gtk_box_pack_start(GTK_BOX(main_vbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);
	gtk_widget_grab_default(button);
	
	gtk_widget_show(main_vbox);
	gtk_container_add(GTK_CONTAINER(pref_channel), main_vbox);
}


void create_country_selector(void)
{
	pref_country = gtk_frame_new("Country");
	gtk_widget_show(pref_country);
}

void create_channel_selector(void)
{
	pref_channel = gtk_frame_new("Channel");
	frequency_setting();
	gtk_widget_show(pref_channel);
}

void create_colour_selector(void)
{
	pref_colour = gtk_frame_new("Colour");
	colour_setting(pref_colour);
	gtk_widget_show(pref_colour);
}
	

static void make_preferences_box(void)
{
	if(dialog==NULL)
	{
		dialog = gtk_window_new(GTK_WINDOW_DIALOG);
	
		preferences = gtk_notebook_new();
	
		create_country_selector();
		create_channel_selector();
		create_colour_selector();
		gtk_notebook_append_page(GTK_NOTEBOOK(preferences), pref_country, make_label("Country"));
		gtk_notebook_append_page(GTK_NOTEBOOK(preferences), pref_channel, make_label("Channel"));
		gtk_notebook_append_page(GTK_NOTEBOOK(preferences), pref_colour, make_label("Colour"));
	
		gtk_widget_show(preferences);
	
		gtk_container_add(GTK_CONTAINER(dialog), preferences);
	}
	gtk_widget_show(dialog);
}


void preferences_page(void)
{
	make_preferences_box();
}

