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

#include <gnome.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sound.h"
#include "gui.h"
#include "prog.h"

gdouble
get_play_time (const gchar* filename)
{
	struct stat fileinfo;
	gdouble size;
	gdouble play_time = 0;
	gchar* fname = NULL;
	gint soundchannels = 0;
	gint soundformat = 0;
	gboolean nofile = FALSE;

	fname = g_strdup (filename);

	if (stat(fname, &fileinfo))
		nofile = TRUE;

	if (channels)
		soundchannels = 1;
	else
		soundchannels = 2;

	if (!audioformat)
		soundformat = 1;
	else
		audioformat = 2;

	/* Playtime */
	if (!nofile) {
		size = (double) fileinfo.st_size;
		size = size / 100000;
		play_time = size / (soundchannels * soundformat);
	}
	else
		play_time = 0;

	g_free (fname);

	return play_time;
}

void
set_min_sec_time (gint sec, gboolean set_topic)
{
	static gint minutes = 0;
	static gint seconds = 0;
	
	gchar* temp_string = NULL;
	gchar* show_mess = NULL;

        // Time in seconds -> time in seconds & minutes
	minutes = (int) sec / 60;
	seconds = sec - (minutes * 60);

        // Show it on the main window
	if (minutes <= 0)
		temp_string = g_strdup ("00");
	else if (minutes < 10)
		temp_string = g_strdup_printf ("0%i", minutes);
	else
		temp_string = g_strdup_printf ("%i", minutes);

	if (set_topic)
		gtk_label_set_text (GTK_LABEL (grecord_widgets.timemin_label), "00");
	else
		gtk_label_set_text (GTK_LABEL (grecord_widgets.timemin_label), temp_string);

	if (sec != 0 && set_topic) {
		gchar* temp_string2;
		temp_string2 = g_strdup (strrchr (active_file, '/'));
		temp_string2[0] = ' ';
		show_mess = g_strconcat (_(maintopic), temp_string2, " - ", temp_string, NULL);
		g_free (temp_string2);
	}
	else if (sec == 0 && set_topic) {
		show_mess = g_strconcat (_(maintopic), _(" untitled.wav"),  " - ", "00", NULL);
	}

	g_free (temp_string);

	if (seconds <= 0)
		temp_string = g_strdup ("00");
	else if (seconds < 10)
		temp_string = g_strdup_printf ("0%i", seconds);
	else
		temp_string = g_strdup_printf ("%i", seconds);

	if (set_topic)
		gtk_label_set_text (GTK_LABEL (grecord_widgets.timesec_label), "00");
	else
		gtk_label_set_text (GTK_LABEL (grecord_widgets.timesec_label), temp_string);

	// Set topic
	if (set_topic) {
		gchar* temp;
		temp = g_strconcat (show_mess, ":", temp_string, NULL);
		gtk_window_set_title (GTK_WINDOW (grecord_widgets.grecord_window), temp);
		g_free (temp);
	}
	g_free (temp_string);
	g_free (show_mess);
}

/******************************/
/* ------ Effects ----------- */
/******************************/

void
add_echo (gchar* filename, gboolean overwrite)
{
	static gboolean first_time = TRUE;
	gchar* backup_file;
	gchar* temp_file;

	gchar* make_backup;
	gchar* command;
	gchar* copy_back;

	backup_file = g_concat_dir_and_file (temp_dir, temp_filename_backup);
	temp_file = g_concat_dir_and_file (temp_dir, temp_filename);

	make_backup = g_strconcat ("cp -f ", active_file, " ", backup_file, NULL);
	command = g_strconcat (sox_command, " ", active_file, " ", temp_filename,
			       " echo 0.8 0.88 60.0 0.4", NULL);
	copy_back = g_strconcat ("cp -f ", temp_filename, " ", active_file, NULL);

	/* Make a backup only the first time */
	if (first_time) {
		system (make_backup);
		first_time = FALSE;
	}

	run_command (make_backup, _("Making backup..."));
	run_command (command, _("Adding echo to sample..."));
	run_command (copy_back, _("Please wait..."));

	g_free (backup_file);
	g_free (temp_file);
	g_free (make_backup);
	g_free (command);
	g_free (copy_back);
}

void
remove_echo (gchar* filename, gboolean overwrite)
{
}

void
increase_speed (gchar* filename, gboolean overwrite)
{
}

void
decrease_speed (gchar* filename, gboolean overwrite)
{
}











