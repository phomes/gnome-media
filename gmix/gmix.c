/*
 * GMIX 3.0
 *
 * A total rewrite of gmix 2.5...
 *
 * This version includes most features of gnome-hello, except DND and SM. That
 * will follow.
 *
 * Supported sound-drivers:
 * - OSS (only lite checked)
 *
 * TODO:
 * - ALSA support
 * - /dev/audio (if the admins install GTK and GNOME on our Sparcs...)
 * - other sound systems
 * - you name it...
 * - Animated ramping of mixer sliders to preset values
 * - Load Settings button, several for presets ?
 * - more configuration
 *
 * Copyright (C) 1998 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
 * Config dialog added by Matt Martin <Matt.Martin@ieee.org>, Sept 1999
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#ifdef HAVE_LINUX_SOUNDCARD_H 
#include <linux/soundcard.h>
#else 
#include <machine/soundcard.h>
#endif

#include <gnome.h>

static void about_cb (GtkWidget *widget, void *data);
static void options_cb (GtkWidget *widget, void *data);
static void quit_cb (GtkWidget *widget, void *data);
static void config_cb (GtkWidget *widget, void *data);
void fill_in_device_guis(GtkWidget *notebook);

static void get_device_config(void);
static void put_device_config(void);
static void get_gui_config(void);
static void put_gui_config(void);
void scan_devices(void);
void free_devices(void);
void init_devices(void);
void open_dialog(void);

/*
 * Gnome info:
 */
GtkWidget *app;
GtkWidget *configwin;
GtkWidget  *slidernotebook;

typedef struct {
  gboolean set_mixer_on_start;
  gboolean hide_menu;
  gboolean use_icons;
  gboolean use_labels;
} mixerprefs;

mixerprefs prefs={FALSE,FALSE,TRUE,TRUE};

/* Menus */
static GnomeUIInfo help_menu[] = {
    GNOMEUIINFO_ITEM_STOCK(N_("_About"), NULL, about_cb,
                           GNOME_STOCK_MENU_ABOUT),
    GNOMEUIINFO_END
};
 
static GnomeUIInfo program_menu[] = {
    GNOMEUIINFO_ITEM_STOCK(N_("_Properties"),NULL,config_cb,GNOME_STOCK_MENU_PROP),
    GNOMEUIINFO_MENU_EXIT_ITEM(quit_cb, NULL),
    GNOMEUIINFO_END
};      

static GnomeUIInfo main_menu[] = {
        GNOMEUIINFO_SUBTREE(N_("_Program"), &program_menu),
        GNOMEUIINFO_SUBTREE(N_("_Help"), &help_menu),
        GNOMEUIINFO_END
};
/* End of menus */ 

/* 
 * All, that is known about a mixer-device
 */
typedef struct device_info {
	int fd;
	mixer_info info;
	int recsrc;	/* current recording-source(s) */
	int devmask;	/* devices supported by this driver */
	int recmask;	/* devices, that can be a recording-source */
	int stereodevs;	/* devices with stereo abilities */
	int caps;	/* features supported by the mixer */
	int volume_left[32], volume_right[32]; /* volume, mono only left */

	int mute_bitmask; /* which channels are muted */
	int record_bitmask; /* which channels have recording enabled */
	int lock_bitmask; /* which channels have the L/R sliders linked together */
	int enabled_bitmask; /* which channels should be visible in the GUI ? */
	GList *channels;
} device_info;

/*
 * All channels, that are visible in the mixer-window
 */
typedef struct channel_info {
	/* general info */
	device_info *device;	/* refferrence back to the device */
	int channel;		/* which channel of that device am I ? */
	/* GUI info */
	char *title; /* or char *titel ? */
	char *pixmap;
	/* here are the widgets... */
	GtkObject *left, *right;
	GtkWidget *lock, *rec, *mute;

	int passive; /* avoid recursive calls to event handler */
} channel_info;

GList *devices;

int mode=0, mode_norestore=0, mode_initonly=0, mode_nosave=0, num_mixers, mode_restore=0;
#define M_NORESTORE	1
#define M_INITONLY 	2
#define M_NOSAVE	4

static const struct poptOption options[] = {
  {"norestore", 'r', POPT_ARG_NONE, &mode_norestore, 0, N_("don't restore mixer-settings from configuration"), NULL},
  {"restore", 'R', POPT_ARG_NONE, &mode_restore, 0, N_("restore mixer-settings from configuration"), NULL},
  {"initonly", 'i', POPT_ARG_NONE, &mode_initonly, 0, N_("initialise the mixer(s) from stored configuration and exit"), NULL},
  {"nosave", 's', POPT_ARG_NONE, &mode_nosave, 0, N_("don't save (modified) mixer-settings into configuration"), NULL},
  {NULL, '\0', 0, NULL, 0}
};

/*
 * Names for the mixer-channels. device_labels are the initial labels for the
 * sliders, device_names are used in the setup-file.
 *
 * i18n note: These names are defined in the soundcard.h file, but they are
 * only used in the initial configuration.
 * Don't translate the "device_names", because they're used for configuration.
 */
const char *device_labels[] = SOUND_DEVICE_LABELS;
const char *device_names[]  = SOUND_DEVICE_NAMES;
#ifndef GNOME_STOCK_PIXMAP_BLANK
#define GNOME_STOCK_PIXMAP_BLANK NULL
#endif
const char *device_pixmap[] = {
   GNOME_STOCK_PIXMAP_VOLUME,               /* Master Volume */
   GNOME_STOCK_PIXMAP_BLANK,                /* Bass */
   GNOME_STOCK_PIXMAP_BLANK,                /* Treble */
   GNOME_STOCK_PIXMAP_BLANK,                /* Synth */
   GNOME_STOCK_PIXMAP_BLANK,                /* PCM */
   GNOME_STOCK_PIXMAP_VOLUME,               /* Speaker */
   GNOME_STOCK_PIXMAP_LINE_IN,              /* Line In */
   GNOME_STOCK_PIXMAP_MIC,                  /* Microphone */
   GNOME_STOCK_PIXMAP_CDROM,                /* CD-Rom */
   GNOME_STOCK_PIXMAP_BLANK,                /* Recording monitor ? */
   GNOME_STOCK_PIXMAP_BLANK,                /* ALT PCM */
   GNOME_STOCK_PIXMAP_BLANK,                /* Rec Level? */
   GNOME_STOCK_PIXMAP_BLANK,                /* In Gain */
   GNOME_STOCK_PIXMAP_BLANK,                /* Out Gain */
   GNOME_STOCK_PIXMAP_LINE_IN,              /* Aux 1 */
   GNOME_STOCK_PIXMAP_LINE_IN,              /* Aux 2 */
   GNOME_STOCK_PIXMAP_LINE_IN,              /* Line */
   GNOME_STOCK_PIXMAP_BLANK,                /* Digital 1 ? */
   GNOME_STOCK_PIXMAP_BLANK,                /* Digital 2 ? */
   GNOME_STOCK_PIXMAP_BLANK,                /* Digital 3 ? */
   GNOME_STOCK_PIXMAP_BLANK,                /* Phone in */
   GNOME_STOCK_PIXMAP_BLANK,                /* Phone Out */
   GNOME_STOCK_PIXMAP_BLANK,                /* Video */
   GNOME_STOCK_PIXMAP_BLANK,                /* Radio */
   GNOME_STOCK_PIXMAP_BLANK,                /* Monitor (usually mic) vol */
   GNOME_STOCK_PIXMAP_BLANK,                /* 3d Depth/space param */
   GNOME_STOCK_PIXMAP_BLANK,                /* 3d center param */
   GNOME_STOCK_PIXMAP_BLANK                 /* Midi */
};

/*
 * GMIX version, for version-checking the config-file
 */
#define GMIX_VERSION 0x030000

void lock_cb (GtkWidget *widget, channel_info *data);
void rec_cb (GtkWidget *widget, channel_info *data);
void mute_cb (GtkWidget *widget, channel_info *data);
void adj_left_cb (GtkAdjustment *widget, channel_info *data);
void adj_right_cb (GtkAdjustment *widget, channel_info *data);

/* Prototypes to make gcc happy. TPG */
device_info *open_device(int num);
GList *make_channels(device_info *device);
void scan_devices(void);
void free_one_device(gpointer a, gpointer b);
void free_devices(void);
void init_one_device(gpointer a, gpointer b);
void init_devices(void);
void get_one_device_config(gpointer a, gpointer b);
void put_one_device_config(gpointer a, gpointer b);
GtkWidget *make_slider_mixer(channel_info *ci);

static void error_close_cb(void)
{
        g_error("No mixers found.");
}

static void apply_cb(GtkWidget *widget, void *data)
{       
        /* This is a sloppy way to re-draw the mixers */
        GList *d;

        put_gui_config();

	gtk_widget_hide(slidernotebook);
	/* Assumes that the number of devices is static... */
	for (d=devices; d; d=d->next) 
	  gtk_notebook_remove_page(GTK_NOTEBOOK(slidernotebook),0);
	gtk_widget_show(slidernotebook);

	fill_in_device_guis(slidernotebook);
	
}

static void cancel_cb(GtkWidget *widget, void *data)
{       
        configwin=NULL;
}

static void bool_changed_cb(GtkWidget *widget, gboolean *data)
{
	if( *data )
		*data = FALSE;
	else        
		*data = TRUE;
	gnome_property_box_changed(GNOME_PROPERTY_BOX(configwin));
}

GtkWidget *make_config_win()
{
}

GtkWidget *optpage()
{
  GtkWidget *start_frame, *gui_frame;
  GtkWidget *ubervbox;
  GtkWidget *vbox,*init_start,*menu_hide,*temp;

  ubervbox = gtk_vbox_new(TRUE, 0);

  start_frame = gtk_frame_new(_("On startup"));
  gtk_container_border_width(GTK_CONTAINER(start_frame), GNOME_PAD_SMALL);
    
	vbox = gtk_vbox_new(TRUE, 0);

	/* Set on start */
	init_start = gtk_check_button_new_with_label(_("Restore saved mixer levels on startup"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(init_start), \
				     prefs.set_mixer_on_start);
	gtk_signal_connect(GTK_OBJECT(init_start), "clicked",
			   GTK_SIGNAL_FUNC(bool_changed_cb), &prefs.set_mixer_on_start);

	menu_hide = gtk_check_button_new_with_label(_("Hide menu on start"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(menu_hide), \
				     prefs.hide_menu);
	gtk_signal_connect(GTK_OBJECT(menu_hide), "clicked",
			   GTK_SIGNAL_FUNC(bool_changed_cb), &prefs.hide_menu);

	gtk_box_pack_start_defaults(GTK_BOX(vbox), init_start);
	/*	gtk_box_pack_start_defaults(GTK_BOX(vbox), menu_hide);*/

	gtk_widget_show_all(vbox);


	gtk_container_add(GTK_CONTAINER(start_frame), vbox);

	gtk_container_add(GTK_CONTAINER(ubervbox), start_frame);

	gui_frame = gtk_frame_new(_("GUI"));

	vbox = gtk_vbox_new(TRUE, 0);
	temp = gtk_check_button_new_with_label(_("Use Mixer Icons"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), \
				     prefs.use_icons);
	gtk_signal_connect(GTK_OBJECT(temp), "clicked",
			   GTK_SIGNAL_FUNC(bool_changed_cb), &prefs.use_icons);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), temp);
	
	
	temp = gtk_check_button_new_with_label(_("Use Mixer Labels"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), \
				     prefs.use_labels);
	gtk_signal_connect(GTK_OBJECT(temp), "clicked",
			   GTK_SIGNAL_FUNC(bool_changed_cb), &prefs.use_labels);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), temp);
	
	
	gtk_container_add(GTK_CONTAINER(gui_frame), vbox);
	
	gtk_container_add(GTK_CONTAINER(ubervbox), gui_frame);
	
	gtk_widget_show_all(ubervbox);

	return ubervbox;
}

void config_cb(GtkWidget *widget, void *data)
{
        GtkWidget *label;
	if (!configwin) {
	        configwin=gnome_property_box_new();
		gtk_widget_realize(configwin);
		label = gtk_label_new(_("Preferences..."));
		gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(configwin)->notebook),
					 optpage(), label);
		/*    
		      label = gtk_label_new(_("Devicenames"));
		      gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(configwin)->notebook),
		      NULL, label);
		*/
		
		/*    
		      label = gtk_label_new(_("Icon Selection"));
		      gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(configwin)->notebook),
		      NULL, label);
		*/
		
		gtk_signal_connect(GTK_OBJECT(configwin), "apply",
				   GTK_SIGNAL_FUNC(apply_cb), NULL);
		gtk_signal_connect(GTK_OBJECT(configwin), "destroy",
				   GTK_SIGNAL_FUNC(cancel_cb), NULL);
		gtk_signal_connect(GTK_OBJECT(configwin), "delete_event",
				   GTK_SIGNAL_FUNC(cancel_cb), NULL);

		gtk_widget_show_all(configwin);	
    
	};
};

int main(int argc, char *argv[]) 
{
	mode=0;

	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);
	gnome_init_with_popt_table("gmix", VERSION, argc, argv, options,
				   0, NULL);

	if(mode_nosave) mode |= M_NOSAVE;
	if(mode_initonly) mode |= M_INITONLY;

	scan_devices();
	if (devices) {
		if (~mode & M_INITONLY) {
		        get_gui_config();
			/* Beweare boolean bastardization */
			if (!prefs.set_mixer_on_start)
			  mode |= M_NORESTORE;
			  
		}
		/* Command line always overrides */
		if(mode_norestore) mode |= M_NORESTORE;
		if(mode_restore) mode &= ~M_NORESTORE;

		if (~mode & M_NORESTORE) {
			get_device_config();
			init_devices();
		} 

		if (~mode & M_INITONLY) {

			open_dialog();

			put_gui_config();/* Do we need this*/
		}

		if (~mode & M_NOSAVE) {
			put_device_config();
		}
		gnome_config_sync();
		free_devices();
	} else {
	    GtkWidget *box;
	    box = gnome_error_dialog("No mixers found.\nMake sure you have sound support compiled into the kernel.");
	    gtk_signal_connect(GTK_OBJECT(box), "close",
			       GTK_SIGNAL_FUNC(error_close_cb), NULL);
	    gtk_main();
	}
	return 0;
}

/*
 * Open the device and query configuration
 */
device_info *open_device(int num)
{
	device_info *new_device;
	char device_name[255];
	int res, ver, cnt;
	/*
	 * create new device configureation
	 */
	new_device = g_new0(device_info, 1);
	/*
	 * open the mixer-device
	 */
	if (num==0) {
		sprintf(device_name, "/dev/mixer");
	} else {
		sprintf(device_name, "/dev/mixer%i", num);
	}
	new_device->fd=open(device_name, O_RDWR, 0);
	if (new_device->fd<0) {
		g_free(new_device);
		return NULL;
	}
	/*
	 * check driver-version
	 */
#ifdef OSS_GETVERSION
	res=ioctl(new_device->fd, OSS_GETVERSION, &ver);
	if ((res!=EINVAL) && (ver!=SOUND_VERSION)) {
		fprintf(stderr, "Warning: This version of gmix was compiled with a different version of\nsoundcard.h.\n");
	}
#endif
	/*
	 * mixer-name
	 */
	res=ioctl(new_device->fd, SOUND_MIXER_INFO, &new_device->info);
	if (res!=0) {
		g_free(new_device);
		return NULL;
	}
	if(!isalpha(new_device->info.name[0]))
		g_snprintf(new_device->info.name, 31, "Card %d", num+1);
	/* 
	 * several bitmasks describing the mixer
	 */
	res=ioctl(new_device->fd, SOUND_MIXER_READ_DEVMASK, &new_device->devmask);
        res|=ioctl(new_device->fd, SOUND_MIXER_READ_RECMASK, &new_device->recmask);
        res|=ioctl(new_device->fd, SOUND_MIXER_READ_RECSRC, &new_device->recsrc);
        res|=ioctl(new_device->fd, SOUND_MIXER_READ_STEREODEVS, &new_device->stereodevs);
        res|=ioctl(new_device->fd, SOUND_MIXER_READ_CAPS, &new_device->caps);
	if (res!=0) {
		g_free(new_device);
		return NULL;
	}

	/*
	 * get current volume
	 */
	new_device->mute_bitmask=0;				/* not muted */
	new_device->record_bitmask=0;				/* no recording */
	new_device->lock_bitmask=new_device->stereodevs;	/* all locked */
	new_device->enabled_bitmask=new_device->devmask;	/* all enabled */
	for (cnt=0; cnt<SOUND_MIXER_NRDEVICES; cnt++) {
		if (new_device->devmask & (1<<cnt)) {
		    unsigned long vol; // l: vol&0xff, r:(vol&0xff00)>>8
			res=ioctl(new_device->fd, MIXER_READ(cnt), &vol);
		                                                
			new_device->volume_left[cnt]=vol & 0xff;
			if (new_device->stereodevs & (1<<cnt)) {
				new_device->volume_right[cnt]=(vol&0xff00)>>8;
			} else {
				new_device->volume_right[cnt]=vol&0xff;
			}
		}
	}
#if 0
	/*
	 * print debug-information
	 */
	printf("%s\n", new_device->info.id);
	printf("%s\n", new_device->info.name);
	
	for (cnt=0; cnt<SOUND_MIXER_NRDEVICES; cnt++) {
		if (new_device->devmask & (1<<cnt)) {
			printf("%s:", device_labels[cnt]);
			if (new_device->recmask & (1<<cnt)) {
				printf("recmask ");
			}
			if (new_device->recsrc & (1<<cnt)) {
				printf("recsrc ");
			}
			if (new_device->stereodevs & (1<<cnt)) {
				printf("stereo %i/%i", new_device->volume_left[cnt], new_device->volume_right[cnt]);
			} else {
				printf("mono %i", new_device->volume_left[cnt]);
			}
			printf("\n");
		}
	}
#endif
	return new_device;
}

GList *make_channels(device_info *device)
{
	int i;
	GList *channels;
	channels=NULL;
	for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
		if (device->devmask & (1<<i)) {
			channel_info *new_channel;
			new_channel=(channel_info *)malloc(sizeof(channel_info));
			new_channel->device=device;
			new_channel->channel=i;
	 		new_channel->pixmap = g_strdup (device_pixmap[i]);
			new_channel->title= g_strdup(device_labels[i]);
			new_channel->passive=0;
			channels=g_list_append(channels, new_channel);
		}
	}
	return channels;
}

void scan_devices(void)
{
	int cnt;
	device_info *new_device;
	cnt=0; devices=NULL;
	do {
		new_device=open_device(cnt++);
		if (new_device) {
			new_device->channels=make_channels(new_device);
			devices=g_list_append(devices, new_device);
		}
	} while (new_device);
	num_mixers=cnt-1;
}

void free_one_device(gpointer a, gpointer b)
{
	device_info *info = (device_info *)a;
	close(info->fd);
}

void free_devices(void)
{
	g_list_foreach(devices, free_one_device, NULL);
	g_list_free(devices);
}

void init_one_device(gpointer a, gpointer b)
{
    unsigned long vol;
	int c;
	
	device_info *info = (device_info *)a;
	ioctl(info->fd, SOUND_MIXER_WRITE_RECSRC, &info->record_bitmask);

	for (c=0; c<SOUND_MIXER_NRDEVICES; c++) {
		if (info->devmask & (1<<c)) {
			if (info->mute_bitmask & (1<<c)) {
				vol=0;
			} else {
			    vol = info->volume_left[c];
			    vol |= info->volume_right[c]<<8;
			}
			ioctl(info->fd, MIXER_WRITE(c), &vol);
		}
	}
}

void init_devices(void)
{
	g_list_foreach(devices, init_one_device, NULL);
}

void get_one_device_config(gpointer a, gpointer b)
{
	device_info *info = (device_info *)a;
	int cc;
	/*
	 * restore mixer-configuration
	 */ 
	char name[255];
	for (cc=0; cc<SOUND_MIXER_NRDEVICES; cc++) {
		if (info->devmask & (1<<cc)) {
			sprintf(name, "/gmix/%s_%s/mute=%s", info->info.id,\
				device_names[cc],\
				(info->mute_bitmask & (1<<cc))?"true":"false");
			info->mute_bitmask&=~(1<<cc);
			info->mute_bitmask|= gnome_config_get_bool(name) ? (1<<cc) : 0;
			if (info->recmask & (1<<cc)) {
				sprintf(name, "/gmix/%s_%s/recsrc=%s", info->info.id, device_names[cc], (info->record_bitmask & (1<<cc))?"true":"false");
				info->record_bitmask&=~(1<<cc);
				info->record_bitmask|=gnome_config_get_bool(name) ? (1<<cc) : 0;
			}
			if (info->stereodevs & (1<<cc)) {
				sprintf(name, "/gmix/%s_%s/lock=%s", info->info.id, device_names[cc], (info->lock_bitmask & (1<<cc))?"true":"false");
				info->lock_bitmask&=~(1<<cc);
				info->lock_bitmask|=gnome_config_get_bool(name) ? (1<<cc) : 0;
				sprintf(name, "/gmix/%s_%s/volume_left=%i", info->info.id, device_names[cc], info->volume_left[cc]);
				info->volume_left[cc]=gnome_config_get_int(name);
				sprintf(name, "/gmix/%s_%s/volume_right=%i", info->info.id, device_names[cc], info->volume_right[cc]);
				info->volume_right[cc]=gnome_config_get_int(name);
			} else {
				sprintf(name, "/gmix/%s_%s/volume=%i", info->info.id, device_names[cc], info->volume_left[cc]);
				info->volume_left[cc]=gnome_config_get_int(name);
			}
		}
	}
}

void get_device_config(void)
{
	g_list_foreach(devices, get_one_device_config, NULL);
}

void put_one_device_config(gpointer a, gpointer b)
{
	device_info *info = (device_info *)a;
	int cc;
	/*
	 * save mixer-configuration
	 */ 
	char name[255];
	for (cc=0; cc<SOUND_MIXER_NRDEVICES; cc++) {
		if (info->devmask & (1<<cc)) {
			sprintf(name, "/gmix/%s_%s/mute", info->info.id, device_names[cc]);
			gnome_config_set_bool(name, (info->mute_bitmask & (1<<cc))!=0);
			if (info->recmask & (1<<cc)) {
				sprintf(name, "/gmix/%s_%s/recsrc", info->info.id, device_names[cc]);
				gnome_config_set_bool(name, (info->record_bitmask & (1<<cc))!=0);
			}
			if (info->stereodevs & (1<<cc)) {
				sprintf(name, "/gmix/%s_%s/lock", info->info.id, device_names[cc]);
				gnome_config_set_bool(name, (info->lock_bitmask & (1<<cc))!=0);
				sprintf(name, "/gmix/%s_%s/volume_left", info->info.id, device_names[cc]);
				gnome_config_set_int(name, info->volume_left[cc]);
				sprintf(name, "/gmix/%s_%s/volume_right", info->info.id, device_names[cc]);
				gnome_config_set_int(name, info->volume_right[cc]);
			} else {
				sprintf(name, "/gmix/%s_%s/volume", info->info.id, device_names[cc]);
				gnome_config_set_int(name, info->volume_left[cc]);
			}
		}
	}
}

void put_device_config(void)
{
	g_list_foreach(devices, put_one_device_config, NULL);
}

void get_gui_config(void)
{
	prefs.set_mixer_on_start=gnome_config_get_bool("/gmix/startup/init");
	prefs.hide_menu=gnome_config_get_bool("/gmix/gui/menu");
	prefs.use_icons=gnome_config_get_bool("/gmix/gui/icons");
	prefs.use_labels=gnome_config_get_bool("/gmix/gui/labels");
}

void put_gui_config(void)
{
	gnome_config_set_bool("/gmix/startup/init",prefs.set_mixer_on_start);
	gnome_config_set_bool("/gmix/gui/menu",prefs.hide_menu);
	gnome_config_set_bool("/gmix/gui/icons",prefs.use_icons);
	gnome_config_set_bool("/gmix/gui/labels",prefs.use_labels);
}

/*
 * dialogs:
 */
GtkWidget *make_slider_mixer(channel_info *ci)
{
	GtkWidget *hbox, *scale;
	device_info *di;
	di=ci->device;

	hbox=gtk_hbox_new(FALSE,1);

	/* Left channel */
	ci->left=gtk_adjustment_new (-ci->device->volume_left[ci->channel], -101.0, 0.0, 1.0, 1.0, 0.0);
	gtk_signal_connect(GTK_OBJECT(ci->left), "value_changed", (GtkSignalFunc)adj_left_cb, (gpointer)ci);
	scale = gtk_vscale_new (GTK_ADJUSTMENT(ci->left));
	gtk_range_set_update_policy (GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
	gtk_widget_show (scale);

	if (di->stereodevs & (1<<ci->channel)) {
		/* Right channel, display only if we have stereo */
		ci->right=gtk_adjustment_new (-ci->device->volume_right[ci->channel], -101.0, 0.0, 1.0, 1.0, 0.0);
		gtk_signal_connect(GTK_OBJECT(ci->right), "value_changed", (GtkSignalFunc)adj_right_cb, (gpointer)ci);
		scale = gtk_vscale_new (GTK_ADJUSTMENT(ci->right));
		gtk_range_set_update_policy (GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
		gtk_scale_set_draw_value (GTK_SCALE(scale), FALSE);
		gtk_box_pack_start (GTK_BOX(hbox), scale, TRUE, TRUE, 0);
		gtk_widget_show (scale);
	}

	return hbox;
}

void fill_in_device_guis(GtkWidget *notebook){
	GList *d, *c;
	GtkWidget *table, *spixmap;
	int i,j;

	i=0;
	for (d=devices; d; d=d->next) {
		device_info *di;
		di=d->data;
		j=0;
		for (c=((device_info *)d->data)->channels;c;c=c->next) {
			j+=2;
		}

		/* 
		 * David: changed 7 to 8 for table rows (06/04/1999)
		 */
		table=gtk_table_new(i*2, 8, FALSE);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			table, 
			gtk_label_new(di->info.name));
		gtk_table_set_row_spacings (GTK_TABLE (table), 0);
		gtk_table_set_col_spacings (GTK_TABLE (table), 0);
		gtk_container_border_width (GTK_CONTAINER (table), 0);
		gtk_widget_show (table);

		for (c=((device_info *)d->data)->channels;c;c=c->next) {
			GtkWidget *label, *mixer, *separator;
			channel_info *ci;
			ci=c->data;
			if ((ci->pixmap)&&(prefs.use_icons))
			 {
			  spixmap = gnome_stock_pixmap_widget (app, ci->pixmap);
                          gtk_table_attach (GTK_TABLE (table), spixmap, i, i+1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
			  gtk_widget_show (spixmap);
			 }
			if (prefs.use_labels) {
			  label=gtk_label_new(ci->title);
			  gtk_misc_set_alignment (GTK_MISC(label), 0.1, 0.5);
			  gtk_table_attach (GTK_TABLE (table), label, i, i+1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
			  gtk_widget_show(label);
			}

			mixer=make_slider_mixer(ci);
			gtk_table_attach (GTK_TABLE (table), mixer, i, i+1,\
					  3, 4, GTK_EXPAND | GTK_FILL,\
					  GTK_EXPAND | GTK_FILL, 0, 0);
			gtk_widget_show (mixer);

			if (ci->device->stereodevs & (1<<ci->channel)) {
				/* lock-button, only useful for stereo */
				ci->lock = gtk_check_button_new_with_label(_("Lock"));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ci->lock), (ci->device->lock_bitmask & (1<<ci->channel))!=0);
				gtk_signal_connect (GTK_OBJECT (ci->lock), "toggled", (GtkSignalFunc) lock_cb, (gpointer)ci);
				gtk_table_attach (GTK_TABLE (table), ci->lock, i, i+1, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
				gtk_widget_show (ci->lock);
			}
	/*
	 * recording sources
	 */
			if (ci->device->recmask & (1<<ci->channel)) {
				ci->rec = gtk_check_button_new_with_label (_("Rec."));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ci->rec), (ci->device->record_bitmask & (1<<ci->channel))!=0);
				gtk_signal_connect (GTK_OBJECT (ci->rec), "toggled", (GtkSignalFunc) rec_cb, (gpointer)ci);
				/* David: changed TOP_ATTACH to 7 and
				 * BOTTOM_ATTACH to 8   06/04/1999
				 */
				gtk_table_attach (GTK_TABLE (table), ci->rec, i, i+1, 7, 8, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
				gtk_widget_show (ci->rec);
			} else { /* 
				* David: need to init it to null
				* otherwise we get a segfault when
				* trying to toggle
				* the buttons 
				*/
				ci->rec = NULL;
			}
	
			ci->mute=gtk_check_button_new_with_label(_("Mute"));
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ci->mute), (ci->device->mute_bitmask & (1<<ci->channel))!=0);
			gtk_signal_connect (GTK_OBJECT (ci->mute), "toggled", (GtkSignalFunc) mute_cb, (gpointer)ci);
			gtk_table_attach (GTK_TABLE (table), ci->mute, i, i+1, 6, 7, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
			gtk_widget_show (ci->mute);

			separator = gtk_vseparator_new ();
			/* BOTTOM_ATTACH changed to 8 */
			gtk_table_attach (GTK_TABLE (table), separator, i+1, i+2, 1, 8, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
			gtk_widget_show (separator);
			i+=2; 
		}
	}

}


void open_dialog(void)
{
	GList *d, *c;
	
	int i,j;

	app = gnome_app_new ("gmix", _("GMIX 3.0") );
	gtk_widget_realize (app);
	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
		GTK_SIGNAL_FUNC (quit_cb),
		NULL);

	/*
	 * Build main menue
	 */
	gnome_app_create_menus(GNOME_APP(app), main_menu);

	/*
	 * count channels for table size;
	 */
	i=0;
	for (d=devices; d; d=d->next) {
		for (c=((device_info *)d->data)->channels; c; c=c->next) {
			i++;
		}
	}

	/*
	 * Build table with sliders
	 */	
	slidernotebook = gtk_notebook_new();
	gtk_widget_show(slidernotebook);
	gnome_app_set_contents(GNOME_APP (app), slidernotebook);

	fill_in_device_guis(slidernotebook);

	gtk_widget_show(app);

	/*
	 * Go into gtk event-loop
	 */
	gtk_main();
}

/*
 * GTK Callbacks:
 */
void quit_cb (GtkWidget *widget, void *data)
{
	gtk_main_quit();
}

void lock_cb (GtkWidget *widget, channel_info *data)
{
	if (data==NULL) return;
	data->device->lock_bitmask&=~(1<<data->channel);
	if (GTK_TOGGLE_BUTTON (data->lock)->active) {
		data->device->lock_bitmask|=1<<data->channel;
		GTK_ADJUSTMENT(data->right)->value=GTK_ADJUSTMENT(data->left)->value = (GTK_ADJUSTMENT(data->right)->value+GTK_ADJUSTMENT(data->left)->value)/2.0;
		gtk_signal_emit_by_name(GTK_OBJECT(data->left),"value_changed");
		gtk_signal_emit_by_name(GTK_OBJECT(data->right),"value_changed");
	}
}

void mute_cb (GtkWidget *widget, channel_info *data)
{
    unsigned long vol;
	if (data==NULL) return;
	data->device->mute_bitmask&=~(1<<data->channel);
	if (GTK_TOGGLE_BUTTON (data->mute)->active) {
		data->device->mute_bitmask|=1<<data->channel;
		vol=0;
		ioctl(data->device->fd, MIXER_WRITE(data->channel), &vol);
	} else {
		vol=data->device->volume_left[data->channel];
		vol|=data->device->volume_right[data->channel] << 8;
		ioctl(data->device->fd, MIXER_WRITE(data->channel), &vol);
	}
}

void rec_cb(GtkWidget *widget, channel_info *data)
{
	GList *c;
	if (data==NULL) return;
	if (data->passive) return;

	data->device->record_bitmask&=~(1<<data->channel);

	if (GTK_TOGGLE_BUTTON (data->rec)->active) {
		if (data->device->caps & SOUND_CAP_EXCL_INPUT) {
			data->device->record_bitmask=1<<data->channel;
		} else {
			data->device->record_bitmask|=(1<<data->channel);
		}
	}

	ioctl(data->device->fd,SOUND_MIXER_WRITE_RECSRC, &data->device->record_bitmask);
	
	ioctl(data->device->fd,SOUND_MIXER_READ_RECSRC, &data->device->record_bitmask);

	for (c=data->device->channels; c; c=c->next) {
		channel_info *info;
		info=c->data;
		info->passive=1;
		/* check to see if channel can record */
		if( info->rec != NULL )
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->rec), (data->device->record_bitmask & (1<<info->channel))!=0);
		info->passive=0;
	}

}

void adj_left_cb (GtkAdjustment *adjustment, channel_info *data)
{
    unsigned long vol;
	if (data==NULL) return;
	
	if (data->device->stereodevs & (1<<data->channel)) {
		if (data->device->lock_bitmask & (1<<data->channel)) {
			data->device->volume_right[data->channel]=data->device->volume_left[data->channel]=-GTK_ADJUSTMENT(data->left)->value;
			if (GTK_ADJUSTMENT(data->left)->value!=GTK_ADJUSTMENT(data->right)->value) {
				GTK_ADJUSTMENT(data->right)->value = GTK_ADJUSTMENT(data->left)->value;
				gtk_signal_emit_by_name(GTK_OBJECT(data->right),"value_changed");
			}
		} else {
			data->device->volume_left[data->channel]=-GTK_ADJUSTMENT(data->left)->value;
			data->device->volume_right[data->channel]=-GTK_ADJUSTMENT(data->right)->value;
		}
	} else {
		data->device->volume_left[data->channel]=-GTK_ADJUSTMENT(data->left)->value;
	}
	vol=data->device->volume_left[data->channel];
	vol|=data->device->volume_right[data->channel] << 8;
	ioctl(data->device->fd, MIXER_WRITE(data->channel), &vol);

	if (GTK_TOGGLE_BUTTON (data->mute)->active) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->mute), FALSE);
	}
}

void adj_right_cb (GtkAdjustment *adjustment, channel_info *data)
{
    unsigned long vol;
	if (data==NULL) return;
	
	if (data->device->stereodevs & (1<<data->channel)) {
		if (data->device->lock_bitmask & (1<<data->channel)) {
			data->device->volume_right[data->channel]=data->device->volume_left[data->channel]=-GTK_ADJUSTMENT(data->right)->value;
			if (GTK_ADJUSTMENT(data->left)->value!=GTK_ADJUSTMENT(data->right)->value) {
				GTK_ADJUSTMENT(data->left)->value = GTK_ADJUSTMENT(data->right)->value;
				gtk_signal_emit_by_name(GTK_OBJECT(data->left),"value_changed");
			}
		} else {
			data->device->volume_left[data->channel]=-GTK_ADJUSTMENT(data->left)->value;
			data->device->volume_right[data->channel]=-GTK_ADJUSTMENT(data->right)->value;
		}
	} else {
		data->device->volume_left[data->channel]=-GTK_ADJUSTMENT(data->left)->value;
	}
	vol=data->device->volume_left[data->channel];
	vol|=data->device->volume_right[data->channel] << 8;
	ioctl(data->device->fd, MIXER_WRITE(data->channel), &vol);

	if (GTK_TOGGLE_BUTTON (data->mute)->active) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->lock), FALSE);
	}
}

void about_cb (GtkWidget *widget, void *data)
{
	GtkWidget *about;
	static const char *authors[] = {
		"Jens Ch. Restemeier",
		NULL
	};
	about = gnome_about_new ( _("GMIX - The Gnome Mixer"), VERSION,
		"(C) 1998 Jens Ch. Restemeier",
		(const gchar**)authors,
		_("This is a mixer for OSS sound-devices."),
		NULL);
	gtk_widget_show (about);
}
