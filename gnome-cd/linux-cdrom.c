/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * linux-cdrom.c: Linux CD controlling functions.
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <errno.h>

#include "gnome-cd.h"
#include "linux-cdrom.h"

static GnomeCDRomClass *parent_class = NULL;

typedef struct _LinuxCDRomTrackInfo {
	char *name;
	unsigned char track;
	unsigned int audio_track:1;
	GnomeCDRomMSF address;
	GnomeCDRomMSF length;
} LinuxCDRomTrackInfo;

struct _LinuxCDRomPrivate {
	char *cdrom_device;
	guint32 update_id;
	int cdrom_fd;
	int ref_count;

	GnomeCDRomUpdate update;

	struct cdrom_tochdr *tochdr;
	int number_tracks;
	unsigned char track0, track1;

	LinuxCDRomTrackInfo *track_info;
	
	GnomeCDRomStatus *recent_status;
};

static gboolean linux_cdrom_eject (GnomeCDRom *cdrom,
				   GError **error);
static gboolean linux_cdrom_next (GnomeCDRom *cdrom,
				  GError **error);
static gboolean linux_cdrom_ffwd (GnomeCDRom *cdrom,
				  GError **error);
static gboolean linux_cdrom_play (GnomeCDRom *cdrom,
				  int start_track,
				  GnomeCDRomMSF *start,
				  int finish_track,
				  GnomeCDRomMSF *finish,
				  GError **error);
static gboolean linux_cdrom_pause (GnomeCDRom *cdrom,
				   GError **error);
static gboolean linux_cdrom_stop (GnomeCDRom *cdrom,
				  GError **error);
static gboolean linux_cdrom_rewind (GnomeCDRom *cdrom,
				    GError **error);
static gboolean linux_cdrom_back (GnomeCDRom *cdrom,
				  GError **error);
static gboolean linux_cdrom_get_status (GnomeCDRom *cdrom,
					GnomeCDRomStatus **status,
					GError **error);
static gboolean linux_cdrom_close_tray (GnomeCDRom *cdrom,
					GError **error);

static GnomeCDRomMSF blank_msf = { 0, 0, 0};

static void
finalize (GObject *object)
{
	LinuxCDRom *cdrom;

	cdrom = LINUX_CDROM (object);
	if (cdrom->priv == NULL) {
		return;
	}

	close (cdrom->priv->cdrom_fd);
	g_free (cdrom->priv->cdrom_device);

	g_free (cdrom->priv);
	cdrom->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static int
msf_to_frames (GnomeCDRomMSF *msf)
{
	return (msf->minute * 60 * CD_FRAMES) + (msf->second * CD_FRAMES) + msf->frame;
}

static void
frames_to_msf (GnomeCDRomMSF *msf,
	       int frames)
{
	/* Now convert the difference in frame lengths back into MSF
	   format */
	msf->minute = frames / (60 * CD_FRAMES);
	frames -= (msf->minute * 60 * CD_FRAMES);
	msf->second = frames / CD_FRAMES;
	frames -= (msf->second * CD_FRAMES);
	msf->frame = frames;
}

static void
add_msf (GnomeCDRomMSF *msf1,
	 GnomeCDRomMSF *msf2,
	 GnomeCDRomMSF *dest)
{
	int frames1, frames2, total;

	frames1 = msf_to_frames (msf1);
	frames2 = msf_to_frames (msf2);

	total = frames1 + frames2;

	frames_to_msf (dest, total);
}

static gboolean
linux_cdrom_open (LinuxCDRom *lcd,
		  GError **error)
{
	if (lcd->priv->cdrom_fd != -1) {
		lcd->priv->ref_count++;
		return TRUE;
	}

	lcd->priv->cdrom_fd = open (lcd->priv->cdrom_device, O_RDONLY | O_NONBLOCK);
	if (lcd->priv->cdrom_fd < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_OPENED,
					      "Unable to open %s. This may be caused by:\n"
					      "a) CD support is not compiled into Linux\n"
					      "b) You do not have the correct permissions to access the CD drive\n"
					      "c) %s is not the CD drive.\n",					     
					      lcd->priv->cdrom_device, lcd->priv->cdrom_device);
		}

		return FALSE;
	}

	lcd->priv->ref_count = 1;
	return TRUE;
}

static void
linux_cdrom_close (LinuxCDRom *lcd)
{
	if (lcd->priv->cdrom_fd == -1) {
		return;
	}

	lcd->priv->ref_count--;

	if (lcd->priv->ref_count <= 0) {
		close (lcd->priv->cdrom_fd);
		lcd->priv->cdrom_fd = -1;
	}

	return;
}

static void
linux_cdrom_invalidate (LinuxCDRom *lcd)
{
	if (lcd->priv->track_info == NULL) {
		g_free (lcd->priv->track_info);
		lcd->priv->track_info = NULL;
	}
}

static void
calculate_track_lengths (LinuxCDRom *lcd)
{
	LinuxCDRomPrivate *priv;
	int i;

	priv = lcd->priv;
	for (i = 0; i < priv->number_tracks; i++) {
		GnomeCDRomMSF *msf1, *msf2;
		int f1, f2, df;

		msf1 = &priv->track_info[i].address;
		msf2 = &priv->track_info[i + 1].address;

		/* Convert all addresses to frames */
		f1 = msf_to_frames (msf1);
		f2 = msf_to_frames (msf2);

		df = f2 - f1;
		frames_to_msf (&priv->track_info[i].length, df);
	}
}

static void
linux_cdrom_update_cd (LinuxCDRom *lcd)
{
	LinuxCDRomPrivate *priv;
	struct cdrom_tocentry tocentry;
	int i, j;
	GError *error;

	priv = lcd->priv;

	if (linux_cdrom_open (lcd, &error) == FALSE) {
		g_warning ("Error opening CD");
		return;
	}

	if (ioctl (priv->cdrom_fd, CDROMREADTOCHDR, priv->tochdr) < 0) {
		g_warning ("Error reading CD header");
		linux_cdrom_close (lcd);

		return;
	}
	
	priv->track0 = priv->tochdr->cdth_trk0;
	priv->track1 = priv->tochdr->cdth_trk1;
	priv->number_tracks = priv->track1 - priv->track0 + 1;

	linux_cdrom_invalidate (lcd);
	priv->track_info = g_malloc ((priv->number_tracks + 1) * sizeof (LinuxCDRomTrackInfo));
	for (i = 0, j = priv->track0; i < priv->number_tracks; i++, j++) {
		tocentry.cdte_track = j;
		tocentry.cdte_format = CDROM_MSF;

		if (ioctl (priv->cdrom_fd, CDROMREADTOCENTRY, &tocentry) < 0) {
			g_warning ("IOCtl failed");
			continue;
		}

		priv->track_info[i].track = j;
		priv->track_info[i].audio_track = tocentry.cdte_ctrl != CDROM_DATA_TRACK ? 1 : 0;
		ASSIGN_MSF (priv->track_info[i].address, tocentry.cdte_addr.msf);
	}

	tocentry.cdte_track = CDROM_LEADOUT;
	tocentry.cdte_format = CDROM_MSF;
	if (ioctl (priv->cdrom_fd, CDROMREADTOCENTRY, &tocentry) < 0) {
		g_warning ("Error getting leadout");
		linux_cdrom_invalidate (lcd);
		return;
	}
	ASSIGN_MSF (priv->track_info[priv->number_tracks].address, tocentry.cdte_addr.msf);
	calculate_track_lengths (lcd);

#ifdef DEBUG
	g_print ("CD changed\n"
		 "Track count: %d\n------------------\n",
		 priv->number_tracks);
#endif
	
	linux_cdrom_close (lcd);
	return;
}

static gboolean
linux_cdrom_eject (GnomeCDRom *cdrom,
		   GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomStatus *status;

	lcd = LINUX_CDROM (cdrom);

	if (linux_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		linux_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_TRAY_OPEN) {
		if (ioctl (lcd->priv->cdrom_fd, CDROMEJECT, 0) < 0) {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      "(eject): ioctl failed: %s",
						      strerror (errno));
			}

			g_free (status);
			linux_cdrom_close (lcd);
			return FALSE;
		}
	} else {
		/* Try to close the tray if it's open */
		if (gnome_cdrom_close_tray (cdrom, error) == FALSE) {
			
			g_free (status);
			linux_cdrom_close (lcd);

			return FALSE;
		}
	}

	g_free (status);
	linux_cdrom_close (lcd);
	return TRUE;
}

static gboolean
linux_cdrom_next (GnomeCDRom *cdrom,
		  GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF msf, *endmsf;
	int track, end_track;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (linux_cdrom_get_status (cdrom, &status, error) == FALSE) {
		linux_cdrom_close (lcd);
		return FALSE;
	}

	track = status->track + 1;
	g_free (status);
	if (track > lcd->priv->number_tracks) {
		/* Do nothing */
		linux_cdrom_close (lcd);
		return TRUE;
	}

	msf.minute = 0;
	msf.second = 0;
	msf.frame = 0;

	if (cdrom->playmode == GNOME_CDROM_WHOLE_CD) {
		end_track = -1;
		endmsf = NULL;
	} else {
		end_track = track + 1;
		endmsf = &msf;
	}
	
	if (linux_cdrom_play (cdrom, track, &msf, end_track, endmsf, error) == FALSE) {
		linux_cdrom_close (lcd);
		return FALSE;
	}

	linux_cdrom_close (lcd);
	return TRUE;
}

static gboolean
linux_cdrom_ffwd (GnomeCDRom *cdrom,
		  GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF *msf, *endmsf, end;
	int discend, frames, end_track;
	
	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (linux_cdrom_get_status (cdrom, &status, error) == FALSE) {
		linux_cdrom_close (lcd);
		return FALSE;
	}

	msf = &status->absolute;
	/* Convert MSF to frames to do calculations on it */
	frames = msf_to_frames (msf);
	/* Add a second */
	frames += CD_FRAMES;

	/* Check if we've skipped past the end */
	discend = msf_to_frames (&lcd->priv->track_info[lcd->priv->number_tracks].address);
	if (frames >= discend) {
		/* Do nothing */
		g_free (status);
		linux_cdrom_close (lcd);
		return TRUE;
	}
	
	/* Convert back to MSF */
	frames_to_msf (msf, frames);
	/* Zero the frames */
	msf->frame = 0;

	if (cdrom->playmode == GNOME_CDROM_WHOLE_CD) {
		endmsf = NULL;
		end_track = -1;
	} else {
		end.minute = 0;
		end.second = 0;
		end.frame = 0;
		endmsf = &end;
		end_track = status->track + 1;
	}
	
	if (linux_cdrom_play (cdrom, -1, msf, end_track, endmsf, error) == FALSE) {
		g_free (status);
		linux_cdrom_close (lcd);
		return FALSE;
	}

	g_free (status);
	linux_cdrom_close (lcd);

	return TRUE;
}

static gboolean
linux_cdrom_play (GnomeCDRom *cdrom,
		  int start_track,
		  GnomeCDRomMSF *start,
		  int finish_track,
		  GnomeCDRomMSF *finish,
		  GError **error)
{
	LinuxCDRom *lcd;
	LinuxCDRomPrivate *priv;
	GnomeCDRomStatus *status;
	struct cdrom_msf msf;
	int minutes, seconds, frames;

	lcd = LINUX_CDROM (cdrom);
	priv = lcd->priv;
	if (linux_cdrom_open(lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		linux_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		if (status->cd == GNOME_CDROM_STATUS_TRAY_OPEN) {
			if (linux_cdrom_close_tray (cdrom, error) == FALSE) {
				linux_cdrom_close (lcd);
				g_free (status);
				return FALSE;
			}
		} else {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_NOT_READY,
						      "(linux_cdrom_play): Drive not ready");
			}

			linux_cdrom_close (lcd);
			g_free (status);
			return FALSE;
		}
	}

	g_free (status);
	/* Get the status again: It might have changed */
	if (gnome_cdrom_get_status (GNOME_CDROM (lcd), &status, error) == FALSE) {
		linux_cdrom_close (lcd);
		return FALSE;
	}
	if (status->cd != GNOME_CDROM_STATUS_OK) {
		/* Stuff if :) */
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_READY,
					      "(linux_cdrom_play): Drive still not ready");
		}

		linux_cdrom_close (lcd);
		g_free (status);
		return FALSE;
	}

	switch (status->audio) {
	case GNOME_CDROM_AUDIO_PAUSE:
		if (gnome_cdrom_pause (GNOME_CDROM (lcd), error) == FALSE) {
			g_free (status);
			linux_cdrom_close (lcd);
			return FALSE;
		}

	case GNOME_CDROM_AUDIO_NOTHING:
	case GNOME_CDROM_AUDIO_COMPLETE:
	case GNOME_CDROM_AUDIO_STOP:
	case GNOME_CDROM_AUDIO_ERROR:
	default:
		/* Start playing */
		if (start == NULL) {
			msf.cdmsf_min0 = status->absolute.minute;
			msf.cdmsf_sec0 = status->absolute.second;
			msf.cdmsf_frame0 = status->absolute.frame;
		} else {
			if (start_track >= 0) {
				GnomeCDRomMSF tmpmsf;
				
				add_msf (&priv->track_info[start_track - 1].address, start, &tmpmsf);
				msf.cdmsf_min0 = tmpmsf.minute;
				msf.cdmsf_sec0 = tmpmsf.second;
				msf.cdmsf_frame0 = tmpmsf.frame;
			} else {
				msf.cdmsf_min0 = start->minute;
				msf.cdmsf_sec0 = start->second;
				msf.cdmsf_frame0 = start->frame;
			}
		}

		if (finish == NULL) {
			msf.cdmsf_min1 = priv->track_info[priv->number_tracks].address.minute;
			msf.cdmsf_sec1 = priv->track_info[priv->number_tracks].address.second;
			msf.cdmsf_frame1 = priv->track_info[priv->number_tracks].address.frame;
		} else {
			if (finish_track >= 0) {
				GnomeCDRomMSF tmpmsf;

				add_msf (&priv->track_info[finish_track - 1].address, finish, &tmpmsf);
				msf.cdmsf_min1 = tmpmsf.minute;
				msf.cdmsf_sec1 = tmpmsf.second;
				msf.cdmsf_frame1 = tmpmsf.frame;
			} else {
				msf.cdmsf_min1 = finish->minute;
				msf.cdmsf_sec1 = finish->second;
				msf.cdmsf_frame1 = finish->frame;
			}
		}

		/* PLAY IT AGAIN */
		if (ioctl (priv->cdrom_fd, CDROMPLAYMSF, &msf) < 0) {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      "(linux_cdrom_play) ioctl failed %s",
						      strerror (errno));
			}

			linux_cdrom_close (lcd);
			g_free (status);
			return FALSE;
		}
	}

	linux_cdrom_close (lcd);
	g_free (status);
	return TRUE;
}

static gboolean
linux_cdrom_pause (GnomeCDRom *cdrom,
		   GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomStatus *status;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		linux_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_READY,
					      "(linux_cdrom_pause): Drive not ready");
		}

		g_free (status);
		linux_cdrom_close (lcd);
		return FALSE;
	}

	if (status->audio == GNOME_CDROM_AUDIO_PAUSE) {
		if (ioctl (lcd->priv->cdrom_fd, CDROMRESUME) < 0) {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      "(linux_cdrom_pause): Resume failed %s",
						      strerror (errno));
			}

			g_free (status);
			linux_cdrom_close (lcd);
			return FALSE;
		}

		linux_cdrom_close (lcd);
		g_free (status);
		return TRUE;
	}

	if (status->audio == GNOME_CDROM_AUDIO_PLAY) {
		if (ioctl (lcd->priv->cdrom_fd, CDROMPAUSE, 0) < 0) {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      "(linux_cdrom_pause): ioctl failed %s",
						      strerror (errno));
			}

			g_free (status);
			linux_cdrom_close (lcd);
			return FALSE;
		}
	}

	g_free (status);
	linux_cdrom_close (lcd);
	return TRUE;
}

static gboolean
linux_cdrom_stop (GnomeCDRom *cdrom,
		  GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomStatus *status;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		linux_cdrom_close (lcd);
		return FALSE;
	}

#if 0
	if (status->audio == GNOME_CDROM_AUDIO_PAUSE) {
		if (linux_cdrom_pause (cdrom, error) == FALSE) {
			linux_cdrom_close (lcd);
			g_free (status);
			return FALSE;
		}
	}
#endif

	if (ioctl (lcd->priv->cdrom_fd, CDROMSTOP, 0) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(linux_cdrom_stop) ioctl failed %s",
					      strerror (errno));
		}

		linux_cdrom_close (lcd);
		g_free (status);
		return FALSE;
	}

	linux_cdrom_close (lcd);
	g_free (status);
	return TRUE;
}

static gboolean
linux_cdrom_rewind (GnomeCDRom *cdrom,
		    GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomMSF *msf, tmpmsf, end, *endmsf;
	GnomeCDRomStatus *status;
	int discstart, frames, end_track;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (linux_cdrom_get_status (cdrom, &status, error) == FALSE) {
		linux_cdrom_close (lcd);
		return FALSE;
	}

	msf = &status->absolute;

	frames = msf_to_frames (msf);
	frames -= CD_FRAMES; /* Back one second */

	/* Check we've not run back past the start */
	discstart = msf_to_frames (&lcd->priv->track_info[0].address);
	if (frames < discstart) {
		g_free (status);
		linux_cdrom_close (lcd);
		return TRUE;
	}

	frames_to_msf (&tmpmsf, frames);
	tmpmsf.frame = 0; /* Zero the frames */

	if (cdrom->playmode = GNOME_CDROM_WHOLE_CD) {
		end_track = -1;
		endmsf = NULL;
	} else {
		end_track = status->track + 1;
		end.minute = 0;
		end.second = 0;
		end.frame = 0;
		endmsf = &end;
	}
	
	if (linux_cdrom_play (cdrom, -1, &tmpmsf, end_track, endmsf, error) == FALSE) {
		g_free (status);
		
		linux_cdrom_close (lcd);
		return FALSE;
	}

	linux_cdrom_close (lcd);
	g_free (status);

	return TRUE;
}

static gboolean
linux_cdrom_back (GnomeCDRom *cdrom,
		  GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF msf, *endmsf;
	int track, end_track;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (linux_cdrom_get_status (cdrom, &status, error) == FALSE) {
		linux_cdrom_close (lcd);
		return FALSE;
	}

	/* If we're > 0:00 on the track go back to the start of it, 
	   otherwise go to the previous track */
	if (status->relative.minute != 0 || status->relative.second != 0) {
		track = status->track;
	} else {
		track = status->track - 1;
	}

	if (track <= 0) {
		/* nothing */
		g_free (status);
		linux_cdrom_close (lcd);
		return TRUE;
	}

	msf.minute = 0;
	msf.second = 0;
	msf.frame = 0;

	if (cdrom->playmode == GNOME_CDROM_WHOLE_CD) {
		end_track = -1;
		endmsf = NULL;
	} else {
		end_track = track + 1;
		endmsf = &msf;
	}
	if (linux_cdrom_play (cdrom, track, &msf, end_track, endmsf, error) == FALSE) {
		g_free (status);
		linux_cdrom_close (lcd);
		return FALSE;
	}

	g_free (status);
	linux_cdrom_close (lcd);
	return TRUE;
}

/* There should probably be 2 get_status functions. A public one and the private one.
   The private one would get called by the update handler every second, and the
   public one would just return a copy of the status */
static gboolean
linux_cdrom_get_status (GnomeCDRom *cdrom,
			GnomeCDRomStatus **status,
			GError **error)
{
	LinuxCDRom *lcd;
	LinuxCDRomPrivate *priv;
	GnomeCDRomStatus *realstatus;
	struct cdrom_subchnl subchnl;
	int cd_status;

	g_return_val_if_fail (status != NULL, TRUE);
	
	lcd = LINUX_CDROM (cdrom);
	priv = lcd->priv;

	*status = g_new (GnomeCDRomStatus, 1);
	realstatus = *status;

	if (linux_cdrom_open (lcd, error) == FALSE) {
		g_free (realstatus);
		linux_cdrom_close (lcd);
		return FALSE;
	}

	cd_status = ioctl (priv->cdrom_fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
	if (cd_status != -1) {
		switch (cd_status) {
		case CDS_NO_INFO:
			realstatus->cd = GNOME_CDROM_STATUS_NO_DISC;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;

			linux_cdrom_close (lcd);
			return TRUE;

		case CDS_NO_DISC:
			realstatus->cd = GNOME_CDROM_STATUS_NO_DISC;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;

			linux_cdrom_close (lcd);
			return TRUE;
			
		case CDS_TRAY_OPEN:
			realstatus->cd = GNOME_CDROM_STATUS_TRAY_OPEN;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;

			linux_cdrom_close (lcd);
			return TRUE;

		case CDS_DRIVE_NOT_READY:
			realstatus->cd = GNOME_CDROM_STATUS_DRIVE_NOT_READY;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;
			
			linux_cdrom_close (lcd);
			return TRUE;

		default:
			realstatus->cd = GNOME_CDROM_STATUS_OK;
			break;
		}
	} else {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(linux_cdrom_get_status): ioctl error %s",
					      strerror (errno));
		}

		linux_cdrom_close (lcd);
		g_free (realstatus);
		return FALSE;
	}

	subchnl.cdsc_format = CDROM_MSF;
	if (ioctl (priv->cdrom_fd, CDROMSUBCHNL, &subchnl) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(linux_cdrom_get_status): CDROMSUBCHNL ioctl failed %s",
					      strerror (errno));
		}

		linux_cdrom_close (lcd);
		g_free (realstatus);
		return FALSE;
	}

	linux_cdrom_close (lcd);

	ASSIGN_MSF (realstatus->relative, blank_msf);
	ASSIGN_MSF (realstatus->absolute, blank_msf);
	realstatus->track = 1;
	switch (subchnl.cdsc_audiostatus) {
	case CDROM_AUDIO_PLAY:
		realstatus->audio = GNOME_CDROM_AUDIO_PLAY;
		ASSIGN_MSF (realstatus->relative, subchnl.cdsc_reladdr.msf);
		ASSIGN_MSF (realstatus->absolute, subchnl.cdsc_absaddr.msf);
		realstatus->track = subchnl.cdsc_trk;

		break;

	case CDROM_AUDIO_PAUSED:
		realstatus->audio = GNOME_CDROM_AUDIO_PAUSE;
		ASSIGN_MSF (realstatus->relative, subchnl.cdsc_reladdr.msf);
		ASSIGN_MSF (realstatus->absolute, subchnl.cdsc_absaddr.msf);
		realstatus->track = subchnl.cdsc_trk;

		break;

	case CDROM_AUDIO_COMPLETED:
		realstatus->audio = GNOME_CDROM_AUDIO_COMPLETE;
		ASSIGN_MSF (realstatus->relative, subchnl.cdsc_reladdr.msf);
		ASSIGN_MSF (realstatus->absolute, subchnl.cdsc_absaddr.msf);
		realstatus->track = subchnl.cdsc_trk;		
		break;
		
	case CDROM_AUDIO_INVALID:
	case CDROM_AUDIO_NO_STATUS:
		realstatus->audio = GNOME_CDROM_AUDIO_STOP;
		break;
		
	case CDROM_AUDIO_ERROR:
	default:
		realstatus->audio = GNOME_CDROM_AUDIO_ERROR;
		break;
	}

	return TRUE;
}

static gboolean
linux_cdrom_close_tray (GnomeCDRom *cdrom,
			GError **error)
{
	LinuxCDRom *lcd;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (ioctl (lcd->priv->cdrom_fd, CDROMCLOSETRAY) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(linux_cdrom_close_tray): ioctl failed %s",
					      strerror (errno));
		}

		linux_cdrom_close (lcd);
		return FALSE;
	}

	linux_cdrom_close (lcd);
	return TRUE;
}

static gboolean
linux_cdrom_get_cddb_data (GnomeCDRom *cdrom,
			   GnomeCDRomCDDBData **data,
			   GError **error)
{
	LinuxCDRom *lcd;
	LinuxCDRomPrivate *priv;
	int i, t = 0, n = 0;

	lcd = LINUX_CDROM (cdrom);
	priv = lcd->priv;

	if (linux_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (priv->track_info == NULL) {
		*data = NULL;
		return TRUE;
	}
	
	*data = g_new (GnomeCDRomCDDBData, 1);

	for (i = 0; i < priv->number_tracks; i++) {
		n += cddb_sum ((priv->track_info[i].address.minute * 60) + 
			       priv->track_info[i].address.second);
		t += ((priv->track_info[i + 1].address.minute * 60) +
		      priv->track_info[i + 1].address.second) - 
			((priv->track_info[i].address.minute * 60) + 
			 priv->track_info[i].address.second);
	}

	(*data)->discid = ((n % 0xff) << 24 | t << 8 | (priv->track1));
	(*data)->ntrks = priv->track1;
	(*data)->nsecs = (priv->track_info[priv->track1].address.minute * 60) + priv->track_info[priv->track1].address.second;
	(*data)->offsets = g_new (unsigned int, priv->track1 + 1);

	for (i = priv->track0 - 1; i < priv->track1; i++) {
		(*data)->offsets[i] = msf_to_frames (&priv->track_info[i].address);
		g_print ("%d: %u\n", i, msf_to_frames (&priv->track_info[i].address));
	}

	linux_cdrom_close (lcd);
	return TRUE;
}

static gboolean
linux_cdrom_set_device (GnomeCDRom *cdrom,
			const char *device,
			GError **error)
{
	LinuxCDRom *lcd;
	LinuxCDRomPrivate *priv;
	GnomeCDRomStatus *status;
	
	lcd = LINUX_CDROM (cdrom);
	priv = lcd->priv;

	if (strcmp (priv->cdrom_device, device) == 0) {
		/* device is the same */
		return TRUE;
	}
	
	if (linux_cdrom_get_status (cdrom, &status, NULL) == TRUE) {
		if (status->audio == GNOME_CDROM_AUDIO_PLAY) {
			if (linux_cdrom_stop (cdrom, &error) == FALSE) {
				return FALSE;
			}
		}
	}

	if (priv->cdrom_device != NULL) {
		g_free (priv->cdrom_device);
	}
	priv->cdrom_device = g_strdup (device);

	/* Force the new CD to be scanned at next update cycle */
	g_free (priv->recent_status);
	priv->recent_status = NULL;

	return TRUE;
}

static void
class_init (LinuxCDRomClass *klass)
{
	GObjectClass *object_class;
	GnomeCDRomClass *cdrom_class;

	object_class = G_OBJECT_CLASS (klass);
	cdrom_class = GNOME_CDROM_CLASS (klass);

	object_class->finalize = finalize;

	cdrom_class->eject = linux_cdrom_eject;
	cdrom_class->next = linux_cdrom_next;
	cdrom_class->ffwd = linux_cdrom_ffwd;
	cdrom_class->play = linux_cdrom_play;
	cdrom_class->pause = linux_cdrom_pause;
	cdrom_class->stop = linux_cdrom_stop;
	cdrom_class->rewind = linux_cdrom_rewind;
	cdrom_class->back = linux_cdrom_back;
	cdrom_class->get_status = linux_cdrom_get_status;
	cdrom_class->close_tray = linux_cdrom_close_tray;

	/* For CDDB */
  	cdrom_class->get_cddb_data = linux_cdrom_get_cddb_data;

	cdrom_class->set_device = linux_cdrom_set_device;
	
	parent_class = g_type_class_peek_parent (klass);
}

static void
init (LinuxCDRom *cdrom)
{
	cdrom->priv = g_new (LinuxCDRomPrivate, 1);
	cdrom->priv->tochdr = g_new (struct cdrom_tochdr, 1);
	cdrom->priv->cdrom_fd = -1;
	cdrom->priv->ref_count = 0;
	cdrom->priv->update_id = -1;
	cdrom->priv->recent_status = NULL;
	cdrom->priv->track_info = NULL;
}

static gboolean
update_cd (gpointer data)
{
	LinuxCDRom *lcd;
	LinuxCDRomPrivate *priv;
	GnomeCDRomStatus *status;
	GError *error;

	lcd = data;
	priv = lcd->priv;

	/* Do an update */
	if (linux_cdrom_get_status (GNOME_CDROM (lcd), &status, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);

		g_error_free (error);
		return TRUE;
	}

	if (priv->recent_status == NULL) {
		priv->recent_status = status;
		
		/* emit changed signal */
		linux_cdrom_update_cd (lcd);
		if (priv->update != GNOME_CDROM_UPDATE_NEVER) {
			gnome_cdrom_status_changed (GNOME_CDROM (lcd), priv->recent_status);
		}
	} else {
		if (gnome_cdrom_status_equal (status, priv->recent_status) == TRUE) {
			if (priv->update == GNOME_CDROM_UPDATE_CONTINOUS) {
				/* emit changed signal */
				gnome_cdrom_status_changed (GNOME_CDROM (lcd), priv->recent_status);
			}
		} else {
			if (priv->recent_status->cd != GNOME_CDROM_STATUS_OK &&
			    status->cd == GNOME_CDROM_STATUS_OK) {
				linux_cdrom_update_cd (lcd);
			}

			g_free (priv->recent_status);
			priv->recent_status = status;

			if (priv->update != GNOME_CDROM_UPDATE_NEVER) {
				/* emit changed signal */
				gnome_cdrom_status_changed (GNOME_CDROM (lcd), priv->recent_status);
			}
		}
	}

	return TRUE;
}
	
/* API */
GType
linux_cdrom_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (LinuxCDRomClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (LinuxCDRom), 0, (GInstanceInitFunc) init,
		};

		type = g_type_register_static (GNOME_CDROM_TYPE, "LinuxCDRom", &info, 0);
	}

	return type;
}

/* This is the creation function. It returns a GnomeCDRom object instead of
   a LinuxCDRom one because the caller doesn't know about LinuxCDRom only
   GnomeCDRom */
GnomeCDRom *
gnome_cdrom_new (const char *cdrom_device,
		 GnomeCDRomUpdate update,
		 GError **error)
{
	LinuxCDRom *cdrom;
	LinuxCDRomPrivate *priv;
	int fd;

	cdrom = g_object_new (linux_cdrom_get_type (), NULL);
	priv = cdrom->priv;

	priv->cdrom_device = g_strdup (cdrom_device);
	priv->update = update;

	if (linux_cdrom_open (cdrom, error) == FALSE) {
	}
	linux_cdrom_close (cdrom);
	
	/* Force an update so that a status will always exist */
	update_cd (cdrom);
	
	/* All worked, start the counter */
	switch (update) {
	case GNOME_CDROM_UPDATE_NEVER:
		break;
		
	case GNOME_CDROM_UPDATE_WHEN_CHANGED:
	case GNOME_CDROM_UPDATE_CONTINOUS:
		priv->update_id = g_timeout_add (1000, update_cd, cdrom);
		break;
		
	default:
		break;
	}

	return GNOME_CDROM (cdrom);
}
