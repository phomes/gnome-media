/*
 * GNOME sound-recorder: a soundrecorder and soundplayer for GNOME.
 *
 * Copyright (C) 2000 :
 * Andreas Hyden <a.hyden@cyberpoint.se>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gconf/gconf-client.h>

#include <gnome.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "gui.h"
#include "preferences.h"

#include "prog.h"

static GConfClient *client = NULL;
extern gboolean default_file;

static void
record_timeout_changed (GConfClient *_client,
			guint cnxn_id,
			GConfEntry *entry,
			gpointer data)
{
  	GConfValue *value = gconf_entry_get_value (entry);

	record_timeout = gconf_client_get_int (client, "/apps/gnome-sound-recorder/record-timeout", NULL);
}

static void
stop_on_timeout_changed (GConfClient *_client,
			 guint cnxn_id,
			 GConfEntry *entry,
			 gpointer data)
{
	stop_on_timeout = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/stop-on-timeout", NULL);
}

static void
save_when_finished_changed (GConfClient *_client,
			    guint cnxn_id,
			    GConfEntry *entry,
			    gpointer data)
{
	save_when_finished = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/save-when-finished", NULL);
}

static void
popup_warning_changed (GConfClient *_client,
		       guint cnxn_id,
		       GConfEntry *entry,
		       gpointer data)
{
	popup_warn_mess = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/popup-warning", NULL);
}

static void
stop_record_changed (GConfClient *_client,
		     guint cnxn_id,
		     GConfEntry *entry,
		     gpointer data)
{
	stop_record = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/stop-record", NULL);
}

static void
popup_warn_mess_v_changed (GConfClient *_client,
			   guint cnxn_id,
			   GConfEntry *entry,
			   gpointer data)
{
	GConfValue *value = gconf_entry_get_value (entry);

	popup_warn_mess_v = gconf_client_get_int (client, "/apps/gnome-sound-recorder/popup-warning-v", NULL);
}

static void
stop_recording_v_changed (GConfClient *_client,
			  guint cnxn_id,
			  GConfEntry *entry,
			  gpointer data)
{
	GConfValue *value = gconf_entry_get_value (entry);

	stop_record_v = gconf_client_get_int (client, "/apps/gnome-sound-recorder/stop-recording-v", NULL);
}

static void
play_repeat_changed (GConfClient *_client,
		     guint cnxn_id,
		     GConfEntry *entry,
		     gpointer data)
{
	playrepeat = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/play-repeat", NULL);
}

static void
play_repeat_forever_changed (GConfClient *_client,
			     guint cnxn_id,
			     GConfEntry *entry,
			     gpointer data)
{
	playrepeatforever = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/play-repeat-forever", NULL);
}

static void
play_x_times_changed (GConfClient *_client,
		      guint cnxn_id,
		      GConfEntry *entry,
		      gpointer data)
{
	GConfValue *value = gconf_entry_get_value (entry);

	playxtimes = gconf_client_get_int (client, "/apps/gnome-sound-recorder/play-x-times", NULL);
}

static void
sox_command_changed (GConfClient *_client,
		     guint cnxn_id,
		     GConfEntry *entry,
		     gpointer data)
{
	const char *s;
	char *sox;
	GConfValue *value = gconf_entry_get_value (entry);

	s = gconf_client_get_string (client, "/apps/gnome-sound-recorder/sox-command", NULL);
	sox = g_find_program_in_path (s);
	if (sox == NULL) {
		g_free (sox_command);
		sox_command = g_strdup (s);
	} else {
		g_free (sox_command);
		sox_command = sox;
	}
}

static void
temp_dir_changed (GConfClient *_client,
		  guint cnxn_id,
		  GConfEntry *entry,
		  gpointer data)
{
	const char *t;
	GConfValue *value = gconf_entry_get_value (entry);

	g_free (temp_dir);
	t = gconf_client_get_string (client, "/apps/gnome-sound-recorder/tempdir", NULL);
	temp_dir = g_strdup (t);
}

static void
audio_format_changed (GConfClient *_client,
		      guint cnxn_id,
		      GConfEntry *entry,
		      gpointer data)
{
	audioformat = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/audio-format", NULL);
	if (default_file == FALSE) {
		return;
	}
	
	gtk_label_set_text (GTK_LABEL (grecord_widgets.audio_format_label),
			    audioformat ? _("Audio format: 8bit PCM") : _("Audio format: 16bit PCM"));
}

static void
sample_rate_changed (GConfClient *_client,
		     guint cnxn_id,
		     GConfEntry *entry,
		     gpointer data)
{
	const char *sample;
	char *s;
	GConfValue *value = gconf_entry_get_value (entry);
	
	g_free (samplerate);
	sample = gconf_client_get_string (client, "/apps/gnome-sound-recorder/sample-rate", NULL);
	samplerate = g_strdup (sample);

	if (default_file == FALSE) {
		return;
	}

	s = g_strdup_printf (_("Sample rate: %s"), samplerate);
	gtk_label_set_text (GTK_LABEL (grecord_widgets.sample_rate_label), s);
	g_free (s);
}

static void
channels_changed (GConfClient *_client,
		  guint cnxn_id,
		  GConfEntry *entry,
		  gpointer data)
{
	channels = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/channels", NULL);

	if (default_file == FALSE) {
		return;
	}
	
	gtk_label_set_text (GTK_LABEL (grecord_widgets.nr_of_channels_label),
			    channels ? _("Channels: mono") : _("Channels: stereo"));
}

static void
show_time_changed (GConfClient *_client,
		   guint cnxn_id,
		   GConfEntry *entry,
		   gpointer data)
{
	show_time = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/show-time", NULL);
	if (show_time) {
		gtk_widget_show (GTK_WIDGET (grecord_widgets.timespace_label));
		gtk_widget_show (GTK_WIDGET (grecord_widgets.timemin_label));
		gtk_widget_show (GTK_WIDGET (grecord_widgets.timesec_label));
	}
	else {
		gtk_widget_hide (GTK_WIDGET (grecord_widgets.timespace_label));
		gtk_widget_hide (GTK_WIDGET (grecord_widgets.timemin_label));
		gtk_widget_hide (GTK_WIDGET (grecord_widgets.timesec_label));
	}
}

static void
show_sound_info_changed (GConfClient *_client,
			 guint cnxn_id,
			 GConfEntry *entry,
			 gpointer data)
{
	show_soundinfo = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/show-sound-info", NULL);
	
	if (show_soundinfo) {
		gtk_widget_show (GTK_WIDGET (grecord_widgets.audio_format_label));
		gtk_widget_show (GTK_WIDGET (grecord_widgets.sample_rate_label));
		gtk_widget_show (GTK_WIDGET (grecord_widgets.nr_of_channels_label));
	}
	else {
		gtk_widget_hide (GTK_WIDGET (grecord_widgets.audio_format_label));
		gtk_widget_hide (GTK_WIDGET (grecord_widgets.sample_rate_label));
		gtk_widget_hide (GTK_WIDGET (grecord_widgets.nr_of_channels_label));
	}
}

void
load_config_file    (void)
{
	char *s;

	if (client == NULL) {
		client = gconf_client_get_default ();
	}

	gconf_client_add_dir (client, "/apps/gnome-sound-recorder",
			      GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	
	record_timeout = gconf_client_get_int (client,
					       "/apps/gnome-sound-recorder/record-timeout", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/record-timeout",
				 record_timeout_changed, NULL, NULL, NULL);
				 
	stop_on_timeout = gconf_client_get_bool (client,
						 "/apps/gnome-sound-recorder/stop-on-timeout", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/stop-on-timeout",
				 stop_on_timeout_changed, NULL, NULL, NULL);
	
	save_when_finished = gconf_client_get_bool (client,
						    "/apps/gnome-sound-recorder/save-when-finished", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/save-when-finished",
				 save_when_finished_changed, NULL, NULL, NULL);
	
	popup_warn_mess = gconf_client_get_bool (client,
						 "/apps/gnome-sound-recorder/popup-warning", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/popup-warning",
				 popup_warning_changed, NULL, NULL, NULL);
	
	stop_record = gconf_client_get_bool (client,
					     "/apps/gnome-sound-recorder/stop-record", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/stop-record",
				 stop_record_changed, NULL, NULL, NULL);
	
	popup_warn_mess_v = gconf_client_get_int (client,
						  "/apps/gnome-sound-recorder/popup-warning-v", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/popup-warning-v",
				 popup_warn_mess_v_changed, NULL, NULL, NULL);
	
	stop_record_v = gconf_client_get_int (client,
					      "/apps/gnome-sound-recorder/stop-recording-v", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/stop-recording-v",
				 stop_recording_v_changed, NULL, NULL, NULL);

	
	playrepeat = gconf_client_get_bool (client,
					    "/apps/gnome-sound-recorder/play-repeat", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/play-repeat",
				 play_repeat_changed, NULL, NULL, NULL);
	
	playrepeatforever = gconf_client_get_bool (client,
						   "/apps/gnome-sound-recorder/play-repeat-forever", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/play-repeat-forever",
				 play_repeat_forever_changed, NULL, NULL, NULL);
	
	playxtimes = gconf_client_get_int (client,
					   "/apps/gnome-sound-recorder/play-x-times", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/play-x-times",
				 play_x_times_changed, NULL, NULL, NULL);

	
	s = gconf_client_get_string (client,
				     "/apps/gnome-sound-recorder/sox-command", NULL);
	sox_command = gnome_is_program_in_path (s);
	if (sox_command == NULL) {
		sox_command = s;
	} else {
		g_free (s);
	}
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/sox-command",
				 sox_command_changed, NULL, NULL, NULL);
				 
	
	temp_dir = gconf_client_get_string (client, "/apps/gnome-sound-recorder/tempdir", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/tempdir",
				 temp_dir_changed, NULL, NULL, NULL);


	audioformat = gconf_client_get_bool (client,
					     "/apps/gnome-sound-recorder/audio-format", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/audio-format",
				 audio_format_changed, NULL, NULL, NULL);
	
	samplerate = gconf_client_get_string (client,
					      "/apps/gnome-sound-recorder/sample-rate", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/sample-rate",
				 sample_rate_changed, NULL, NULL, NULL);
	
	channels = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/channels", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/channels",
				 channels_changed, NULL, NULL, NULL);

	
	show_time = gconf_client_get_bool (client, "/apps/gnome-sound-recorder/show-time", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/show-time",
				 show_time_changed, NULL, NULL, NULL);
	
	show_soundinfo = gconf_client_get_bool (client,
						"/apps/gnome-sound-recorder/show-sound-info", NULL);
	gconf_client_notify_add (client, "/apps/gnome-sound-recorder/show-sound-info",
				 show_sound_info_changed, NULL, NULL, NULL);
}

void
free_vars                  (void)
{
	g_free (sox_command);
	g_free (mixer_command);
	g_free (temp_dir);
	g_free (samplerate);
}


/* Callbacks --------------------------------------------------------- */

void
widget_in_propertybox_changed (GtkWidget* widget,
			       gpointer propertybox)
{
	gnome_property_box_changed (GNOME_PROPERTY_BOX (propertybox));
}

void
on_checkbox_clicked_activate_cb (GtkWidget* widget, gpointer w)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		gtk_widget_set_sensitive (GTK_WIDGET (w), TRUE);
	else
		gtk_widget_set_sensitive (GTK_WIDGET (w), FALSE);
}

void
on_repeat_activate_cb (GtkWidget* widget,
		       gpointer data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		gtk_widget_set_sensitive (GTK_WIDGET (propertywidgets.playrepeatforever_radiobutton_v), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (propertywidgets.playxtimes_radiobutton_v), TRUE);

		/* Don't make it sensitive if playxtimes_radiobutton is selected */
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (propertywidgets.playxtimes_radiobutton_v)))
			gtk_widget_set_sensitive (GTK_WIDGET (propertywidgets.playxtimes_spinbutton_v), TRUE);
	}
	else {
		gtk_widget_set_sensitive (GTK_WIDGET (propertywidgets.playrepeatforever_radiobutton_v), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (propertywidgets.playxtimes_radiobutton_v), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (propertywidgets.playxtimes_spinbutton_v), FALSE);
	}
}

void
on_propertybox_apply_activate (GtkWidget* widget,
			       gint page_num,
			       gpointer widgets)
{
	GtkWidget* mess;
	gint choice;
	struct stat file_info;
	gchar* show_mess = NULL;
	gchar* audioformat_string = NULL;
	gchar* channels_string = NULL;
	gchar* temp_file = NULL;
	gboolean fullpath = FALSE;
	gboolean tempdir = FALSE;
	gint temp_count;

	if (page_num == -1)
		return;

	/* Get configuration from the propertybox ------------------------------------------- */
	record_timeout     = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (propertywidgets.RecordTimeout_spinbutton_v));
	stop_on_timeout    = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (propertywidgets.StopRecordOnTimeout_checkbox_v));
	save_when_finished = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (propertywidgets.PopupSaveOnTimeout_checkbox_v));
	popup_warn_mess    = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (propertywidgets.PopupWarnMessSize_checkbox_v));
	popup_warn_mess_v  = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (propertywidgets.WarningSize_spinbutton_v));
	stop_record        = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (propertywidgets.StopRecordSize_checkbox_v));
	stop_record_v      = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (propertywidgets.StopRecordSize_spinbutton_v));
    
	playrepeat         = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (propertywidgets.playrepeat_checkbox_v));
	playrepeatforever  = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (propertywidgets.playrepeatforever_radiobutton_v));
	playxtimes         = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (propertywidgets.playxtimes_spinbutton_v));
       
	
	temp_file   = g_strdup (gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.Sox_fileentry_v)))));
	
	/* Check if the given sox command exists --------------------- */
	if (!g_strcasecmp (temp_file, "")) {
		show_mess = g_strdup_printf (_("You havn't entered a sox command; you will not be able to record.\nDo you want to use it anyway?"));
		mess = gnome_message_box_new (show_mess,
					      GNOME_MESSAGE_BOX_QUESTION,
					      GNOME_STOCK_BUTTON_YES,
					      GNOME_STOCK_BUTTON_NO,
					      NULL);
		g_free (show_mess);
		choice = gnome_dialog_run (GNOME_DIALOG (mess));
		if (choice == 0) 
			sox_command = g_strdup (temp_file);
		else
			gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.Sox_fileentry_v))), sox_command);
	}
	
	/* Check whetever the specified command is a full path or just the name (check for a '/') */
	temp_count = 0;
	while (temp_file[temp_count] != '\0') {
		if (temp_file[temp_count] == '/')
			fullpath = TRUE;
		temp_count++;
	}

	if (fullpath) {
		/* Check if the given mixer command exists -------------------- */
		if (!g_file_exists (temp_file)) {
			show_mess = g_strdup_printf (_("File %s in 'sox command' does not exist.\nDo you want to use it anyway?"), temp_file);
			mess = gnome_message_box_new (show_mess,
						      GNOME_MESSAGE_BOX_QUESTION,
						      GNOME_STOCK_BUTTON_YES,
						      GNOME_STOCK_BUTTON_NO,
						      NULL);
			g_free (show_mess);
			choice = gnome_dialog_run (GNOME_DIALOG (mess));
			if (choice == 0)
				sox_command = g_strdup (temp_file);
			else
				gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.Sox_fileentry_v))), sox_command);
		}
		else
			sox_command = g_strdup (temp_file);
	}
	else
		sox_command = g_strdup (temp_file);
	
	fullpath = FALSE;
	g_free (temp_file);
	temp_file   = g_strdup (gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.Mixer_fileentry_v)))));
	
	/* Check if the given mix command exists --------------------- */
	if (!g_strcasecmp (temp_file, "")) {
		show_mess = g_strdup_printf (_("You havn't entered a mixer command; you will not be able to start the mixer.\nDo you want to use it anyway?"));
		mess = gnome_message_box_new (show_mess,
					      GNOME_MESSAGE_BOX_QUESTION,
					      GNOME_STOCK_BUTTON_YES,
					      GNOME_STOCK_BUTTON_NO,
					      NULL);
		g_free (show_mess);
		choice = gnome_dialog_run (GNOME_DIALOG (mess));
		if (choice == 0) 
			mixer_command = g_strdup (temp_file);
		else
			gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.Mixer_fileentry_v))), mixer_command);
	}
	
	/* Check whetever the specified command is a full path or just the name (check for a '/') */
	temp_count = 0;
	while (temp_file[temp_count] != '\0') {
		if (temp_file[temp_count] == '/')
			fullpath = TRUE;
		temp_count++;
	}

	if (fullpath) {
		/* Check if the given mixer command exists -------------------- */
		if (!g_file_exists (temp_file)) {
			show_mess = g_strdup_printf (_("File %s in 'mixer command' does not exist.\nDo you want to use it anyway?"), temp_file);
			mess = gnome_message_box_new (show_mess,
						      GNOME_MESSAGE_BOX_QUESTION,
						      GNOME_STOCK_BUTTON_YES,
						      GNOME_STOCK_BUTTON_NO,
						      NULL);
			g_free (show_mess);
			choice = gnome_dialog_run (GNOME_DIALOG (mess));
			if (choice == 0)
				mixer_command = g_strdup (temp_file);
			else
				gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.Mixer_fileentry_v))), mixer_command);
		}
		else
			mixer_command = g_strdup (temp_file);
	}
	else
		mixer_command = g_strdup (temp_file);
	
	g_free (temp_file);
	temp_file = g_strdup (gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.TempDir_fileentry_v)))));

	if (!g_strcasecmp (temp_file, "")) {
		show_mess = g_strdup_printf (_("You havn't entered a temp dir. You will not be able to record.\nDo you want to use it anyway?"));
		mess = gnome_message_box_new (show_mess,
					      GNOME_MESSAGE_BOX_QUESTION,
					      GNOME_STOCK_BUTTON_YES,
					      GNOME_STOCK_BUTTON_NO,
					      NULL);
		g_free (show_mess);
		choice = gnome_dialog_run (GNOME_DIALOG (mess));
		if (choice == 0) 
			temp_dir = g_strdup (temp_file);
		else
			gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.TempDir_fileentry_v))), temp_dir);
		tempdir = TRUE;
	}

	if (!tempdir) {
		/* Check if the given temp directory exists -------------------- */
		if (!stat (temp_file, &file_info)) {         /* Exists */
			if (!S_ISDIR (file_info.st_mode)) {   /* Not a directory */
				show_mess = g_strdup_printf (_("Temp directory %s is a file.\nDo you want to use it anyway?"), temp_file);
				mess = gnome_message_box_new (show_mess,
							      GNOME_MESSAGE_BOX_QUESTION,
							      GNOME_STOCK_BUTTON_YES,
							      GNOME_STOCK_BUTTON_NO,
							      NULL);
				g_free (show_mess);
				choice = gnome_dialog_run (GNOME_DIALOG (mess));
				if (choice == 0)
					temp_dir = g_strdup (temp_file);
				else
					gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.TempDir_fileentry_v))), temp_dir);
			}
			else  { /* Exists and is a directory */
				gint ok;

				/* Check if you have rw permissions */
				ok = access (temp_file, W_OK | R_OK);

				if (ok == -1) {
					show_mess = g_strdup_printf (_("You don't have the correct permissions (read & write) for temp directory %s.\nDo you want to use it anyway?"), temp_file);
					mess = gnome_message_box_new (show_mess,
								      GNOME_MESSAGE_BOX_QUESTION,
								      GNOME_STOCK_BUTTON_YES,
								      GNOME_STOCK_BUTTON_NO,
								      NULL);
					g_free (show_mess);
					choice = gnome_dialog_run (GNOME_DIALOG (mess));
					if (choice == 0)
						temp_dir = g_strdup (temp_file);
					else
						gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.TempDir_fileentry_v))), temp_dir);
				}
				else
					temp_dir = g_strdup (temp_file);
			}
		}
		else if (errno == ENOENT) {                  /* Does not exist */
			show_mess = g_strdup_printf (_("Directory %s does not exist.\nDo want to use it anyway?"), temp_file);
			mess = gnome_message_box_new (show_mess,
						      GNOME_MESSAGE_BOX_QUESTION,
						      GNOME_STOCK_BUTTON_YES,
						      GNOME_STOCK_BUTTON_NO,
						      NULL);
			g_free (show_mess);
			choice = gnome_dialog_run (GNOME_DIALOG (mess));
			if (choice == 0)
				temp_dir = g_strdup (temp_file);
			else
				gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (propertywidgets.TempDir_fileentry_v))), temp_dir);
		}
		else
			temp_dir = g_strdup (temp_file);
	}

	g_free (temp_file);

	if (g_strcasecmp (gtk_entry_get_text (GTK_ENTRY (propertywidgets.Audioformat_combo_entry_v)), "16bit pcm"))
		audioformat = TRUE;
	else
		audioformat = FALSE;
	samplerate  = g_strdup (gtk_entry_get_text (GTK_ENTRY (propertywidgets.Samplerate_combo_entry_v)));
	if (g_strcasecmp (gtk_entry_get_text (GTK_ENTRY (propertywidgets.NrChannel_combo_entry_v)), "stereo"))
		channels = TRUE;
	else
		channels = FALSE;
  
	show_time      = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (propertywidgets.show_time_checkbutton_v));
	show_soundinfo = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (propertywidgets.show_soundinfo_checkbutton_v));
	
	if (!audioformat)
		audioformat_string = g_strdup ("16bit pcm");
	else
		audioformat_string = g_strdup ("8bit pcm");

	if (!channels)
		channels_string = g_strdup ("stereo");
	else
		channels_string = g_strdup ("mono");

	/* Update some of the new information on the mainwindow */
	if (active_file == NULL) {
		gchar* temp_string = g_strconcat (_("Audioformat: "), audioformat_string, NULL);
		gtk_label_set_text (GTK_LABEL (grecord_widgets.audio_format_label), temp_string);
		g_free (temp_string);

		temp_string = g_strconcat (_("Sample rate: "), samplerate, NULL);
		gtk_label_set_text (GTK_LABEL (grecord_widgets.sample_rate_label), temp_string);
		g_free (temp_string);
		
		temp_string = g_strconcat (_("Channels: "), channels_string, NULL);
		gtk_label_set_text (GTK_LABEL (grecord_widgets.nr_of_channels_label), temp_string);
		g_free (temp_string);
	}

	g_free (audioformat_string);
	g_free (channels_string);
}

