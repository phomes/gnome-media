/*
 * GMIX 2.0
 *
 * A program to control /dev/mixer* with a GTK+ UI.
 *
 * Copyright (C) 1997 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
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
#ifdef HAVE_LINUX_SOUNDCARD_H
#    include <linux/soundcard.h>
#else
#    include <machine/soundcard.h>
#endif

#include "gtk/gtk.h"
#include <gnome.h>

typedef struct _vol {
	unsigned char l,r;
} s_vol;

typedef struct _vol_info {
	int devnum;
	GtkObject *adjustment_l, *adjustment_r;
	GtkWidget *lock_button, *mute_button;
} s_vol_info;

int dev_mixer;
int devmask,recmask,recsrc,stereodevs,caps;

char *device_labels[] = SOUND_DEVICE_LABELS;
char *device_names[] = SOUND_DEVICE_NAMES;

s_vol_info vol_info[SOUND_MIXER_NRDEVICES];
s_vol m;

/*
 * The main window
 */

GtkWidget *window;

void toggle_recsource (GtkWidget *widget, gint devnum)
{
	if (recmask & (1<<devnum)) {
		if (caps & SOUND_CAP_EXCL_INPUT) {
			recsrc=1 << devnum;
		} else {
			if (GTK_TOGGLE_BUTTON (widget)->active)
				recsrc|=(1<<devnum);
			else
				recsrc&=~(1<<devnum);
		}
		ioctl(dev_mixer,SOUND_MIXER_WRITE_RECSRC,&recsrc);
	}
}

void toggle_lock (GtkWidget *widget, s_vol_info *info)
{
	if (info == NULL) return;
	if (GTK_TOGGLE_BUTTON (info->lock_button)->active) {
		float med;
		med = (GTK_ADJUSTMENT(info->adjustment_l)->value + GTK_ADJUSTMENT(info->adjustment_r)->value)/2.0;
		GTK_ADJUSTMENT(info->adjustment_r)->value = med;
		GTK_ADJUSTMENT(info->adjustment_l)->value = med;
		gtk_signal_emit_by_name(GTK_OBJECT(info->adjustment_l),"value_changed");
		gtk_signal_emit_by_name(GTK_OBJECT(info->adjustment_r),"value_changed");
	}
}

void toggle_mute (GtkWidget *widget, s_vol_info *info)
{
	if (info == NULL) return;
	if (GTK_TOGGLE_BUTTON (info->mute_button)->active) {
		m.l=0;m.r=0;
		ioctl(dev_mixer, MIXER_WRITE(info->devnum), &m);
	} else {
		m.l=-(GTK_ADJUSTMENT(info->adjustment_l))->value;
		m.r=-(GTK_ADJUSTMENT(info->adjustment_r))->value;
		ioctl(dev_mixer, MIXER_WRITE(info->devnum), &m);
	}
}

void move_slider_l (GtkAdjustment *adjustment, s_vol_info *info)
{
	if (info == NULL) return;
	if ((info->lock_button!=NULL)  && (GTK_TOGGLE_BUTTON (info->lock_button)->active)) {
		if (GTK_ADJUSTMENT(info->adjustment_l)->value!=GTK_ADJUSTMENT(info->adjustment_r)->value) {
			GTK_ADJUSTMENT(info->adjustment_r)->value = GTK_ADJUSTMENT(info->adjustment_l)->value;
			gtk_signal_emit_by_name(GTK_OBJECT(info->adjustment_r),"value_changed");
		}
	}
	m.l=-GTK_ADJUSTMENT(info->adjustment_l)->value;
	m.r=-GTK_ADJUSTMENT(info->adjustment_r)->value;
	ioctl(dev_mixer, MIXER_WRITE(info->devnum), &m);
	if (info->mute_button!=NULL) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(info->mute_button),FALSE);
}

void move_slider_r (GtkAdjustment *adjustment, s_vol_info *info)
{
	if (info == NULL) return;
	if ((info->lock_button!=NULL) && (GTK_TOGGLE_BUTTON (info->lock_button)->active)) {
		if (GTK_ADJUSTMENT(info->adjustment_l)->value!=GTK_ADJUSTMENT(info->adjustment_r)->value) {
			GTK_ADJUSTMENT(info->adjustment_l)->value = GTK_ADJUSTMENT(info->adjustment_r)->value;
			gtk_signal_emit_by_name(GTK_OBJECT(info->adjustment_l),"value_changed");
		}
	}
	m.l=-GTK_ADJUSTMENT(info->adjustment_l)->value;
	m.r=-GTK_ADJUSTMENT(info->adjustment_r)->value;
	ioctl(dev_mixer, MIXER_WRITE(info->devnum), &m);
	if (info->mute_button!=NULL) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(info->mute_button),FALSE);
}

void cleanup()
{
	int i;
	gint x,y,w,h;
	char cnf_string[255];
	for (i=0;i<SOUND_MIXER_NRDEVICES;i++) {
		if (devmask & (1<<i)) {
			sprintf(cnf_string, "/gmix/%s/left", device_names[i]);
			gnome_config_set_int(cnf_string, -(GTK_ADJUSTMENT(vol_info[i].adjustment_l))->value);
			sprintf(cnf_string, "/gmix/%s/right", device_names[i]);
			gnome_config_set_int(cnf_string, -(GTK_ADJUSTMENT(vol_info[i].adjustment_r))->value);
			sprintf(cnf_string, "/gmix/%s/recsrc", device_names[i]);
			gnome_config_set_int(cnf_string,  (recsrc & (1<<i)) ? 1 : 0);
		}
	}
	/*
	 * Save window position and size:
	 */
	gdk_window_get_origin(window->window, &x, &y);
	gdk_window_get_size(window->window, &w, &h);
#ifdef WE_HAVE_FIXED_DISALLOWED_SHRINKING_BETWEEN_SESSIONS
	gnome_config_set_int("/gmix/geometry/width",w);
	gnome_config_set_int("/gmix/geometry/height",h);
	gnome_config_set_int("/gmix/geometry/xpos",x);
	gnome_config_set_int("/gmix/geometry/ypos",y);
#endif
	gnome_config_sync();
	close(dev_mixer);
}

void destroy ()
{
	cleanup();
	gtk_exit (0);
}

void create_dialog()
{
	GtkWidget *box;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *hbox;
	GtkWidget *scale;
	GSList *group;
	int i;
	char cnf_string[255];

	window=gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(window),
			   "delete_event",
			   GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

	gtk_window_set_title (GTK_WINDOW(window), "G-MIX 2.0");
#ifdef WE_HAVE_FIXED_DISALLOWED_SHRINKING_BETWEEN_SESSIONS
/* Try making the window bigger, quitting gmix, starting it up
   again, and shrinking the window. If you can shrink it, then the
   disable_resize hack can go, along with the #ifdef on the
   gnome_config_set_int stuff in the config saver */
	gtk_widget_set_usize(window, 
		gnome_config_get_int("/gmix/geometry/width=-1"),
		gnome_config_get_int("/gmix/geometry/height=200"));

	gtk_widget_set_uposition(window, 
		gnome_config_get_int("/gmix/geometry/xpos=-2"),
		gnome_config_get_int("/gmix/geometry/ypos=-2"));
#else
	gtk_container_disable_resize(GTK_CONTAINER(window));
#endif
	
	box=gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(window), box);
	gtk_widget_show(box);

	for (i=0;i<SOUND_MIXER_NRDEVICES;i++) {
		if (devmask & (1<<i)) {
			vol_info[i].devnum=i;
			vol_info[i].lock_button=NULL;
			vol_info[i].mute_button=NULL;

			/* read current volume */
			ioctl(dev_mixer, MIXER_READ(i), &m);

			sprintf(cnf_string, "/gmix/%s/left=%i", device_names[i], m.l);
			vol_info[i].adjustment_l=gtk_adjustment_new (-gnome_config_get_int(cnf_string), -101.0, 0.0, 1.0, 1.0, 0.0);

			sprintf(cnf_string, "/gmix/%s/right=%i", device_names[i], m.r);
			vol_info[i].adjustment_r=gtk_adjustment_new (-gnome_config_get_int(cnf_string), -101.0, 0.0, 1.0, 1.0, 0.0);
		
			/* draw control */
			sprintf(cnf_string, "/gmix/%s/titel=%s", device_names[i], device_labels[i]);
			frame=gtk_frame_new(gnome_config_get_string(cnf_string));
			gtk_box_pack_start_defaults(GTK_BOX(box), frame);
			gtk_widget_show(frame);
		
			vbox=gtk_vbox_new(FALSE,5);
			gtk_container_add(GTK_CONTAINER(frame), vbox);
			gtk_widget_show(vbox);

			hbox=gtk_hbox_new(FALSE,5);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
			gtk_widget_show(hbox);
			
			/* Left channel */
			gtk_signal_connect(GTK_OBJECT(vol_info[i].adjustment_l),
				"value_changed",
				(GtkSignalFunc)move_slider_l,
				(gpointer)&(vol_info[i]));
			scale = gtk_vscale_new (GTK_ADJUSTMENT (vol_info[i].adjustment_l));
			gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_CONTINUOUS);
			gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
			gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
			gtk_widget_show (scale);

			if (stereodevs & (1<<i)) {
				/* Right channel, display only if we HAVE stereo */
				gtk_signal_connect(GTK_OBJECT(vol_info[i].adjustment_r),
					"value_changed",
					(GtkSignalFunc)move_slider_r,
					(gpointer)&(vol_info[i]));
				scale = gtk_vscale_new (GTK_ADJUSTMENT (vol_info[i].adjustment_r));
				gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_CONTINUOUS);
				gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
				gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
				gtk_widget_show (scale);

				/* lock-button, only useful for stereo */
				vol_info[i].lock_button=gtk_check_button_new_with_label(_("Lock"));
				gtk_signal_connect (GTK_OBJECT (vol_info[i].lock_button), 
					"toggled", 
					(GtkSignalFunc) toggle_lock, 
					(gpointer)&(vol_info[i]));
				gtk_box_pack_start (GTK_BOX (vbox), vol_info[i].lock_button, FALSE, FALSE, 0);
				gtk_widget_show (vol_info[i].lock_button);
			}

			vol_info[i].mute_button=gtk_check_button_new_with_label(_("Mute"));
			gtk_signal_connect (GTK_OBJECT (vol_info[i].mute_button),
				"toggled", 
				(GtkSignalFunc) toggle_mute, 
				(gpointer)&(vol_info[i]));
			gtk_box_pack_start (GTK_BOX (vbox), vol_info[i].mute_button, FALSE, FALSE, 0);
			gtk_widget_show (vol_info[i].mute_button);
		}		
	}

	vbox=gtk_vbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(box), vbox, TRUE, TRUE, 0);
	gtk_widget_show(vbox);

	frame=gtk_frame_new(_("rec-src"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox2=gtk_vbox_new(FALSE,5);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_widget_show(vbox2);

	group=NULL;
	for (i=0;i<SOUND_MIXER_NRDEVICES;i++) {
		if (recmask & (1<<i)) {
			sprintf(cnf_string, "/gmix/%s/titel=%s", device_names[i], device_labels[i]);
			if (caps & SOUND_CAP_EXCL_INPUT) {
				button = gtk_radio_button_new_with_label (group, gnome_config_get_string(cnf_string));
				group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
			} else {
				button = gtk_check_button_new_with_label (gnome_config_get_string(cnf_string));
			}

			sprintf(cnf_string, "/gmix/%s/recsrc=%i", device_names[i], (recsrc & (1<<i)) ? 1 : 0);
			if (gnome_config_get_int(cnf_string)>0)
				gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button),TRUE);
			else
				gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button),FALSE);

			gtk_signal_connect (GTK_OBJECT (button), 
				"toggled", 
				(GtkSignalFunc) toggle_recsource, 
				(gpointer)i);

			gtk_box_pack_start (GTK_BOX (vbox2), button, TRUE, TRUE, 0);
			gtk_widget_show (button);
		}
	}

	button=gtk_button_new_with_label(_("Exit"));
	gtk_signal_connect (GTK_OBJECT (button), 
		"clicked", 
		(GtkSignalFunc) destroy, 
		NULL);
	gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (gtk_widget_destroy),
		GTK_OBJECT (window));
	gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	gtk_widget_show(window);

	for (i=0;i<SOUND_MIXER_NRDEVICES;i++) {
		if (vol_info[i].lock_button!=NULL) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(vol_info[i].lock_button),TRUE);
		if (vol_info[i].mute_button!=NULL) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(vol_info[i].mute_button),FALSE);
	}
}

/*** gerror -> should be moved to a more general place ! *********************/

void gerror_destroy(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
}

void gerror(char *s)
{
	GtkWidget *dialog;
	GtkWidget *button;
	GtkWidget *label;
	int e;
	e=errno;

	dialog=gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW(dialog), s);
		gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		(GtkSignalFunc) gtk_main_quit,
		NULL);

	/* Errormessage */
	label = gtk_label_new (strerror(e));
	gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	/* Button to dismiss the dialog */
	button = gtk_button_new_with_label (_("OK"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		(GtkSignalFunc) gerror_destroy,
		(gpointer)dialog);
	gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area), 5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	gtk_widget_show(dialog);
	gtk_main();
}

/****************************************************************************/

int main (int argc, char *argv[])
{
	int res;

	gnome_init ("gmix", &argc, &argv);
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	if (argc==2) {
		gnome_config_set_string ("/gmix/setup/device", argv[1]);
	}
	
	dev_mixer=open(gnome_config_get_string("/gmix/setup/device=/dev/mixer"), O_RDWR, 0);

	if (dev_mixer<0) {
		gerror(argv[0]);
	} else {
		/* get mixer information */
		res=ioctl(dev_mixer, SOUND_MIXER_READ_DEVMASK, &devmask);
		if (res>=0) res=ioctl(dev_mixer, SOUND_MIXER_READ_RECMASK, &recmask);
		if (res>=0) res=ioctl(dev_mixer, SOUND_MIXER_READ_RECSRC, &recsrc);
		if (res>=0) res=ioctl(dev_mixer, SOUND_MIXER_READ_STEREODEVS, &stereodevs);
		if (res>=0) res=ioctl(dev_mixer, SOUND_MIXER_READ_CAPS, &caps);
		if (res<0) {
			gerror(argv[0]);
		} else {
			create_dialog();
			gtk_main ();
		}
		close(dev_mixer);
	}
	return 0;
}
