/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * freebsd-cdrom.c: FreeBSD CD controlling functions.
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 *          Theo van Klaveren  <t.vanklaveren@student.utwente.nl>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/gnome-i18n.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef HAVE_SYS_CDIO_H
# include <sys/cdio.h>
#endif
#include <errno.h>

#include "freebsd-cdrom.h"

#define CD_FRAMES	75

static GnomeCDRomClass *parent_class = NULL;

typedef struct _FreeBSDCDRomTrackInfo {
	char *name;
	unsigned char track;
	unsigned int audio_track:1;
	GnomeCDRomMSF address;
	GnomeCDRomMSF length;
} FreeBSDCDRomTrackInfo;

struct _FreeBSDCDRomPrivate {
	char *cdrom_device;
	guint32 update_id;
	int cdrom_fd;
	int ref_count;

	GnomeCDRomUpdate update;

	struct ioc_toc_header *tochdr;
	int number_tracks;
	unsigned char track0, track1;

	FreeBSDCDRomTrackInfo *track_info;
	
	GnomeCDRomStatus *recent_status;
};

static gboolean freebsd_cdrom_eject (GnomeCDRom *cdrom,
				   GError **error);
static gboolean freebsd_cdrom_next (GnomeCDRom *cdrom,
				  GError **error);
static gboolean freebsd_cdrom_ffwd (GnomeCDRom *cdrom,
				  GError **error);
static gboolean freebsd_cdrom_play (GnomeCDRom *cdrom,
				  int start_track,
				  GnomeCDRomMSF *start,
				  int finish_track,
				  GnomeCDRomMSF *finish,
				  GError **error);
static gboolean freebsd_cdrom_pause (GnomeCDRom *cdrom,
				   GError **error);
static gboolean freebsd_cdrom_stop (GnomeCDRom *cdrom,
				  GError **error);
static gboolean freebsd_cdrom_rewind (GnomeCDRom *cdrom,
				    GError **error);
static gboolean freebsd_cdrom_back (GnomeCDRom *cdrom,
				  GError **error);
static gboolean freebsd_cdrom_get_status (GnomeCDRom *cdrom,
					GnomeCDRomStatus **status,
					GError **error);
static gboolean freebsd_cdrom_close_tray (GnomeCDRom *cdrom,
					GError **error);

static GnomeCDRomMSF blank_msf = { 0, 0, 0};

static void
finalize (GObject *object)
{
	FreeBSDCDRom *cdrom;

	cdrom = FREEBSD_CDROM (object);
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
freebsd_cdrom_open (FreeBSDCDRom *lcd,
		  GError **error)
{
	if (lcd->priv->cdrom_fd != -1) {
		lcd->priv->ref_count++;
		return TRUE;
	}

	lcd->priv->cdrom_fd = open (lcd->priv->cdrom_device, O_RDONLY | O_NONBLOCK);
	if (lcd->priv->cdrom_fd < 0) {
		if (errno == EACCES & error != NULL) {
			*error = g_error_new(GNOME_CDROM_ERROR,
					     GNOME_CDROM_ERROR_NOT_OPENED,
					     _("You do not seem to have permission to access %s."),
					     lcd->priv->cdrom_device);
		} else if (error != NULL) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_OPENED,
					      _("Unable to open %s. This may be caused by:\n"
					      "a) CD support is not compiled into the FreeBSD kernel\n"
					      "b) You do not have the correct permissions to access the CD drive\n"
					      "c) %s is not the CD drive.\n"),					     
					      lcd->priv->cdrom_device, lcd->priv->cdrom_device);
		}

		return FALSE;
	}

	lcd->priv->ref_count = 1;
	return TRUE;
}

static void
freebsd_cdrom_close (FreeBSDCDRom *lcd)
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
freebsd_cdrom_invalidate (FreeBSDCDRom *lcd)
{
	if (lcd->priv->track_info == NULL) {
		g_free (lcd->priv->track_info);
		lcd->priv->track_info = NULL;
	}
}

static void
calculate_track_lengths (FreeBSDCDRom *lcd)
{
	FreeBSDCDRomPrivate *priv;
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
freebsd_cdrom_update_cd (FreeBSDCDRom *lcd)
{
	FreeBSDCDRomPrivate *priv;
	struct ioc_read_toc_single_entry tocentry;
	int i, j;
	GError *error;

	priv = lcd->priv;

	if (freebsd_cdrom_open (lcd, &error) == FALSE) {
		g_warning (_("Error opening CD"));
		return;
	}

	if (ioctl (priv->cdrom_fd, CDIOREADTOCHEADER, priv->tochdr) < 0) {
		g_warning (_("Error reading CD header"));
		freebsd_cdrom_close (lcd);

		return;
	}
	
	priv->track0 = priv->tochdr->starting_track;
	priv->track1 = priv->tochdr->ending_track;
	priv->number_tracks = priv->track1 - priv->track0 + 1;

	freebsd_cdrom_invalidate (lcd);
	priv->track_info = g_malloc ((priv->number_tracks + 1) * sizeof (FreeBSDCDRomTrackInfo));
	for (i = 0, j = priv->track0; i < priv->number_tracks; i++, j++) {
		tocentry.track = j;
		tocentry.address_format = CD_MSF_FORMAT;

		if (ioctl (priv->cdrom_fd, CDIOREADTOCENTRY, &tocentry) < 0) {
			g_warning ("IOCtl failed");
			continue;
		}

		priv->track_info[i].track = j;
		priv->track_info[i].audio_track = tocentry.entry.control == 0 ? 1 : 0;
		ASSIGN_MSF (priv->track_info[i].address, tocentry.entry.addr.msf);
	}

	/* On FreeBSD, the leadout track is the track LAST_TRACK + 1. */
	tocentry.track = priv->number_tracks + 1;
	tocentry.address_format = CD_MSF_FORMAT;
	if (ioctl (priv->cdrom_fd, CDIOREADTOCENTRY, &tocentry) < 0) {
		g_warning ("Error getting leadout");
		freebsd_cdrom_invalidate (lcd);
		return;
	}
	ASSIGN_MSF (priv->track_info[priv->number_tracks].address, tocentry.entry.addr.msf);
	calculate_track_lengths (lcd);

#ifdef DEBUG
	g_print (_("CD changed\n"
		 "Track count: %d\n------------------\n"),
		 priv->number_tracks);
#endif
	
	freebsd_cdrom_close (lcd);
	return;
}

static gboolean
freebsd_cdrom_eject (GnomeCDRom *cdrom,
		   GError **error)
{
	FreeBSDCDRom *lcd;
	GnomeCDRomStatus *status;

	lcd = FREEBSD_CDROM (cdrom);

	if (freebsd_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_TRAY_OPEN) {
		if (ioctl (lcd->priv->cdrom_fd, CDIOCEJECT, 0) < 0) {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      _("(eject): ioctl failed: %s"),
						      strerror (errno));
			}

			g_free (status);
			freebsd_cdrom_close (lcd);
			return FALSE;
		}
	} else {
		/* Try to close the tray if it's open */
		if (gnome_cdrom_close_tray (cdrom, error) == FALSE) {
			
			g_free (status);
			freebsd_cdrom_close (lcd);

			return FALSE;
		}
	}

	g_free (status);

	lcd->priv->ref_count = 0;
	freebsd_cdrom_close (lcd);
	return TRUE;
}

static gboolean
freebsd_cdrom_next (GnomeCDRom *cdrom,
		  GError **error)
{
	FreeBSDCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF msf, *endmsf;
	int track, end_track;

	lcd = FREEBSD_CDROM (cdrom);
	if (freebsd_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (freebsd_cdrom_get_status (cdrom, &status, error) == FALSE) {
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		freebsd_cdrom_close (lcd);
		g_free (status);
		
		return TRUE;
	}

	track = status->track + 1;
	g_free (status);
	if (track > lcd->priv->number_tracks) {
		/* Do nothing */
		freebsd_cdrom_close (lcd);
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
	
	if (freebsd_cdrom_play (cdrom, track, &msf, end_track, endmsf, error) == FALSE) {
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	freebsd_cdrom_close (lcd);
	return TRUE;
}

static gboolean
freebsd_cdrom_ffwd (GnomeCDRom *cdrom,
		  GError **error)
{
	FreeBSDCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF *msf, *endmsf, end;
	int discend, frames, end_track;
	
	lcd = FREEBSD_CDROM (cdrom);
	if (freebsd_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (freebsd_cdrom_get_status (cdrom, &status, error) == FALSE) {
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		freebsd_cdrom_close (lcd);
		g_free (status);
		
		return TRUE;
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
		freebsd_cdrom_close (lcd);
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
	
	if (freebsd_cdrom_play (cdrom, -1, msf, end_track, endmsf, error) == FALSE) {
		g_free (status);
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	g_free (status);
	freebsd_cdrom_close (lcd);

	return TRUE;
}

static gboolean
freebsd_cdrom_play (GnomeCDRom *cdrom,
		  int start_track,
		  GnomeCDRomMSF *start,
		  int finish_track,
		  GnomeCDRomMSF *finish,
		  GError **error)
{
	FreeBSDCDRom *lcd;
	FreeBSDCDRomPrivate *priv;
	GnomeCDRomStatus *status;
	struct ioc_play_msf msf;
	int minutes, seconds, frames;

	lcd = FREEBSD_CDROM (cdrom);
	priv = lcd->priv;
	if (freebsd_cdrom_open(lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		if (status->cd == GNOME_CDROM_STATUS_TRAY_OPEN) {
			if (freebsd_cdrom_close_tray (cdrom, error) == FALSE) {
				freebsd_cdrom_close (lcd);
				g_free (status);
				return FALSE;
			}
		} else {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_NOT_READY,
						      _("(freebsd_cdrom_play): Drive not ready"));
			}

			freebsd_cdrom_close (lcd);
			g_free (status);
			return FALSE;
		}
	}

	g_free (status);
	/* Get the status again: It might have changed */
	if (gnome_cdrom_get_status (GNOME_CDROM (lcd), &status, error) == FALSE) {
		freebsd_cdrom_close (lcd);
		return FALSE;
	}
	if (status->cd != GNOME_CDROM_STATUS_OK) {
		/* Stuff if :) */
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_READY,
					      _("(freebsd_cdrom_play): Drive still not ready"));
		}

		freebsd_cdrom_close (lcd);
		g_free (status);
		return FALSE;
	}

	switch (status->audio) {
	case GNOME_CDROM_AUDIO_PAUSE:
		if (gnome_cdrom_pause (GNOME_CDROM (lcd), error) == FALSE) {
			g_free (status);
			freebsd_cdrom_close (lcd);
			return FALSE;
		}

	case GNOME_CDROM_AUDIO_NOTHING:
	case GNOME_CDROM_AUDIO_COMPLETE:
	case GNOME_CDROM_AUDIO_STOP:
	case GNOME_CDROM_AUDIO_ERROR:
	default:
		/* Start playing */
		if (start == NULL) {
			msf.start_m = status->absolute.minute;
			msf.start_s = status->absolute.second;
			msf.start_f = status->absolute.frame;
		} else {
			if (start_track >= 0) {
				GnomeCDRomMSF tmpmsf;
				
				add_msf (&priv->track_info[start_track - 1].address, start, &tmpmsf);
				msf.start_m = tmpmsf.minute;
				msf.start_s = tmpmsf.second;
				msf.start_f = tmpmsf.frame;
			} else {
				msf.start_m = start->minute;
				msf.start_s = start->second;
				msf.start_f = start->frame;
			}
		}

		if (finish == NULL) {
			msf.end_m = priv->track_info[priv->number_tracks].address.minute;
			msf.end_s = priv->track_info[priv->number_tracks].address.second;
			msf.end_f = priv->track_info[priv->number_tracks].address.frame;
		} else {
			if (finish_track >= 0) {
				GnomeCDRomMSF tmpmsf;

				add_msf (&priv->track_info[finish_track - 1].address, finish, &tmpmsf);
				msf.end_m = tmpmsf.minute;
				msf.end_s = tmpmsf.second;
				msf.end_f = tmpmsf.frame;
			} else {
				msf.end_m = finish->minute;
				msf.end_s = finish->second;
				msf.end_f = finish->frame;
			}
		}

		/* PLAY IT AGAIN */
		if (ioctl (priv->cdrom_fd, CDIOCPLAYMSF, &msf) < 0) {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      _("(freebsd_cdrom_play) ioctl failed %s"),
						      strerror (errno));
			}

			freebsd_cdrom_close (lcd);
			g_free (status);
			return FALSE;
		}
	}

	freebsd_cdrom_close (lcd);
	g_free (status);
	return TRUE;
}

static gboolean
freebsd_cdrom_pause (GnomeCDRom *cdrom,
		   GError **error)
{
	FreeBSDCDRom *lcd;
	GnomeCDRomStatus *status;

	lcd = FREEBSD_CDROM (cdrom);
	if (freebsd_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_READY,
					      _("(freebsd_cdrom_pause): Drive not ready"));
		}

		g_free (status);
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	if (status->audio == GNOME_CDROM_AUDIO_PAUSE) {
		if (ioctl (lcd->priv->cdrom_fd, CDIOCRESUME) < 0) {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      _("(freebsd_cdrom_pause): Resume failed %s"),
						      strerror (errno));
			}

			g_free (status);
			freebsd_cdrom_close (lcd);
			return FALSE;
		}

		freebsd_cdrom_close (lcd);
		g_free (status);
		return TRUE;
	}

	if (status->audio == GNOME_CDROM_AUDIO_PLAY) {
		if (ioctl (lcd->priv->cdrom_fd, CDIOCPAUSE, 0) < 0) {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      _("(freebsd_cdrom_pause): ioctl failed %s"),
						      strerror (errno));
			}

			g_free (status);
			freebsd_cdrom_close (lcd);
			return FALSE;
		}
	}

	g_free (status);
	freebsd_cdrom_close (lcd);
	return TRUE;
}

static gboolean
freebsd_cdrom_stop (GnomeCDRom *cdrom,
		  GError **error)
{
	FreeBSDCDRom *lcd;
	GnomeCDRomStatus *status;

	lcd = FREEBSD_CDROM (cdrom);
	if (freebsd_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

#if 0
	if (status->audio == GNOME_CDROM_AUDIO_PAUSE) {
		if (freebsd_cdrom_pause (cdrom, error) == FALSE) {
			freebsd_cdrom_close (lcd);
			g_free (status);
			return FALSE;
		}
	}
#endif

	if (ioctl (lcd->priv->cdrom_fd, CDIOCSTOP, 0) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      _("(freebsd_cdrom_stop) ioctl failed %s"),
					      strerror (errno));
		}

		freebsd_cdrom_close (lcd);
		g_free (status);
		return FALSE;
	}

	freebsd_cdrom_close (lcd);
	g_free (status);
	return TRUE;
}

static gboolean
freebsd_cdrom_rewind (GnomeCDRom *cdrom,
		    GError **error)
{
	FreeBSDCDRom *lcd;
	GnomeCDRomMSF *msf, tmpmsf, end, *endmsf;
	GnomeCDRomStatus *status;
	int discstart, frames, end_track;

	lcd = FREEBSD_CDROM (cdrom);
	if (freebsd_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (freebsd_cdrom_get_status (cdrom, &status, error) == FALSE) {
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		freebsd_cdrom_close (lcd);
		g_free (status);

		return TRUE;
	}

	msf = &status->absolute;

	frames = msf_to_frames (msf);
	frames -= CD_FRAMES; /* Back one second */

	/* Check we've not run back past the start */
	discstart = msf_to_frames (&lcd->priv->track_info[0].address);
	if (frames < discstart) {
		g_free (status);
		freebsd_cdrom_close (lcd);
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
	
	if (freebsd_cdrom_play (cdrom, -1, &tmpmsf, end_track, endmsf, error) == FALSE) {
		g_free (status);
		
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	freebsd_cdrom_close (lcd);
	g_free (status);

	return TRUE;
}

static gboolean
freebsd_cdrom_back (GnomeCDRom *cdrom,
		  GError **error)
{
	FreeBSDCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF msf, *endmsf;
	int track, end_track;

	lcd = FREEBSD_CDROM (cdrom);
	if (freebsd_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (freebsd_cdrom_get_status (cdrom, &status, error) == FALSE) {
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		freebsd_cdrom_close (lcd);
		g_free (status);

		return TRUE;
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
		freebsd_cdrom_close (lcd);
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
	if (freebsd_cdrom_play (cdrom, track, &msf, end_track, endmsf, error) == FALSE) {
		g_free (status);
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	g_free (status);
	freebsd_cdrom_close (lcd);
	return TRUE;
}

/* There should probably be 2 get_status functions. A public one and the private one.
   The private one would get called by the update handler every second, and the
   public one would just return a copy of the status */
static gboolean
freebsd_cdrom_get_status (GnomeCDRom *cdrom,
			GnomeCDRomStatus **status,
			GError **error)
{
	FreeBSDCDRom *lcd;
	FreeBSDCDRomPrivate *priv;
	GnomeCDRomStatus *realstatus;
	struct ioc_read_subchannel subchnl;
	struct cd_sub_channel_info subchnl_info;
	struct ioc_vol vol;
	int cd_status;

	g_return_val_if_fail (status != NULL, TRUE);
	
	lcd = FREEBSD_CDROM (cdrom);
	priv = lcd->priv;

	*status = g_new (GnomeCDRomStatus, 1);
	realstatus = *status;
	realstatus->volume = 0;

	if (freebsd_cdrom_open (lcd, error) == FALSE) {
		g_free (realstatus);
		freebsd_cdrom_close (lcd);
		return FALSE;
	}

#if 0
	cd_status = ioctl (priv->cdrom_fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
	if (cd_status != -1) {
		switch (cd_status) {
		case CDS_NO_INFO:
			realstatus->cd = GNOME_CDROM_STATUS_NO_DISC;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;

			freebsd_cdrom_close (lcd);
			return TRUE;

		case CDS_NO_DISC:
			realstatus->cd = GNOME_CDROM_STATUS_NO_DISC;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;

			freebsd_cdrom_close (lcd);
			return TRUE;
			
		case CDS_TRAY_OPEN:
			realstatus->cd = GNOME_CDROM_STATUS_TRAY_OPEN;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;

			freebsd_cdrom_close (lcd);
			return TRUE;

		case CDS_DRIVE_NOT_READY:
			realstatus->cd = GNOME_CDROM_STATUS_DRIVE_NOT_READY;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;
			
			freebsd_cdrom_close (lcd);
			return TRUE;

		default:
			realstatus->cd = GNOME_CDROM_STATUS_OK;
			break;
		}
	} else {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      _("(freebsd_cdrom_get_status): ioctl error %s"),
					      strerror (errno));
		}

		freebsd_cdrom_close (lcd);
		g_free (realstatus);
		return FALSE;
	}
#endif
	/* FIXME: Figure out how to do this on FreeBSD */
	realstatus->cd = GNOME_CDROM_STATUS_OK;

	subchnl.address_format = CD_MSF_FORMAT;
	subchnl.data_format = CD_CURRENT_POSITION;
	subchnl.data = &subchnl_info;
	subchnl.data_len = sizeof(struct cd_sub_channel_info);
	if (ioctl (priv->cdrom_fd, CDIOCREADSUBCHANNEL, &subchnl) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      _("(freebsd_cdrom_get_status): CDIOCREADSUBCHANNEL ioctl failed %s"),
					      strerror (errno));
		}

		freebsd_cdrom_close (lcd);
		g_free (realstatus);
		return FALSE;
	}

	/* Get the volume */
	if (ioctl (priv->cdrom_fd, CDIOCGETVOL, &vol) < 0) {
		g_warning (_("(freebsd_cdrom_get_status): CDIOCGETVOL ioctl failed %s"),
			   strerror (errno));
		realstatus->volume = -1; /* -1 means no volume command */
	} else {
		realstatus->volume = vol.vol[0];
	}

	freebsd_cdrom_close (lcd);

	ASSIGN_MSF (realstatus->relative, blank_msf);
	ASSIGN_MSF (realstatus->absolute, blank_msf);
	realstatus->track = 1;
	switch (subchnl.data->header.audio_status) {
	case CD_AS_PLAY_IN_PROGRESS:
		realstatus->audio = GNOME_CDROM_AUDIO_PLAY;
		ASSIGN_MSF (realstatus->relative, subchnl.data->what.position.reladdr.msf);
		ASSIGN_MSF (realstatus->absolute, subchnl.data->what.position.absaddr.msf);
		realstatus->track = subchnl.data->what.position.track_number;
		break;

	case CD_AS_PLAY_PAUSED:
		realstatus->audio = GNOME_CDROM_AUDIO_PAUSE;
		ASSIGN_MSF (realstatus->relative, subchnl.data->what.position.reladdr.msf);
		ASSIGN_MSF (realstatus->absolute, subchnl.data->what.position.absaddr.msf);
		realstatus->track = subchnl.data->what.position.track_number;
		break;

	case CD_AS_PLAY_COMPLETED:
		realstatus->audio = GNOME_CDROM_AUDIO_COMPLETE;
		ASSIGN_MSF (realstatus->relative, subchnl.data->what.position.reladdr.msf);
		ASSIGN_MSF (realstatus->absolute, subchnl.data->what.position.absaddr.msf);
		realstatus->track = subchnl.data->what.position.track_number;
		break;

	case CD_AS_AUDIO_INVALID:
	case CD_AS_NO_STATUS:
		realstatus->audio = GNOME_CDROM_AUDIO_STOP;
		break;

	case CD_AS_PLAY_ERROR:
	default:
		realstatus->audio = GNOME_CDROM_AUDIO_ERROR;
		break;
	}

	return TRUE;
}

static gboolean
freebsd_cdrom_close_tray (GnomeCDRom *cdrom,
			GError **error)
{
	FreeBSDCDRom *lcd;

	lcd = FREEBSD_CDROM (cdrom);
	if (freebsd_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (ioctl (lcd->priv->cdrom_fd, CDIOCCLOSE) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(freebsd_cdrom_close_tray): ioctl failed %s",
					      strerror (errno));
		}

		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	freebsd_cdrom_close (lcd);
	return TRUE;
}

static gboolean
freebsd_cdrom_set_volume (GnomeCDRom *cdrom,
			int volume,
			GError **error)
{
	FreeBSDCDRom *lcd;
	FreeBSDCDRomPrivate *priv;
	struct ioc_vol vol;

	lcd = FREEBSD_CDROM (cdrom);
	priv = lcd->priv;

	if (freebsd_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}
	
	vol.vol[0] = (u_char)volume;
	vol.vol[1] = vol.vol[2] = vol.vol[3] = vol.vol[0];
	
	if (ioctl (priv->cdrom_fd, CDIOCSETVOL, &vol) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(freebsd_cdrom_set_volume:1): ioctl failed %s",
					      strerror (errno));
		}

		freebsd_cdrom_close (lcd);
		return FALSE;
	}

	freebsd_cdrom_close (lcd);
	return TRUE;
}

static gboolean
freebsd_cdrom_is_cdrom_device (GnomeCDRom *cdrom,
			       const char *device,
			       GError **error)
{
	int fd;

	if (device == NULL || *device == 0) {
		return FALSE;
	}
	
	fd = open (device, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		return FALSE;
	}

	/* Fire a harmless ioctl at the device. */
	if (ioctl (fd, CDIOCCAPABILITY, 0) < 0) {
		/* Failed, it's not a CDROM drive */
		g_print ("%s is not a CDROM drive", device);
		close (fd);
		
		return FALSE;
	}
	
	g_print (_("%s is a CDROM drive"), device);
	close (fd);

	return TRUE;
}

static gboolean
freebsd_cdrom_get_cddb_data (GnomeCDRom *cdrom,
			   GnomeCDRomCDDBData **data,
			   GError **error)
{
	FreeBSDCDRom *lcd;
	FreeBSDCDRomPrivate *priv;
	int i, t = 0, n = 0;

	lcd = FREEBSD_CDROM (cdrom);
	priv = lcd->priv;

	if (freebsd_cdrom_open (lcd, error) == FALSE) {
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
	}

	freebsd_cdrom_close (lcd);
	return TRUE;
}

static gboolean
freebsd_cdrom_set_device (GnomeCDRom *cdrom,
			const char *device,
			GError **error)
{
	FreeBSDCDRom *lcd;
	FreeBSDCDRomPrivate *priv;
	GnomeCDRomStatus *status;
	int fd;
	
	lcd = FREEBSD_CDROM (cdrom);
	priv = lcd->priv;

	if (strcmp (priv->cdrom_device, device) == 0) {
		/* device is the same */
		return TRUE;
	}
	
	if (freebsd_cdrom_get_status (cdrom, &status, NULL) == TRUE) {
		if (status->audio == GNOME_CDROM_AUDIO_PLAY) {
			if (freebsd_cdrom_stop (cdrom, error) == FALSE) {
				return FALSE;
			}
		}
	}

	if (priv->cdrom_device != NULL) {
		g_free (priv->cdrom_device);
	}
	priv->cdrom_device = g_strdup (device);

	/* Attempt an open to see if we have permission */
	fd = open (priv->cdrom_device, O_RDONLY | O_NONBLOCK);
	if (fd < 0 && errno == EACCES && error != NULL) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_OPENED,
				      _("You do not seem to have permission to acess %s."),
				      priv->cdrom_device);
		return FALSE;
	} else if (gnome_cdrom_is_cdrom_device (GNOME_CDROM (lcd),
						lcd->priv->cdrom_device, error) == FALSE) {
		close (fd);

		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_OPENED,
					      _("%s does not point to a valid CDRom device. This may be caused by:\n"
					      "a) CD support is not compiled into Linux\n"
					      "b) You do not have the correct permissions to access the CD drive\n"
					      "c) %s is not the CD drive.\n"),					     
					      priv->cdrom_device, priv->cdrom_device);
		}
		return FALSE;
	} else {
		close (fd);
	}

	/* Force the new CD to be scanned at next update cycle */
	g_free (priv->recent_status);
	priv->recent_status = NULL;

	return TRUE;
}

static void
class_init (FreeBSDCDRomClass *klass)
{
	GObjectClass *object_class;
	GnomeCDRomClass *cdrom_class;

	object_class = G_OBJECT_CLASS (klass);
	cdrom_class = GNOME_CDROM_CLASS (klass);

	object_class->finalize = finalize;

	cdrom_class->eject = freebsd_cdrom_eject;
	cdrom_class->next = freebsd_cdrom_next;
	cdrom_class->ffwd = freebsd_cdrom_ffwd;
	cdrom_class->play = freebsd_cdrom_play;
	cdrom_class->pause = freebsd_cdrom_pause;
	cdrom_class->stop = freebsd_cdrom_stop;
	cdrom_class->rewind = freebsd_cdrom_rewind;
	cdrom_class->back = freebsd_cdrom_back;
	cdrom_class->get_status = freebsd_cdrom_get_status;
	cdrom_class->close_tray = freebsd_cdrom_close_tray;
	cdrom_class->set_volume = freebsd_cdrom_set_volume;
	cdrom_class->is_cdrom_device = freebsd_cdrom_is_cdrom_device;
	
	/* For CDDB */
  	cdrom_class->get_cddb_data = freebsd_cdrom_get_cddb_data;

	cdrom_class->set_device = freebsd_cdrom_set_device;
	
	parent_class = g_type_class_peek_parent (klass);
}

static void
init (FreeBSDCDRom *cdrom)
{
	cdrom->priv = g_new (FreeBSDCDRomPrivate, 1);
	cdrom->priv->tochdr = g_new (struct ioc_toc_header, 1);
	cdrom->priv->cdrom_fd = -1;
	cdrom->priv->ref_count = 0;
	cdrom->priv->update_id = -1;
	cdrom->priv->recent_status = NULL;
	cdrom->priv->track_info = NULL;
}

static gboolean
update_cd (gpointer data)
{
	FreeBSDCDRom *lcd;
	FreeBSDCDRomPrivate *priv;
	GnomeCDRomStatus *status;
	GError *error;

	lcd = data;
	priv = lcd->priv;

	/* Do an update */
	if (freebsd_cdrom_get_status (GNOME_CDROM (lcd), &status, &error) == FALSE) {
		g_warning ("%s: %s", __FUNCTION__, error->message);
		
		g_error_free (error);
		return TRUE;
	}
	
	if (priv->recent_status == NULL) {
		priv->recent_status = status;
		
		/* emit changed signal */
		freebsd_cdrom_update_cd (lcd);
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
				freebsd_cdrom_update_cd (lcd);
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
freebsd_cdrom_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (FreeBSDCDRomClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (FreeBSDCDRom), 0, (GInstanceInitFunc) init,
		};

		type = g_type_register_static (GNOME_CDROM_TYPE, "FreeBSDCDRom", &info, 0);
	}

	return type;
}

/* This is the creation function. It returns a GnomeCDRom object instead of
   a FreeBSDCDRom one because the caller doesn't know about FreeBSDCDRom only
   GnomeCDRom */
GnomeCDRom *
gnome_cdrom_new (const char *cdrom_device,
		 GnomeCDRomUpdate update,
		 GError **error)
{
	FreeBSDCDRom *cdrom;
	FreeBSDCDRomPrivate *priv;
	int fd;

	cdrom = g_object_new (freebsd_cdrom_get_type (), NULL);
	priv = cdrom->priv;

	priv->cdrom_device = g_strdup (cdrom_device);
	priv->update = update;

	freebsd_cdrom_open (cdrom, error);
	freebsd_cdrom_close (cdrom);
	if (gnome_cdrom_is_cdrom_device (GNOME_CDROM (cdrom),
					 cdrom->priv->cdrom_device,
					 error) == FALSE) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_OPENED,
					      "%s does not point to a valid CDRom device. This may be caused by:\n"
					      "a) CD support is not compiled into Linux\n"
					      "b) You do not have the correct permissions to access the CD drive\n"
					      "c) %s is not the CD drive.\n",					     
					      cdrom->priv->cdrom_device, cdrom->priv->cdrom_device);
		}
	}
	
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
