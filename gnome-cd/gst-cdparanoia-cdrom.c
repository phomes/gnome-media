/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gst-cdparanoia-cdrom.c: Gstreamer CD Paranoia CD controlling functions.
 *
 * Copyright (C) 2001 Iain Holmes
 * Copyright (C) 2003 Jonathan Nall
 * Authors: Iain Holmes  <iain@ximian.com>
 *          Jonathan Nall  <nall@themountaingoats.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <libgnome/gnome-i18n.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "gst-cdparanoia-cdrom.h"
#include "cddb.h"

#include <gst/gconf/gconf.h>
#include <gst/gst.h>
#ifdef __linux__
#include <linux/cdrom.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/cdio.h>
#if defined(__FreeBSD__)
#include <sys/cdrio.h>
#endif
#define CD_FRAMES		75
#define CD_MSF_OFFSET		150
#define CDROM_DATA_TRACK	0x04
#define CDROM_LEADOUT		0xAA
#endif

static GnomeCDRomClass *parent_class = NULL;

typedef struct _GstCdparanoiaCDRomTrackInfo {
	char *name;
	unsigned char track;
	unsigned int audio_track:1;
	GnomeCDRomMSF address;
	GnomeCDRomMSF length;
} GstCdparanoiaCDRomTrackInfo;

struct _GstCdparanoiaCDRomPrivate {
	GnomeCDRomUpdate update;

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	struct ioc_toc_header *tochdr;
#else
	struct cdrom_tochdr *tochdr;
#endif
	int number_tracks;
	unsigned char track0, track1;
	char *cd_device;

	GstCdparanoiaCDRomTrackInfo *track_info;

	GstElement *play_thread;
	GstElement *cdparanoia;
	GstElement *audio_sink;
	GstElement *vol_element;

	GstFormat sector_format;
	GstPad *cdp_pad;
	int cur_track;		/* keep up with which track we're playing locally */
	int cur_rel_frame;
	int cur_abs_frame;

	int check_playtime;
	int end_playtime;
};

static gboolean gst_cdparanoia_cdrom_eject (GnomeCDRom * cdrom,
					    GError ** error);
static gboolean gst_cdparanoia_cdrom_next (GnomeCDRom * cdrom,
					   GError ** error);
static gboolean gst_cdparanoia_cdrom_ffwd (GnomeCDRom * cdrom,
					   GError ** error);
static gboolean gst_cdparanoia_cdrom_play (GnomeCDRom * cdrom,
					   int start_track,
					   GnomeCDRomMSF * start,
					   int finish_track,
					   GnomeCDRomMSF * finish,
					   GError ** error);
static gboolean gst_cdparanoia_cdrom_pause (GnomeCDRom * cdrom,
					    GError ** error);
static gboolean gst_cdparanoia_cdrom_stop (GnomeCDRom * cdrom,
					   GError ** error);
static gboolean gst_cdparanoia_cdrom_rewind (GnomeCDRom * cdrom,
					     GError ** error);
static gboolean gst_cdparanoia_cdrom_back (GnomeCDRom * cdrom,
					   GError ** error);
static gboolean gst_cdparanoia_cdrom_get_status (GnomeCDRom * cdrom,
						 GnomeCDRomStatus **
						 status, GError ** error);
static gboolean gst_cdparanoia_cdrom_close_tray (GnomeCDRom * cdrom,
						 GError ** error);

static GnomeCDRomMSF blank_msf = { 0, 0, 0 };

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
static guint64
msf_struct_to_frames (struct ioc_play_msf *msf, int entry)
{
	guint64 frames;
	if (entry == 0) {
		frames =
		    (msf->start_m * 60 * CD_FRAMES) +
		    (msf->start_s * CD_FRAMES) + msf->start_f;
	} else {
		frames =
		    (msf->end_m * 60 * CD_FRAMES) +
		    (msf->end_s * CD_FRAMES) + msf->end_f;
	}
	return (frames - CD_MSF_OFFSET);
}
#else
static guint64
msf_struct_to_frames (struct cdrom_msf *msf, int entry)
{
	guint64 frames;
	if (entry == 0) {
		frames =
		    (msf->cdmsf_min0 * 60 * CD_FRAMES) +
		    (msf->cdmsf_sec0 * CD_FRAMES) + msf->cdmsf_frame0;
	} else {
		frames =
		    (msf->cdmsf_min1 * 60 * CD_FRAMES) +
		    (msf->cdmsf_sec1 * CD_FRAMES) + msf->cdmsf_frame1;
	}
	return (frames - CD_MSF_OFFSET);
}
#endif

static int
msf_to_frames (GnomeCDRomMSF * msf)
{
	return (msf->minute * 60 * CD_FRAMES) + (msf->second * CD_FRAMES) +
	    msf->frame;
}

static void
frames_to_msf (GnomeCDRomMSF * msf, int frames)
{
	/* Now convert the difference in frame lengths back into MSF
	   format */
	msf->minute = frames / (60 * CD_FRAMES);
	frames -= (msf->minute * 60 * CD_FRAMES);
	msf->second = frames / CD_FRAMES;
	frames -= (msf->second * CD_FRAMES);
	msf->frame = frames;
}

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
static void
frames_to_msf_struct (union msf_lba *msflba, int frames)
{
	/* Now convert the difference in frame lengths back into MSF
	   format */
	msflba->msf.minute = frames / (60 * CD_FRAMES);
	frames -= (msflba->msf.minute * 60 * CD_FRAMES);
	msflba->msf.second = frames / CD_FRAMES;
	frames -= (msflba->msf.second * CD_FRAMES);
	msflba->msf.frame = frames;
}
#else
static void
frames_to_msf_struct (struct cdrom_msf0 *msf, int frames)
{
	/* Now convert the difference in frame lengths back into MSF
	   format */
	msf->minute = frames / (60 * CD_FRAMES);
	frames -= (msf->minute * 60 * CD_FRAMES);
	msf->second = frames / CD_FRAMES;
	frames -= (msf->second * CD_FRAMES);
	msf->frame = frames;
}
#endif

static void
add_msf (GnomeCDRomMSF * msf1, GnomeCDRomMSF * msf2, GnomeCDRomMSF * dest)
{
	int frames1, frames2, total;

	frames1 = msf_to_frames (msf1);
	frames2 = msf_to_frames (msf2);

	total = frames1 + frames2;

	frames_to_msf (dest, total);
}

/* eos will be called when the src element has an end of stream */
static void
eos (GstElement * element, gpointer data)
{
	/*GstCdparanoiaCDRomPrivate *priv =
	    (GstCdparanoiaCDRomPrivate *) data;*/
}

static void
check_playtime (GstBin * bin, gpointer data)
{
	GstCdparanoiaCDRomPrivate *priv =
	    (GstCdparanoiaCDRomPrivate *) data;
	guint64 value = 0;
	gboolean ret;
	int i = 0;

	/* figure out where we are in the stream */
	ret =
	    gst_pad_query (GST_PAD (priv->cdp_pad), GST_QUERY_POSITION,
			   &priv->sector_format, &value);
	if (ret && priv->check_playtime && value >= priv->end_playtime) {
		gst_element_set_state (GST_ELEMENT (priv->play_thread),
				       GST_STATE_NULL);
		priv->check_playtime = 0;
	} else {
		for (i = 0; i < priv->number_tracks; i++) {
			if (value <
			    (msf_to_frames (&priv->track_info[i].address) -
			     CD_MSF_OFFSET)) {
				priv->cur_track = i;
				break;
			}
		}
	}
}

static void
cb_error (GstElement *el, GstElement *src,
	  GError *err, gchar *debug, gpointer data)
{
	g_warning (err->message);
}

static void
build_pipeline (GstCdparanoiaCDRom * lcd)
{
	GstCdparanoiaCDRomPrivate *priv;
	static int pipeline_built = 0;
	char *sink;
	char *sink_options_start = NULL;
	GstElement *conv, *scale, *vol, *pthread, *queue;

	if (pipeline_built == 1)
		return; 

	priv = lcd->priv;

	priv->play_thread = gst_thread_new ("play_thread");
	g_assert (priv->play_thread != 0);	/* TBD: GError */

	g_signal_connect (priv->play_thread, "error",
			  G_CALLBACK (cb_error), priv);

	g_signal_connect (priv->play_thread, "iterate",
			  G_CALLBACK (check_playtime), priv);

	priv->cdparanoia =
	    gst_element_factory_make ("cdparanoia", "cdparanoia");
	g_assert (priv->cdparanoia != 0);	/* TBD: GError */

	if (priv->cd_device) {
		if (g_object_class_find_property (
				G_OBJECT_GET_CLASS (priv->cdparanoia),
				"device")) {
			g_object_set (G_OBJECT (priv->cdparanoia), "device",
				      priv->cd_device, NULL);
		} else {
			g_object_set (G_OBJECT (priv->cdparanoia), "location",
				      priv->cd_device, NULL);
		}
	}
	g_signal_connect (G_OBJECT (priv->cdparanoia), "eos",
			  G_CALLBACK (eos), priv);

	priv->sector_format = gst_format_get_by_nick ("sector");
	g_assert (priv->sector_format != 0);	/* TBD: GError */
	priv->cdp_pad = gst_element_get_pad (priv->cdparanoia, "src");
	g_assert (priv->cdp_pad != 0);	/* TBD: GError */

	priv->audio_sink = gst_gconf_get_default_audio_sink ();
	g_assert (priv->audio_sink != 0);	/* TBD: GError */

	pthread = gst_element_factory_make ("thread", "outthr");
	queue = gst_element_factory_make ("queue", "q");
	conv = gst_element_factory_make ("audioconvert", "conv");
	scale = gst_element_factory_make ("audioscale", "scale");
	priv->vol_element = vol = gst_element_factory_make ("volume", "vol");

	/* Now build the pipeline */
	gst_bin_add_many (GST_BIN (pthread), queue, conv,
			  scale, vol, priv->audio_sink, NULL);
	gst_bin_add_many (GST_BIN (priv->play_thread), priv->cdparanoia,
			  pthread, NULL);

	/* Link 'er up */
	gst_element_link_many (priv->cdparanoia, queue, conv, scale, vol,
			       priv->audio_sink, NULL);

	pipeline_built = 1;
}


static void
gst_cdparanoia_cdrom_finalize (GObject * object)
{
	GstCdparanoiaCDRom *cdrom = (GstCdparanoiaCDRom *) object;

	if (cdrom->priv->play_thread != NULL) {
		gst_object_unref (GST_OBJECT (cdrom->priv->play_thread));
	}
	if (cdrom->priv->cd_device != NULL) {
		g_free (cdrom->priv->cd_device);
	}
	g_free (cdrom->priv);


	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_cdparanoia_cdrom_open (GstCdparanoiaCDRom * lcd, GError ** error)
{
	return gnome_cdrom_open_dev (GNOME_CDROM (lcd), error);
}

static void
gst_cdparanoia_cdrom_close (GstCdparanoiaCDRom * lcd)
{
	gnome_cdrom_close_dev (GNOME_CDROM (lcd), FALSE);
}

static void
gst_cdparanoia_cdrom_invalidate (GstCdparanoiaCDRom * lcd)
{
	if (lcd->priv->track_info == NULL) {
		g_free (lcd->priv->track_info);
		lcd->priv->track_info = NULL;
	}
}

static void
calculate_track_lengths (GstCdparanoiaCDRom * lcd)
{
	GstCdparanoiaCDRomPrivate *priv;
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
gst_cdparanoia_cdrom_update_cd (GnomeCDRom * cdrom)
{
	GstCdparanoiaCDRom *lcd = GST_CDPARANOIA_CDROM (cdrom);
	GstCdparanoiaCDRomPrivate *priv;
#if defined(__FreeBSD__)
	struct ioc_read_toc_single_entry tocentry;
#elif defined(__NetBSD__) || defined(__OpenBSD__)
	struct ioc_read_toc_entry tocentries;
	struct cd_toc_entry tocentry;
#else
	struct cdrom_tocentry tocentry;
#endif
	int i, j;
	GError *error;

	priv = lcd->priv;

	if (gst_cdparanoia_cdrom_open (lcd, &error) == FALSE) {
		g_warning ("Error opening CD");
		return;
	}

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	if (ioctl (cdrom->fd, CDIOREADTOCHEADER, priv->tochdr) < 0) {
#else
	if (ioctl (cdrom->fd, CDROMREADTOCHDR, priv->tochdr) < 0) {
#endif
		g_warning ("Error reading CD header");
		gst_cdparanoia_cdrom_close (lcd);

		return;
	}

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	priv->track0 = priv->tochdr->starting_track;
	priv->track1 = priv->tochdr->ending_track;
#else
	priv->track0 = priv->tochdr->cdth_trk0;
	priv->track1 = priv->tochdr->cdth_trk1;
#endif
	priv->number_tracks = priv->track1 - priv->track0 + 1;

	gst_cdparanoia_cdrom_invalidate (lcd);
	priv->track_info =
	    g_malloc ((priv->number_tracks +
		       1) * sizeof (GstCdparanoiaCDRomTrackInfo));
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#if defined(__FreeBSD__)
	for (i = 0, j = priv->track0; i < priv->number_tracks; i++, j++) {
		tocentry.track = j;
		tocentry.address_format = CD_MSF_FORMAT;
#else
	tocentries.data_len = sizeof(tocentry);
	tocentries.data = &tocentry;
	for (i = 0, j = priv->track0; i < priv->number_tracks; i++, j++) {
		tocentries.starting_track = j;
		tocentries.address_format = CD_MSF_FORMAT;
#endif

#if defined(__FreeBSD__)
		if (ioctl (cdrom->fd, CDIOREADTOCENTRY, &tocentry) < 0) {
#else
		if (ioctl (cdrom->fd, CDIOREADTOCENTRYS, &tocentries) < 0) {
#endif
			g_warning ("IOCtl failed");
			continue;
		}

		priv->track_info[i].track = j;
#if defined(__FreeBSD__)
		priv->track_info[i].audio_track =
		    tocentry.entry.control != CDROM_DATA_TRACK ? 1 : 0;
		ASSIGN_MSF (priv->track_info[i].address,
			    tocentry.entry.addr.msf);
#else
		priv->track_info[i].audio_track =
		    tocentry.control != CDROM_DATA_TRACK ? 1 : 0;
		ASSIGN_MSF (priv->track_info[i].address, tocentry.addr.msf);
#endif
#else
	for (i = 0, j = priv->track0; i < priv->number_tracks; i++, j++) {
		tocentry.cdte_track = j;
		tocentry.cdte_format = CDROM_MSF;

		if (ioctl (cdrom->fd, CDROMREADTOCENTRY, &tocentry) < 0) {
			g_warning ("IOCtl failed");
			continue;
		}

		priv->track_info[i].track = j;
		priv->track_info[i].audio_track =
		    tocentry.cdte_ctrl != CDROM_DATA_TRACK ? 1 : 0;
		ASSIGN_MSF (priv->track_info[i].address,
			    tocentry.cdte_addr.msf);
#endif
	}
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#if defined(__FreeBSD__)
	tocentry.track = CDROM_LEADOUT;
	tocentry.address_format = CD_MSF_FORMAT;
	if (ioctl (cdrom->fd, CDIOREADTOCENTRY, &tocentry) < 0) {
#else
	tocentries.starting_track = 0xAA;
	tocentries.address_format = CD_MSF_FORMAT;
	if (ioctl (cdrom->fd, CDIOREADTOCENTRYS, &tocentries) < 0) {
#endif
		g_warning ("Error getting leadout");
		gst_cdparanoia_cdrom_invalidate (lcd);
		return;
	}
#if defined(__FreeBSD__)
	ASSIGN_MSF (priv->track_info[priv->number_tracks].address,
		    tocentry.entry.addr.msf);
#else
	ASSIGN_MSF (priv->track_info[priv->number_tracks].address,
		    tocentry.addr.msf);
#endif
#else

	tocentry.cdte_track = CDROM_LEADOUT;
	tocentry.cdte_format = CDROM_MSF;
	if (ioctl (cdrom->fd, CDROMREADTOCENTRY, &tocentry) < 0) {
		g_warning ("Error getting leadout");
		gst_cdparanoia_cdrom_invalidate (lcd);
		return;
	}
	ASSIGN_MSF (priv->track_info[priv->number_tracks].address,
		    tocentry.cdte_addr.msf);
#endif
	calculate_track_lengths (lcd);

	gst_cdparanoia_cdrom_close (lcd);
	return;
}

static gboolean
gst_cdparanoia_cdrom_eject (GnomeCDRom * cdrom, GError ** error)
{
	GstCdparanoiaCDRom *lcd;
	GnomeCDRomStatus *status;

	lcd = GST_CDPARANOIA_CDROM (cdrom);

	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		g_free (status);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_TRAY_OPEN) {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		if (ioctl (cdrom->fd, CDIOCEJECT, 0) < 0) {
#else
		if (ioctl (cdrom->fd, CDROMEJECT, 0) < 0) {
#endif
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_SYSTEM_ERROR,
						      "(eject): ioctl failed: %s",
						      g_strerror (errno));
			}

			g_free (status);
			gst_cdparanoia_cdrom_close (lcd);
			return FALSE;
		}
	} else {
		/* Try to close the tray if it's open */
		if (gnome_cdrom_close_tray (cdrom, error) == FALSE) {

			g_free (status);
			gst_cdparanoia_cdrom_close (lcd);

			return FALSE;
		}
	}

	g_free (status);

	gnome_cdrom_close_dev (cdrom, TRUE);

	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_next (GnomeCDRom * cdrom, GError ** error)
{
	GstCdparanoiaCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF msf, *endmsf;
	int track, end_track;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gst_cdparanoia_cdrom_get_status (cdrom, &status, error) ==
	    FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		gst_cdparanoia_cdrom_close (lcd);
		g_free (status);

		return TRUE;
	}

	track = status->track + 1;
	g_free (status);
	if (track > lcd->priv->number_tracks) {
		/* Do nothing */
		gst_cdparanoia_cdrom_close (lcd);
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

	if (gst_cdparanoia_cdrom_play
	    (cdrom, track, &msf, end_track, endmsf, error) == FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		return FALSE;
	}

	gst_cdparanoia_cdrom_close (lcd);
	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_ffwd (GnomeCDRom * cdrom, GError ** error)
{
	GstCdparanoiaCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF *msf, *endmsf, end;
	int discend, frames, end_track;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gst_cdparanoia_cdrom_get_status (cdrom, &status, error) ==
	    FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		gst_cdparanoia_cdrom_close (lcd);
		g_free (status);

		return TRUE;
	}

	msf = &status->absolute;
	/* Convert MSF to frames to do calculations on it */
	frames = msf_to_frames (msf);
	/* Add a second */
	frames += CD_FRAMES;

	/* Check if we've skipped past the end */
	discend =
	    msf_to_frames (&lcd->priv->
			   track_info[lcd->priv->number_tracks].address);
	if (frames >= discend) {
		/* Do nothing */
		g_free (status);
		gst_cdparanoia_cdrom_close (lcd);
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

	if (gst_cdparanoia_cdrom_play
	    (cdrom, -1, msf, end_track, endmsf, error) == FALSE) {
		g_free (status);
		gst_cdparanoia_cdrom_close (lcd);
		return FALSE;
	}

	g_free (status);
	gst_cdparanoia_cdrom_close (lcd);

	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_play (GnomeCDRom * cdrom,
			   int start_track,
			   GnomeCDRomMSF * start,
			   int finish_track,
			   GnomeCDRomMSF * finish, GError ** error)
{
	GstCdparanoiaCDRom *lcd;
	GstCdparanoiaCDRomPrivate *priv;
	GnomeCDRomStatus *status;
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	struct ioc_play_msf msf;
#else
	struct cdrom_msf msf;
#endif
	gboolean ret;
	guint64 frames;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	priv = lcd->priv;
	g_object_set (G_OBJECT (priv->cdparanoia), "read_speed", 2, NULL);
	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		g_free (status);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		if (status->cd == GNOME_CDROM_STATUS_TRAY_OPEN) {
			if (gst_cdparanoia_cdrom_close_tray (cdrom, error)
			    == FALSE) {
				gst_cdparanoia_cdrom_close (lcd);
				g_free (status);
				return FALSE;
			}
		} else {
			if (error) {
				*error = g_error_new (GNOME_CDROM_ERROR,
						      GNOME_CDROM_ERROR_NOT_READY,
						      "(gst_cdparanoia_cdrom_play): Drive not ready");
			}

			gst_cdparanoia_cdrom_close (lcd);
			g_free (status);
			return FALSE;
		}
	}

	g_free (status);

	/* Get the status again: It might have changed */
	if (gnome_cdrom_get_status (GNOME_CDROM (lcd), &status, error) ==
	    FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		g_free (status);
		return FALSE;
	}
	if (status->cd != GNOME_CDROM_STATUS_OK) {
		/* Stuff if :) */
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_READY,
					      "(gst_cdparanoia_cdrom_play): Drive still not ready");
		}

		gst_cdparanoia_cdrom_close (lcd);
		g_free (status);
		return FALSE;
	}

	switch (status->audio) {
	case GNOME_CDROM_AUDIO_PAUSE:
		if (gnome_cdrom_pause (GNOME_CDROM (lcd), error) == FALSE) {
			g_free (status);
			gst_cdparanoia_cdrom_close (lcd);
			return FALSE;
		}
		break;

	case GNOME_CDROM_AUDIO_NOTHING:
	case GNOME_CDROM_AUDIO_COMPLETE:
	case GNOME_CDROM_AUDIO_STOP:
	case GNOME_CDROM_AUDIO_ERROR:
	default:
		/* Start playing */
		if (start == NULL) {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
			msf.start_m = status->absolute.minute;
			msf.start_s = status->absolute.second;
			msf.start_f =  status->absolute.frame;
#else
			msf.cdmsf_min0 = status->absolute.minute;
			msf.cdmsf_sec0 = status->absolute.second;
			msf.cdmsf_frame0 = status->absolute.frame;
#endif
		} else {
			if (start_track > 0 &&
			    priv && priv->track_info &&
			    start_track <= priv->number_tracks) {
				GnomeCDRomMSF tmpmsf;
				add_msf (&priv->
					 track_info[start_track -
						    1].address, start,
					 &tmpmsf);
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)

				msf.start_m = tmpmsf.minute;
				msf.start_s = tmpmsf.second;
				msf.start_f = tmpmsf.frame;
#else
				msf.cdmsf_min0 = tmpmsf.minute;
				msf.cdmsf_sec0 = tmpmsf.second;
				msf.cdmsf_frame0 = tmpmsf.frame;
#endif
			} else {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
				msf.start_m = start->minute;
				msf.start_s = start->second;
				msf.start_f = start->frame;
#else
				msf.cdmsf_min0 = start->minute;
				msf.cdmsf_sec0 = start->second;
				msf.cdmsf_frame0 = start->frame;
#endif
			}
		}

		if (finish == NULL) {
			if (priv && priv->track_info &&
			    priv->number_tracks > 0) {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
				msf.end_m =
				    priv->track_info[priv->number_tracks].
				    address.minute;
				msf.end_s =
				    priv->track_info[priv->number_tracks].
				    address.second;
				msf.end_f =
				    priv->track_info[priv->number_tracks].
				    address.frame;
#else
				msf.cdmsf_min1 =
				    priv->track_info[priv->number_tracks].
				    address.minute;
				msf.cdmsf_sec1 =
				    priv->track_info[priv->number_tracks].
				    address.second;
				msf.cdmsf_frame1 =
				    priv->track_info[priv->number_tracks].
				    address.frame;
#endif
			} else {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
				msf.end_m = 0;
				msf.end_s = 0;
				msf.end_f = 0;
#else
				msf.cdmsf_min1 = 0;
				msf.cdmsf_sec1 = 0;
				msf.cdmsf_frame1 = 0;
#endif
			}
		} else {
			if (finish_track > 0 &&
			    priv && priv->track_info &&
			    finish_track <= priv->number_tracks) {
				GnomeCDRomMSF tmpmsf;

				add_msf (&priv->
					 track_info[finish_track -
						    1].address, finish,
					 &tmpmsf);
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
				msf.end_m = tmpmsf.minute;
				msf.end_s = tmpmsf.second;
				msf.end_f = tmpmsf.frame;
#else
				msf.cdmsf_min1 = tmpmsf.minute;
				msf.cdmsf_sec1 = tmpmsf.second;
				msf.cdmsf_frame1 = tmpmsf.frame;
#endif
			} else {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
				msf.end_m = finish->minute;
				msf.end_s = finish->second;
				msf.end_f = finish->frame;
#else
				msf.cdmsf_min1 = finish->minute;
				msf.cdmsf_sec1 = finish->second;
				msf.cdmsf_frame1 = finish->frame;
#endif
			}
		}

		/* PLAY IT AGAIN */
		build_pipeline (lcd);
		gst_element_set_state (GST_ELEMENT (priv->play_thread),
				       GST_STATE_PAUSED);

		frames = msf_struct_to_frames (&msf, 0);
		ret = gst_pad_send_event (GST_PAD (priv->cdp_pad),
			gst_event_new_seek (priv->sector_format |
					    GST_SEEK_METHOD_SET |
					    GST_SEEK_FLAG_FLUSH,
					    frames));

		gst_element_set_state (GST_ELEMENT (priv->play_thread),
				       GST_STATE_PLAYING);
		priv->cur_track = start_track;
		priv->cur_abs_frame = frames;
		priv->cur_rel_frame =
		    (start == NULL) ? 0 : msf_to_frames (start);

		priv->check_playtime = 1;
		priv->end_playtime = msf_struct_to_frames (&msf, 1);
	}

	gst_cdparanoia_cdrom_close (lcd);
	g_free (status);
	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_pause (GnomeCDRom * cdrom, GError ** error)
{
	GstCdparanoiaCDRom *lcd;
	GnomeCDRomStatus *status;
	GstCdparanoiaCDRomPrivate *priv;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	priv = lcd->priv;

	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		g_free (status);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_NOT_READY,
					      "(linux_cdrom_pause): Drive not ready");
		}

		g_free (status);
		gst_cdparanoia_cdrom_close (lcd);
		return FALSE;
	}

	if (status->audio == GNOME_CDROM_AUDIO_PAUSE) {
		gst_element_set_state (GST_ELEMENT (priv->play_thread),
				       GST_STATE_PLAYING);
	}

	else if (status->audio == GNOME_CDROM_AUDIO_PLAY) {
		gst_element_set_state (GST_ELEMENT (priv->play_thread),
				       GST_STATE_PAUSED);
	}

	gst_cdparanoia_cdrom_close (lcd);
	g_free (status);
	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_stop (GnomeCDRom * cdrom, GError ** error)
{
	GstCdparanoiaCDRom *lcd;
	GnomeCDRomStatus *status;
	GstCdparanoiaCDRomPrivate *priv;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	priv = lcd->priv;
	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gnome_cdrom_get_status (cdrom, &status, error) == FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		g_free (status);
		return FALSE;
	}
#if 0
	if (status->audio == GNOME_CDROM_AUDIO_PAUSE) {
		if (gst_cdparanoia_cdrom_pause (cdrom, error) == FALSE) {
			gst_cdparanoia_cdrom_close (lcd);
			g_free (status);
			return FALSE;
		}
	}
#endif

	if (priv->play_thread)
		gst_element_set_state (GST_ELEMENT (priv->play_thread),
				       GST_STATE_NULL);

	gst_cdparanoia_cdrom_close (lcd);
	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_rewind (GnomeCDRom * cdrom, GError ** error)
{
	GstCdparanoiaCDRom *lcd;
	GnomeCDRomMSF *msf, tmpmsf, end, *endmsf;
	GnomeCDRomStatus *status;
	int discstart, frames, end_track;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gst_cdparanoia_cdrom_get_status (cdrom, &status, error) ==
	    FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		gst_cdparanoia_cdrom_close (lcd);
		g_free (status);

		return TRUE;
	}

	msf = &status->absolute;

	frames = msf_to_frames (msf);
	frames -= CD_FRAMES;	/* Back one second */

	/* Check we've not run back past the start */
	discstart = msf_to_frames (&lcd->priv->track_info[0].address);
	if (frames < discstart) {
		g_free (status);
		gst_cdparanoia_cdrom_close (lcd);
		return TRUE;
	}

	frames_to_msf (&tmpmsf, frames);
	tmpmsf.frame = 0;	/* Zero the frames */

	if (cdrom->playmode == GNOME_CDROM_WHOLE_CD) {
		end_track = -1;
		endmsf = NULL;
	} else {
		end_track = status->track + 1;
		end.minute = 0;
		end.second = 0;
		end.frame = 0;
		endmsf = &end;
	}

	if (gst_cdparanoia_cdrom_play
	    (cdrom, -1, &tmpmsf, end_track, endmsf, error) == FALSE) {
		g_free (status);

		gst_cdparanoia_cdrom_close (lcd);
		return FALSE;
	}

	gst_cdparanoia_cdrom_close (lcd);
	g_free (status);

	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_back (GnomeCDRom * cdrom, GError ** error)
{
	GstCdparanoiaCDRom *lcd;
	GnomeCDRomStatus *status;
	GnomeCDRomMSF msf, *endmsf;
	int track, end_track;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (gst_cdparanoia_cdrom_get_status (cdrom, &status, error) ==
	    FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		return FALSE;
	}

	if (status->cd != GNOME_CDROM_STATUS_OK) {
		gst_cdparanoia_cdrom_close (lcd);
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
		gst_cdparanoia_cdrom_close (lcd);
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
	if (gst_cdparanoia_cdrom_play
	    (cdrom, track, &msf, end_track, endmsf, error) == FALSE) {
		g_free (status);
		gst_cdparanoia_cdrom_close (lcd);
		return FALSE;
	}

	g_free (status);
	gst_cdparanoia_cdrom_close (lcd);
	return TRUE;
}

/* There should probably be 2 get_status functions: a public one and a private
   one. The private one would get called by the update handler every second,
   and the public one would just return a copy of the status. */
static gboolean
gst_cdparanoia_cdrom_get_status (GnomeCDRom * cdrom,
				 GnomeCDRomStatus ** status,
				 GError ** error)
{
	gdouble vol = 0.0;
	GstCdparanoiaCDRom *lcd;
	GstCdparanoiaCDRomPrivate *priv;
	GnomeCDRomStatus *realstatus;
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	struct cd_sub_channel_position_data subchnl;
#else
	struct cdrom_subchnl subchnl;
#endif
	int cur_gst_status;
	int cd_status;
	guint64 value = 0;
	gboolean ret;

	g_return_val_if_fail (status != NULL, TRUE);

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	priv = lcd->priv;

	*status = g_new0 (GnomeCDRomStatus, 1);
	realstatus = *status;
	realstatus->volume = 0;

	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		gst_cdparanoia_cdrom_close (lcd);
		g_free (realstatus);
		*status = NULL;
		return FALSE;
	}

#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)
	cd_status = ioctl (cdrom->fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
	if (cd_status != -1) {
		switch (cd_status) {
		case CDS_NO_INFO:
			realstatus->cd = GNOME_CDROM_STATUS_NO_DISC;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;

			gst_cdparanoia_cdrom_close (lcd);
			return TRUE;

		case CDS_NO_DISC:
			realstatus->cd = GNOME_CDROM_STATUS_NO_DISC;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;

			gst_cdparanoia_cdrom_close (lcd);
			return TRUE;

		case CDS_TRAY_OPEN:
			realstatus->cd = GNOME_CDROM_STATUS_TRAY_OPEN;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;

			gst_cdparanoia_cdrom_close (lcd);
			return TRUE;

		case CDS_DRIVE_NOT_READY:
			realstatus->cd =
			    GNOME_CDROM_STATUS_DRIVE_NOT_READY;
			realstatus->audio = GNOME_CDROM_AUDIO_NOTHING;
			realstatus->track = -1;

			gst_cdparanoia_cdrom_close (lcd);
			return TRUE;

		default:
			realstatus->cd = GNOME_CDROM_STATUS_OK;
			break;
		}
	} else {
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(gst_cdparanoia_cdrom_get_status): ioctl error %s",
					      g_strerror (errno));
		}

		gst_cdparanoia_cdrom_close (lcd);
		g_free (realstatus);
		*status = NULL;
		return FALSE;
	}
#endif

	gst_cdparanoia_cdrom_close (lcd);

	ASSIGN_MSF (realstatus->relative, blank_msf);
	ASSIGN_MSF (realstatus->absolute, blank_msf);
	ASSIGN_MSF (realstatus->length, blank_msf);


	/* The powerbook CD is slow to update which track it's playing.
	   When you press next, this causes the slider bar to go to the
	   end of the the current track and sit there until a polled update
	   happens that finds the CD caught up. cur_track is an answer to
	   that where we keep better track of what's currently playing than
	   the hardware. */

	build_pipeline (lcd);
	g_object_get (G_OBJECT (priv->vol_element), "volume", &vol, NULL);
	realstatus->volume = lrint (vol * 255.0);
	cur_gst_status =
	    gst_element_get_state (GST_ELEMENT (priv->play_thread));

	if (priv->cdp_pad == NULL
	    || !(cur_gst_status == GST_STATE_PLAYING
		 || cur_gst_status == GST_STATE_PAUSED)) {
		priv->cur_abs_frame = 0;
		priv->cur_rel_frame = 0;
	} else {
		/* figure out where we are in the stream */
		ret =
		    gst_pad_query (priv->cdp_pad, GST_QUERY_POSITION,
				   &priv->sector_format, &value);
		priv->cur_abs_frame = value;
		priv->cur_rel_frame =
		    value -
		    msf_to_frames (&priv->track_info[priv->cur_track - 1].
				   address) + CD_MSF_OFFSET;
	}
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	subchnl.track_number = priv->cur_track;
	frames_to_msf_struct (&subchnl.reladdr,
			      priv->cur_rel_frame);
	frames_to_msf_struct (&subchnl.absaddr,
			      priv->cur_abs_frame);
#else
	subchnl.cdsc_trk = priv->cur_track;
	frames_to_msf_struct (&subchnl.cdsc_reladdr.msf,
			      priv->cur_rel_frame);
	frames_to_msf_struct (&subchnl.cdsc_absaddr.msf,
			      priv->cur_abs_frame);
#endif

	realstatus->track = 1;
	switch (cur_gst_status) {
	case GST_STATE_PLAYING:
		realstatus->audio = GNOME_CDROM_AUDIO_PLAY;
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		ASSIGN_MSF (realstatus->relative,
			    subchnl.reladdr.msf);
		ASSIGN_MSF (realstatus->absolute,
			    subchnl.absaddr.msf);
		realstatus->track = subchnl.track_number;
#else
		ASSIGN_MSF (realstatus->relative,
			    subchnl.cdsc_reladdr.msf);
		ASSIGN_MSF (realstatus->absolute,
			    subchnl.cdsc_absaddr.msf);
		realstatus->track = subchnl.cdsc_trk;
#endif
		if (priv && realstatus->track > 0 &&
		    realstatus->track <= priv->number_tracks) {
			/* track_info may not be initialized */
			ASSIGN_MSF (realstatus->length,
				    priv->track_info[realstatus->track -
						     1].length);
		}
		break;

	case GST_STATE_PAUSED:
		realstatus->audio = GNOME_CDROM_AUDIO_PAUSE;
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		ASSIGN_MSF (realstatus->relative,
			    subchnl.reladdr.msf);
		ASSIGN_MSF (realstatus->absolute,
			    subchnl.absaddr.msf);
		realstatus->track = subchnl.track_number;
#else
		ASSIGN_MSF (realstatus->relative,
			    subchnl.cdsc_reladdr.msf);
		ASSIGN_MSF (realstatus->absolute,
			    subchnl.cdsc_absaddr.msf);
		realstatus->track = subchnl.cdsc_trk;
#endif
		if (priv && realstatus->track > 0 &&
		    realstatus->track <= priv->number_tracks) {
			/* track_info may not be initialized */
			ASSIGN_MSF (realstatus->length,
				    priv->track_info[realstatus->track -
						     1].length);
		}
		break;

	case GST_STATE_NULL:
	case GST_STATE_READY:
		realstatus->audio = GNOME_CDROM_AUDIO_COMPLETE;
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		ASSIGN_MSF (realstatus->relative,
			    subchnl.reladdr.msf);

		ASSIGN_MSF (realstatus->absolute,
			    subchnl.absaddr.msf);
		realstatus->track = subchnl.track_number;
#else
		ASSIGN_MSF (realstatus->relative,
			    subchnl.cdsc_reladdr.msf);
		ASSIGN_MSF (realstatus->absolute,
			    subchnl.cdsc_absaddr.msf);
		realstatus->track = subchnl.cdsc_trk;
#endif
		if (priv && realstatus->track > 0 &&
		    realstatus->track <= priv->number_tracks) {
			/* track_info may not be initialized */
			ASSIGN_MSF (realstatus->length,
				    priv->track_info[realstatus->track -
						     1].length);
		}
		break;

	default:
		realstatus->audio = GNOME_CDROM_AUDIO_ERROR;
		break;
	}

	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_close_tray (GnomeCDRom * cdrom, GError ** error)
{
	GstCdparanoiaCDRom *lcd;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	if (ioctl (cdrom->fd, CDIOCCLOSE) < 0) {
#else
	if (ioctl (cdrom->fd, CDROMCLOSETRAY) < 0) {
#endif
		if (error) {
			*error = g_error_new (GNOME_CDROM_ERROR,
					      GNOME_CDROM_ERROR_SYSTEM_ERROR,
					      "(gst_cdparanoia_cdrom_close_tray): ioctl failed %s",
					      g_strerror (errno));
		}

		gst_cdparanoia_cdrom_close (lcd);
		return FALSE;
	}

	gst_cdparanoia_cdrom_close (lcd);
	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_set_volume (GnomeCDRom * cdrom,
				 int volume, GError ** error)
{
	GstCdparanoiaCDRom *lcd;
	GstCdparanoiaCDRomPrivate *priv;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	priv = lcd->priv;

	g_object_set (G_OBJECT (priv->vol_element), "volume",
		      (gdouble) volume / 255.0, NULL);

	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_is_cdrom_device (GnomeCDRom * cdrom,
				      const char *device, GError ** error)
{
	int fd;
	GstCdparanoiaCDRom *lcd;
	GstCdparanoiaCDRomPrivate *priv;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	priv = lcd->priv;

	if (device == NULL || *device == 0) {
		return FALSE;
	}

	fd = open (device, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		return FALSE;
	}

	/* Fire a harmless ioctl at the device. */
#if defined(__FreeBSD__)
	if (ioctl (fd, CDIOCCAPABILITY, 0) < 0) {
#elif defined(__NetBSD__) || defined(__OpenBSD__)
	if (ioctl (fd, CDIOCGETVOL, 0) < 0) {
#else
	if (ioctl (fd, CDROM_GET_CAPABILITY, 0) < 0) {
#endif
		/* Failed, it's not a CDROM drive */
		close (fd);

		return FALSE;
	}

	close (fd);

	return TRUE;
}

static gboolean
gst_cdparanoia_cdrom_set_device (GnomeCDRom *cdrom,
				 const char *dev,
				 GError **err)
{
	GstCdparanoiaCDRom *lcd;
	GstCdparanoiaCDRomPrivate *priv;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	priv = lcd->priv;

	if (priv->cd_device)
		g_free (priv->cd_device);
	priv->cd_device = g_strdup (dev);
	if (priv->cdparanoia) {
		g_object_set (G_OBJECT (priv->cdparanoia), "location",
			      priv->cd_device, NULL);
	}

	return parent_class->set_device (cdrom, dev, err);
}

static gboolean
gst_cdparanoia_cdrom_get_cddb_data (GnomeCDRom * cdrom,
				    GnomeCDRomCDDBData ** data,
				    GError ** error)
{
	GstCdparanoiaCDRom *lcd;
	GstCdparanoiaCDRomPrivate *priv;
	int i, t = 0, n = 0;

	lcd = GST_CDPARANOIA_CDROM (cdrom);
	priv = lcd->priv;

	if (gst_cdparanoia_cdrom_open (lcd, error) == FALSE) {
		return FALSE;
	}

	if (priv->track_info == NULL) {
		*data = NULL;
		return TRUE;
	}

	*data = g_new0 (GnomeCDRomCDDBData, 1);

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
	(*data)->nsecs =
	    (priv->track_info[priv->track1].address.minute * 60) +
	    priv->track_info[priv->track1].address.second;
	(*data)->offsets = g_new0 (unsigned int, priv->track1 + 1);

	for (i = priv->track0 - 1; i < priv->track1; i++) {
		(*data)->offsets[i] =
		    msf_to_frames (&priv->track_info[i].address);
	}

	gst_cdparanoia_cdrom_close (lcd);
	return TRUE;
}

static void
gst_cdparanoia_cdrom_class_init (GstCdparanoiaCDRomClass * klass)
{
	GObjectClass *object_class;
	GnomeCDRomClass *cdrom_class;

	object_class = G_OBJECT_CLASS (klass);
	cdrom_class = GNOME_CDROM_CLASS (klass);

	object_class->finalize = gst_cdparanoia_cdrom_finalize;

	cdrom_class->eject = gst_cdparanoia_cdrom_eject;
	cdrom_class->next = gst_cdparanoia_cdrom_next;
	cdrom_class->ffwd = gst_cdparanoia_cdrom_ffwd;
	cdrom_class->play = gst_cdparanoia_cdrom_play;
	cdrom_class->pause = gst_cdparanoia_cdrom_pause;
	cdrom_class->stop = gst_cdparanoia_cdrom_stop;
	cdrom_class->rewind = gst_cdparanoia_cdrom_rewind;
	cdrom_class->back = gst_cdparanoia_cdrom_back;
	cdrom_class->get_status = gst_cdparanoia_cdrom_get_status;
	cdrom_class->close_tray = gst_cdparanoia_cdrom_close_tray;
	cdrom_class->set_volume = gst_cdparanoia_cdrom_set_volume;
	cdrom_class->set_device = gst_cdparanoia_cdrom_set_device;
	cdrom_class->is_cdrom_device =
	    gst_cdparanoia_cdrom_is_cdrom_device;
	cdrom_class->update_cd = gst_cdparanoia_cdrom_update_cd;

	/* For CDDB */
	cdrom_class->get_cddb_data = gst_cdparanoia_cdrom_get_cddb_data;

	parent_class = g_type_class_peek_parent (klass);

}

static void
gst_cdparanoia_cdrom_init (GstCdparanoiaCDRom * cdrom)
{
	cdrom->priv = g_new0 (GstCdparanoiaCDRomPrivate, 1);
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	cdrom->priv->tochdr = g_new0 (struct ioc_toc_header, 1);
#else
	cdrom->priv->tochdr = g_new0 (struct cdrom_tochdr, 1);
#endif
	cdrom->priv->track_info = NULL;
	cdrom->priv->cd_device = NULL;
	cdrom->priv->cur_track = 1;
	cdrom->priv->cdp_pad = NULL;
}

/* API */
GType
gst_cdparanoia_cdrom_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (GstCdparanoiaCDRomClass),
			NULL, NULL,
			(GClassInitFunc)
			    gst_cdparanoia_cdrom_class_init, NULL, NULL,
			sizeof (GstCdparanoiaCDRom), 0,
			(GInstanceInitFunc) gst_cdparanoia_cdrom_init,
		};

		type =
		    g_type_register_static (GNOME_CDROM_TYPE,
					    "GstCdparanoiaCDRom", &info,
					    0);
	}

	return type;
}

GnomeCDRom *
gnome_cdrom_new (const char *cdrom_device,
		 GnomeCDRomUpdate update, GError ** error)
{
	return
	    gnome_cdrom_construct (g_object_new
				   (gst_cdparanoia_cdrom_get_type (),
				    NULL), cdrom_device, update,
				   GNOME_CDROM_DEVICE_STATIC, error);
}
