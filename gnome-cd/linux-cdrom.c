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
	int cdrom_fd;
	
	gboolean init;
	gboolean paused;

	struct cdrom_tochdr *tochdr;
	int number_tracks;
	unsigned char track0, track1;

	LinuxCDRomTrackInfo *track_info;
};

static gboolean linux_cdrom_eject (GnomeCDRom *cdrom,
				   GError **error);
static gboolean linux_cdrom_next (GnomeCDRom *cdrom,
				  GError **error);
static gboolean linux_cdrom_ffwd (GnomeCDRom *cdrom,
				  GError **error);
static gboolean linux_cdrom_play (GnomeCDRom *cdrom,
				  int track,
				  GnomeCDRomMSF *msf,
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

#define ASSIGN_MSF(dest, src) \
{ \
  (dest).minute = (src).minute; \
  (dest).second = (src).second; \
  (dest).frame = (src).frame; \
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
linux_cdrom_invalidate (LinuxCDRom *lcd)
{
	
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

static gboolean
linux_cdrom_close (LinuxCDRom *lcd)
{
	close (lcd->priv->cdrom_fd);
	lcd->priv->cdrom_fd = -1;
}

static gboolean
linux_cdrom_read_track_info (LinuxCDRom *lcd,
			     GError **error)
{
	LinuxCDRomPrivate *priv;
	struct cdrom_tocentry tocentry;
	int i, j;

	priv = lcd->priv;
	priv->cdrom_fd = open (priv->cdrom_device, O_RDONLY | O_NONBLOCK);
	if (priv->cdrom_fd < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_OPENED,
					      "(%s) %s could not be opened",
					      __FUNCTION__,
					      priv->cdrom_device);
		}
		return FALSE;
	}

	if (ioctl (priv->cdrom_fd, CDROMREADTOCHDR, priv->tochdr) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(%s) ioctl failed: %s",
					      __FUNCTION__,
					      strerror (errno));
		}

		linux_cdrom_close (lcd);
		return FALSE;
	}

	priv->track0 = priv->tochdr->cdth_trk0;
	priv->track1 = priv->tochdr->cdth_trk1;
	priv->number_tracks = priv->track1 - priv->track0 + 1;

	if (priv->number_tracks <= 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_IO,
					      "(%s) IO Error. Number of tracks is %d",
					      __FUNCTION__,
					      priv->number_tracks);
		}

		linux_cdrom_close (lcd);
		return FALSE;
	}

	priv->track_info = g_malloc ((priv->number_tracks + 1) * sizeof (LinuxCDRomTrackInfo));
	for (i = 0, j = priv->track0; i < priv->number_tracks; i++, j++) {
		tocentry.cdte_track = j;
		tocentry.cdte_format = CDROM_MSF;

		if (ioctl (priv->cdrom_fd, CDROMREADTOCENTRY, &tocentry) < 0) {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      "(%s) ioctl failed %s",
						      __FUNCTION__,
						      strerror (errno));
			}
			
			linux_cdrom_invalidate (lcd);
			return FALSE;
		}

		priv->track_info[i].track = j;
		priv->track_info[i].audio_track = tocentry.cdte_ctrl != 
			CDROM_DATA_TRACK ? 1 : 0;
		ASSIGN_MSF (priv->track_info[i].address, tocentry.cdte_addr.msf);
	}

	tocentry.cdte_track = CDROM_LEADOUT;
	tocentry.cdte_format = CDROM_MSF;
	if (ioctl (priv->cdrom_fd, CDROMREADTOCENTRY, &tocentry) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(%s) ioctl failed %s",
					      __FUNCTION__,
					      strerror (errno));
		}

		linux_cdrom_invalidate (lcd);
		return FALSE;
	}
	ASSIGN_MSF (priv->track_info[priv->number_tracks].address, tocentry.cdte_addr.msf);

	calculate_track_lengths (lcd);

	linux_cdrom_close (lcd);
	return TRUE;
}

static gboolean
linux_cdrom_check (LinuxCDRom *lcd,
		   GError **error)
{
	if (lcd->priv->init == FALSE) {
		lcd->priv->init = TRUE;
		if (linux_cdrom_read_track_info (lcd, error) == FALSE) {
			g_error_free (*error);
			lcd->priv->init = FALSE;
		}
	}

	lcd->priv->cdrom_fd = open (lcd->priv->cdrom_device, O_RDONLY | O_NONBLOCK);
	if (lcd->priv->cdrom_fd < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_OPENED,
					      "(%s) %s could not be opened",
					      __FUNCTION__,
					      lcd->priv->cdrom_device);
		}
		return FALSE;
	}

	return TRUE;
}

static gboolean
linux_cdrom_eject (GnomeCDRom *cdrom,
		   GError **error)
{
	LinuxCDRom *lcd;

	lcd = LINUX_CDROM (cdrom);

	if (linux_cdrom_check (lcd, error) == FALSE) {
		return FALSE;
	}

	if (ioctl (lcd->priv->cdrom_fd, CDROMEJECT, 0) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(%s) ioctl failed: %s",
					      __FUNCTION__,
					      strerror (errno));
		}

		linux_cdrom_close (lcd);
		return FALSE;
	}
	
	return TRUE;
}
	
static gboolean
linux_cdrom_next (GnomeCDRom *cdrom,
		  GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF msf;
	int track;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_check (lcd, error) == FALSE) {
		return FALSE;
	}

	if (linux_cdrom_get_status (cdrom, &status, error) == FALSE) {
		return FALSE;
	}

	track = status->track - 1;
	if (track >= lcd->priv->number_tracks - 1) {
		/* Do nothing */
		return TRUE;
	}

	msf.minute = 0;
	msf.second = 0;
	msf.frame = 0;
	if (linux_cdrom_play (cdrom, track + 1, &msf, error) == FALSE) {
		g_free (status);
		return FALSE;
	}

	g_free (status);

	return TRUE;
}

static gboolean
linux_cdrom_ffwd (GnomeCDRom *cdrom,
		  GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF *msf;
	int frames;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_check (lcd, error) == FALSE) {
		return FALSE;
	}

	if (linux_cdrom_get_status (cdrom, &status, error) == FALSE) {
		return FALSE;
	}

	msf = &status->absolute;

	msf->second++;
	if (msf->second >= 60) {
		msf->minute++;
		msf->second = 0;
	}
	msf->frame = 0;

	if (msf->minute >= lcd->priv->track_info[lcd->priv->number_tracks].address.minute &&
	    msf->second >= lcd->priv->track_info[lcd->priv->number_tracks].address.second) {
		g_free (status);
		return TRUE;
	}

	if (linux_cdrom_play (cdrom, -1, msf, error) == FALSE) {
		g_free (status);
		return FALSE;
	}

	g_free (status);

	return TRUE;
}

static gboolean
linux_cdrom_play (GnomeCDRom *cdrom,
		  int track,
		  GnomeCDRomMSF *pos,
		  GError **error)
{
	LinuxCDRom *lcd;
	LinuxCDRomPrivate *priv;
	struct cdrom_msf msf;
	int minutes, seconds, frames;

	lcd = LINUX_CDROM (cdrom);
	priv = lcd->priv;
	if (linux_cdrom_check (lcd, error) == FALSE) {
		return FALSE;
	}

	if (lcd->priv->paused) {
		if (ioctl (lcd->priv->cdrom_fd, CDROMRESUME, 0) < 0) {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      "(%s) ioctl failed %s",
						      __FUNCTION__,
						      strerror (errno));
			}
			linux_cdrom_close (lcd);
			return FALSE;
		}

		lcd->priv->paused = FALSE;

		linux_cdrom_close (lcd);
		return TRUE;
	}

	if (track >= 0) {
		msf.cdmsf_min0 = priv->track_info[track].address.minute + pos->minute;
		msf.cdmsf_sec0 = priv->track_info[track].address.second + pos->second;
		msf.cdmsf_frame0 = priv->track_info[track].address.frame + pos->frame;
	} else {
		msf.cdmsf_min0 = pos->minute;
		msf.cdmsf_sec0 = pos->second;
		msf.cdmsf_frame0 = pos->frame;
	}

	/* Set the end to be the start of the lead out track */
	msf.cdmsf_min1 = priv->track_info[priv->number_tracks].address.minute;
	msf.cdmsf_sec1 = priv->track_info[priv->number_tracks].address.second;
	msf.cdmsf_frame1 = priv->track_info[priv->number_tracks].address.frame;

	if (ioctl (priv->cdrom_fd, CDROMPLAYMSF, &msf) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(%s) ioctl failed %s",
					      __FUNCTION__,
					      strerror (errno));
		}

		linux_cdrom_close (lcd);
		return FALSE;
	}

	linux_cdrom_close (lcd);
	return TRUE;
}

static gboolean
linux_cdrom_pause (GnomeCDRom *cdrom,
		   GError **error)
{
	LinuxCDRom *lcd;

	lcd = LINUX_CDROM (cdrom);

	if (lcd->priv->paused == TRUE) {
		return;
	}

	if (linux_cdrom_check (lcd, error) == FALSE) {
		return FALSE;
	}

	if (ioctl (lcd->priv->cdrom_fd, CDROMPAUSE, 0) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(%s) ioctl failed %s",
					      __FUNCTION__,
					      strerror (errno));
		}

		linux_cdrom_close (lcd);
		return FALSE;
	}

	lcd->priv->paused = TRUE;

	linux_cdrom_close (lcd);
	return TRUE;
}

static gboolean
linux_cdrom_stop (GnomeCDRom *cdrom,
		  GError **error)
{
	LinuxCDRom *lcd;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_check (lcd, error) == FALSE) {
		return FALSE;
	}

	if (ioctl (lcd->priv->cdrom_fd, CDROMSTOP, 0) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(%s) ioctl failed %s",
					      __FUNCTION__,
					      strerror (errno));
		}

		linux_cdrom_close (lcd);
		return FALSE;
	}

	linux_cdrom_close (lcd);
	return TRUE;
}

static gboolean
linux_cdrom_rewind (GnomeCDRom *cdrom,
		    GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomMSF *msf;
	GnomeCDRomStatus *status;
	int track;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_check (lcd, error) == FALSE) {
		return FALSE;
	}

	if (linux_cdrom_get_status (cdrom, &status, error) == FALSE) {
		return FALSE;
	}

	msf = &status->absolute;

	msf->second--;
	msf->frame = 0;

	if (msf->second <= 0) {
		msf->minute--;
		if (msf->minute <= 0) {
			g_free (status);
			return TRUE;
		}

		msf->second = 59;
	}

	if (linux_cdrom_play (cdrom, -1, msf, error) == FALSE) {
		g_free (status);
		return FALSE;
	}

	g_free (status);

	return TRUE;
}

static gboolean
linux_cdrom_back (GnomeCDRom *cdrom,
		  GError **error)
{
	LinuxCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF msf;
	int track;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_check (lcd, error) == FALSE) {
		return FALSE;
	}

	if (linux_cdrom_get_status (cdrom, &status, error) == FALSE) {
		return FALSE;
	}

	if (status->relative.minute != 0 || status->relative.second != 0) {
		track = status->track - lcd->priv->track0;
	} else {
		track = status->track - lcd->priv->track0 - 1;
	}

	if (track < 0) {
		/* Do nothing */
		g_free (status);
		return TRUE;
	}

	msf.minute = 0;
	msf.second = 0;
	msf.frame = 0;
	if (linux_cdrom_play (cdrom, track, &msf, error) == FALSE) {
		g_free (status);
		return FALSE;
	}

	g_free (status);

	return TRUE;
}

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

	g_return_val_if_fail (status != NULL, FALSE);

	lcd = LINUX_CDROM (cdrom);
	priv = lcd->priv;

	*status = g_new (GnomeCDRomStatus, 1);
	realstatus = *status;

	if (linux_cdrom_check (lcd, error) == FALSE) {
		realstatus->cd = GNOME_CDROM_STATUS_NO_DISC;
		realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
		realstatus->track = -1;
		return TRUE;
	}

	cd_status = ioctl (priv->cdrom_fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
	if (cd_status != -1) {
		switch (cd_status) {
		case CDS_TRAY_OPEN:
			realstatus->cd = GNOME_CDROM_STATUS_TRAY_OPEN;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;
			return TRUE;

		case CDS_DRIVE_NOT_READY:
			realstatus->cd = GNOME_CDROM_STATUS_DRIVE_NOT_READY;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;
			return TRUE;

		default:
			realstatus->cd = GNOME_CDROM_STATUS_OK;
			break;
		}
	}

	subchnl.cdsc_format = CDROM_MSF;
	if (ioctl (priv->cdrom_fd, CDROMSUBCHNL, &subchnl) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(%s:%d) ioctl failed: %s",
					      __FUNCTION__, __LINE__,
					      strerror (errno));
		}

		realstatus->cd = GNOME_CDROM_STATUS_NO_DISC;
		realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
		realstatus->track = -1;

		linux_cdrom_close (lcd);
		return TRUE;
	}

	linux_cdrom_close (lcd);

	switch (subchnl.cdsc_audiostatus) {
	case CDROM_AUDIO_PLAY:
		realstatus->audio = GNOME_CDROM_AUDIO_PLAY;
		break;

	case CDROM_AUDIO_PAUSED:
		realstatus->audio = GNOME_CDROM_AUDIO_PAUSE;
		break;

	case CDROM_AUDIO_COMPLETED:
		realstatus->audio = GNOME_CDROM_AUDIO_COMPLETE;
		break;

	case CDROM_AUDIO_INVALID:
	case CDROM_AUDIO_NO_STATUS:
	case CDROM_AUDIO_ERROR:
		realstatus->audio = GNOME_CDROM_AUDIO_STOP;
		break;

	default:
		realstatus->audio = GNOME_CDROM_AUDIO_ERROR;
	}

	realstatus->track = subchnl.cdsc_trk;
	ASSIGN_MSF (realstatus->relative, subchnl.cdsc_reladdr.msf);
	ASSIGN_MSF (realstatus->absolute, subchnl.cdsc_absaddr.msf);

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

	if (linux_cdrom_check (lcd, error) == FALSE) {
		return FALSE;
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
	(*data)->nsecs = (priv->track_info[priv->track1].address.minute * 60) + (priv->track_info[priv->track1].address.second);
	(*data)->offsets = g_new (unsigned int, priv->track1 + 1);
	
	for (i = priv->track0; i <= priv->track1; i++) {
		(*data)->offsets[i] = msf_to_frames (&priv->track_info[i].address);
	}
		
	linux_cdrom_close (lcd);
	return TRUE;
}

static gboolean
linux_cdrom_close_tray (GnomeCDRom *cdrom,
			GError **error)
{
	LinuxCDRom *lcd;

	lcd = LINUX_CDROM (cdrom);
	if (linux_cdrom_check (lcd, error) == FALSE) {
		return FALSE;
	}

	if (ioctl (lcd->priv->cdrom_fd, CDROMCLOSETRAY) < 0) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(%s) ioctl failed %s",
					      __FUNCTION__,
					      strerror (errno));
		}

		linux_cdrom_close (lcd);
		return FALSE;
	}

	linux_cdrom_close (lcd);
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

	parent_class = g_type_class_peek_parent (klass);
}

static void
init (LinuxCDRom *cdrom)
{
	cdrom->priv = g_new (LinuxCDRomPrivate, 1);
	cdrom->priv->tochdr = g_new (struct cdrom_tochdr, 1);
	cdrom->priv->paused = FALSE;
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
		 GError **error)
{
	LinuxCDRom *cdrom;
	LinuxCDRomPrivate *priv;

	g_return_val_if_fail (cdrom_device != NULL, NULL);

	cdrom = g_object_new (linux_cdrom_get_type (), NULL);
	priv = cdrom->priv;

	priv->cdrom_device = g_strdup (cdrom_device);
	priv->init = FALSE;

	return GNOME_CDROM (cdrom);
}
