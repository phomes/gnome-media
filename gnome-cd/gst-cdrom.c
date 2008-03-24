/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@ximian.com>
 *
 *  Copyright 2002 Iain Holmes
 *  Copyright 2005 Tim-Philipp Müller <tim centricular net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

/*
 * Remaining issues:
 *  - why doesn't it start playing when fired up automatically, even though
 *    that's what is selected in the preferences?
 *  - tray status checking (we should really have an interface for that
 *    in GStreamer, incl. drive locking and eject and things like that)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _ISOC99_SOURCE
#if defined(__FreeBSD_kernel__) && defined(__GLIBC__)
#define _BSD_SOURCE 1
#endif
#include <math.h>
#include <string.h>
#include <glib/gi18n.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <gst/gst.h>
#include <gst/cdda/gstcddabasesrc.h>

#include "gst-cdrom.h"
#include "cddb.h"

#ifdef __linux__
#include <linux/cdrom.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD_kernel__) || defined(__sun)
#include <sys/cdio.h>
#endif
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <sys/cdrio.h>
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
# define GST_CDROM_IOCTL_CDCAPABILITY_REQUEST  CDIOCCAPABILITY
# define GST_CDROM_IOCTL_EJECT_REQUEST         CDIOCEJECT
#elif defined(__NetBSD__) || defined(__OpenBSD__)
# define GST_CDROM_IOCTL_CDCAPABILITY_REQUEST  CDIOCGETVOL
# define GST_CDROM_IOCTL_EJECT_REQUEST         CDIOCEJECT
#else
# define GST_CDROM_IOCTL_CDCAPABILITY_REQUEST  CDROM_GET_CAPABILITY
# define GST_CDROM_IOCTL_EJECT_REQUEST         CDROMEJECT
#endif

GST_DEBUG_CATEGORY_STATIC (gnomecd_debug);
#define GST_CAT_DEFAULT gnomecd_debug

struct _GstCDRomPrivate {
  GstElement          *playbin;
  GstElement          *source; /* e.g. cdparanoia or cddasrc element */

  GnomeCDRomCDDBData  *cddb_data;
  GstTagList          *tags;

  gchar               *device;
  GnomeCDRomStatus     status;
};

static void           gst_cdrom_notify_source_cb (GstCDRom           * cdrom,
                                                  GParamSpec         * pspec,
                                                  gpointer             foo);

static void           gst_cdrom_update_cd        (GnomeCDRom         * cdrom);

static gboolean       gst_cdrom_set_device       (GnomeCDRom         * cdrom,
                                                  const char         * dev,
                                                  GError            ** err);

static gboolean       gst_cdrom_get_status       (GnomeCDRom         * cdrom,
                                                  GnomeCDRomStatus  ** status,
                                                  GError            ** error);

static gboolean       gst_cdrom_eject            (GnomeCDRom         * cdrom,
                                                  GError            ** error);

static gboolean       gst_cdrom_play             (GnomeCDRom         * cdrom,
                                                  int                  start_track,
                                                  GnomeCDRomMSF      * start,
                                                  int                  finish_track,
                                                  GnomeCDRomMSF      * finish,
                                                  GError            ** error);

static gboolean        gst_cdrom_next            (GnomeCDRom         * cdrom,
                                                  GError            ** error);

static gboolean        gst_cdrom_back            (GnomeCDRom         * cdrom,
                                                  GError            ** error);

static gboolean        gst_cdrom_get_cddb_data   (GnomeCDRom          * cdrom,
                                                  GnomeCDRomCDDBData ** data,
                                                  GError             ** error);

static gboolean        gst_cdrom_set_cddb_data_from_tags (GstCDRom  * cdrom,
                                                          GstTagList * tags);


G_DEFINE_TYPE (GstCDRom, gst_cdrom, GNOME_CDROM_TYPE);

static GnomeCDRomMSF blank_msf = { 0, 0, 0 };

static gboolean
nanoseconds_to_msf (gint64 nanoseconds, GnomeCDRomMSF *msf)
{
  gint m = 0, s = 0, f = 0;

  if (nanoseconds >= 0) {
    m = nanoseconds / (GST_SECOND * 60);
    nanoseconds -= m * 60 * GST_SECOND;
    s = nanoseconds / GST_SECOND;
    nanoseconds -= s * GST_SECOND;
    f = nanoseconds / 75;
  }

  if (msf) {
    msf->minute= m;
    msf->second = s;
    msf->frame = f;
  }

  return (nanoseconds >= 0);
}

/* Why do we need this? when a bin changes from READY => NULL state, its
 * bus is set to flushing and we're unlikely to ever see any of its messages
 * if the bin's state reaches NULL before we/the watch in the main thread
 * collects them. That's why we set the state to READY first, process all
 * messages 'manually', and then finally set it to NULL. This makes sure
 * our state-changed handler actually gets to see all the state changes */

static void
gst_cdrom_set_playbin_state_to_null (GstCDRom * cdrom)
{
  GstMessage *msg;
  GstState cur_state, pending;
  GstBus *bus;

  gst_element_get_state (cdrom->priv->playbin, &cur_state, &pending, 0);

  if (cur_state == GST_STATE_NULL && pending == GST_STATE_VOID_PENDING)
    return;

  if (cur_state == GST_STATE_NULL && pending != GST_STATE_VOID_PENDING) {
    gst_element_set_state (cdrom->priv->playbin, GST_STATE_NULL);
    return;
  }

  gst_element_set_state (cdrom->priv->playbin, GST_STATE_READY);
  gst_element_get_state (cdrom->priv->playbin, NULL, NULL, -1);

  bus = gst_element_get_bus (cdrom->priv->playbin);
  while ((msg = gst_bus_pop (bus))) {
    gst_bus_async_signal_func (bus, msg, NULL);
  }
  gst_object_unref (bus);

  gst_element_set_state (cdrom->priv->playbin, GST_STATE_NULL);
  /* maybe we should be paranoid and do _get_state() and check for
   * the return value here, but then errors in shutdown should be
   * rather unlikely */
}

static gboolean
gst_cdrom_is_cdrom_device (GnomeCDRom * cdrom, const char *device,
    GError ** error)
{
  gboolean res = FALSE;

  GST_DEBUG ("device = %s", GST_STR_NULL (device));

  if (device != NULL && *device != '\0') {
    int fd;

    fd = open (device, O_RDONLY | O_NONBLOCK);
    if (fd >= 0) {
#ifdef __sun
      res = TRUE;
#else
      if (ioctl (fd, GST_CDROM_IOCTL_CDCAPABILITY_REQUEST, 0) >= 0) {
        res = TRUE;
      } else {
        GST_DEBUG ("ioctl() failed: %s", g_strerror (errno));
      }
#endif

      close (fd);
    } else {
      GST_DEBUG ("open() failed: %s", g_strerror (errno));
    }
  }

  GST_DEBUG ("%s is%s a CD-ROM device", GST_STR_NULL (device),
      (res) ? "" : " not");

  return res;
}

static gboolean
gst_cdrom_set_volume (GnomeCDRom * gnome_cdrom, int volume, GError ** error)
{
  GstCDRom *cdrom = GST_CDROM (gnome_cdrom);
  gdouble vol;

  vol = CLAMP (volume, 0, 255);
  vol = (gdouble) volume / 255.0;
  g_object_set (cdrom->priv->playbin, "volume", vol, NULL);
  cdrom->priv->status.volume = volume;

  return TRUE;
}

static gboolean
gst_cdrom_pause (GnomeCDRom * gnome_cdrom, GError ** error)
{
  GstCDRom *cdrom = GST_CDROM (gnome_cdrom);
  GstState state, pending_state;
  gboolean res = FALSE;

  gst_element_get_state (cdrom->priv->playbin, &state, &pending_state, 0);

  if (pending_state != GST_STATE_VOID_PENDING)
    state = pending_state;

  GST_DEBUG ("cur_state=%d, pending_state=%d", state, pending_state);

  switch (state) {
    case GST_STATE_NULL:
    case GST_STATE_READY:
      break;
    case GST_STATE_PAUSED:
      /* we are already paused or pausing, nothing to do here */
      res = TRUE;
      break;
    case GST_STATE_PLAYING:
      gst_element_set_state (cdrom->priv->playbin, GST_STATE_PAUSED);
      res = TRUE;
      break;
    default:
      break;
  }

  return res;
}

static gboolean
gst_cdrom_eject (GnomeCDRom * gnome_cdrom, GError ** error)
{
  GnomeCDRomStatus *status = NULL;
  GstCDRom *cdrom;
  gboolean res = FALSE;

  cdrom = GST_CDROM (gnome_cdrom);

  GST_DEBUG ("eject disc");

  gst_cdrom_set_playbin_state_to_null (cdrom);
  gst_element_get_state (cdrom->priv->playbin, NULL, NULL, -1);

  if (!gnome_cdrom_open_dev (gnome_cdrom, error))
    return FALSE;

  if (!gst_cdrom_get_status (gnome_cdrom, &status, error))
    goto done;

  if (status->cd != GNOME_CDROM_STATUS_TRAY_OPEN) {
    if (ioctl (gnome_cdrom->fd, GST_CDROM_IOCTL_EJECT_REQUEST, 0) < 0) {
      GST_DEBUG ("ioctl() failed: %s", g_strerror (errno));
      /* FIXME: try invoking command-line eject utility as fallback? */
      if (error) {
        *error = g_error_new (GNOME_CDROM_ERROR,
                              GNOME_CDROM_ERROR_SYSTEM_ERROR,
                              _("Failed to eject CD: %s"),
                              g_strerror (errno));
      }
      res = FALSE;
    } else {
      res = TRUE;
    }
  } else {
    GST_DEBUG ("tray seems to be open, closing tray ...");
    /* Try to close the tray if it's open */
    /* FIXME: use gst_cdrom_close_tray() directly once implemented */
    res = gnome_cdrom_close_tray (gnome_cdrom, error);
  }

  /* FIXME: update our status in cdrom->priv->status as well? */
done:

  /* force close here so it disconnects the check timeout */
  gnome_cdrom_close_dev (gnome_cdrom, TRUE);
  g_free (status);

  return res;
}

static gboolean
gst_cdrom_next (GnomeCDRom * gnome_cdrom, GError ** error)
{
  GnomeCDRomMSF start_msf, end_msf;
  GstCDRom *cdrom;
  gboolean ret;
  guint cur_track;

  cdrom = GST_CDROM (gnome_cdrom);

  cur_track = cdrom->priv->status.track;

  GST_DEBUG ("cur_track = %d/%d", cur_track, cdrom->priv->cddb_data->ntrks);

  if (cur_track >= cdrom->priv->cddb_data->ntrks)
    return FALSE;

  if (cur_track < 0)
    cur_track = 0;

  start_msf.minute = 0;
  start_msf.second = 0;
  start_msf.frame = 0;

  if (gnome_cdrom->playmode == GNOME_CDROM_WHOLE_CD) {
    ret = gst_cdrom_play (gnome_cdrom, cur_track + 1, &start_msf,
        -1, NULL, error);
  } else {
    /* for clarity ... */
    end_msf.minute = 0;
    end_msf.second = 0;
    end_msf.frame = 0;

    ret = gst_cdrom_play (gnome_cdrom, cur_track + 1, &start_msf,
        cur_track + 2, &end_msf, error);
  }

  return ret;
}

static gboolean
gst_cdrom_back (GnomeCDRom * gnome_cdrom, GError ** error)
{
  GnomeCDRomMSF start_msf, end_msf;
  GstCDRom *cdrom;
  gboolean ret;
  gint cur_track, target_track;

  cdrom = GST_CDROM (gnome_cdrom);

  cur_track = cdrom->priv->status.track;

  GST_DEBUG ("cur_track = %d/%d", cur_track, cdrom->priv->cddb_data->ntrks);

  /* If we're > 0:01 on the track go back to the start 
   * of it, otherwise go to the previous track */
  if (cdrom->priv->status.relative.minute > 0 ||
      (cdrom->priv->status.relative.minute == 0 &&
          cdrom->priv->status.relative.second > 1)) {
    target_track = cur_track;
  } else {
    target_track = cur_track - 1;
  }

  /* can't go before the first track */
  if (target_track < 1)
    target_track = 1;

  nanoseconds_to_msf (0, &start_msf);

  if (gnome_cdrom->playmode == GNOME_CDROM_WHOLE_CD) {
    ret = gst_cdrom_play (gnome_cdrom, target_track, &start_msf,
        -1, NULL, error);
  } else {
    nanoseconds_to_msf (0, &end_msf);

    ret = gst_cdrom_play (gnome_cdrom, target_track, &start_msf,
        CLAMP (target_track + 1, 1, cdrom->priv->cddb_data->ntrks),
        &end_msf, error);
  }

  return ret;
}

static gboolean
gst_cdrom_seek_to_time (GstCDRom * cdrom, guint64 nanoseconds)
{
  GST_DEBUG ("seeking to %" GST_TIME_FORMAT, GST_TIME_ARGS (nanoseconds));

  if (!gst_element_seek (cdrom->priv->playbin, 1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, nanoseconds,
      GST_SEEK_TYPE_NONE, -1)) {
    GST_DEBUG ("seek failed");
    return FALSE;
  }

  /* during the seek playbin will set itself to PAUSED temporarily, and
   * then back to playing. When we were playing, wait for it go back to
   * PLAYING again before doing anything else (but wait no longer than
   * five seconds, just in case something goes wrong) */
  if (cdrom->priv->status.audio != GNOME_CDROM_AUDIO_PAUSE) {
    gst_element_get_state (cdrom->priv->playbin, NULL, NULL, 5 * GST_SECOND);
  }

  return TRUE;
}

static gboolean
gst_cdrom_skip (GstCDRom * cdrom, gint64 diff, GError ** error)
{
  GstFormat fmt = GST_FORMAT_TIME;
  gboolean ret;
  gint64 maxpos, pos = -1;

  if (!gst_element_query_position (cdrom->priv->playbin, &fmt, &pos) || pos < 0) {
    GST_DEBUG ("failed to query position");
    return FALSE;
  }

  if (diff >= 0) {
    GST_DEBUG ("fast forward:  %" GST_TIME_FORMAT " + %" GST_TIME_FORMAT,
       GST_TIME_ARGS (pos), GST_TIME_ARGS (diff));
  } else {
    GST_DEBUG ("fast backward: %" GST_TIME_FORMAT " - %" GST_TIME_FORMAT,
       GST_TIME_ARGS (pos), GST_TIME_ARGS (-diff));
  }

  maxpos = GST_SECOND * (cdrom->priv->status.length.second +
      (60 * cdrom->priv->status.length.minute));

  pos = CLAMP (pos + diff, 0, maxpos);

  if ((ret = gst_cdrom_seek_to_time (cdrom, pos))) {
    nanoseconds_to_msf (pos, &cdrom->priv->status.relative);
  }

  return ret;
}

static gboolean
gst_cdrom_ffwd (GnomeCDRom * gnome_cdrom, GError ** error)
{
  return gst_cdrom_skip (GST_CDROM (gnome_cdrom), GST_SECOND, error);
}

static gboolean
gst_cdrom_rewind (GnomeCDRom * gnome_cdrom, GError ** error)
{
  return gst_cdrom_skip (GST_CDROM (gnome_cdrom), 0 - GST_SECOND, error);
}

static gboolean
gst_cdrom_stop (GnomeCDRom * gnome_cdrom, GError ** error)
{
  GstCDRom *cdrom = GST_CDROM (gnome_cdrom);

  GST_DEBUG ("stopping");

  gst_cdrom_set_playbin_state_to_null (cdrom);

  /* wait until it's finished */
  gst_element_get_state (cdrom->priv->playbin, NULL, NULL, -1);
  return TRUE;
}

static void
gst_cdrom_finalize (GObject * obj)
{
  GstCDRom *cdrom = GST_CDROM (obj);

  if (cdrom->priv->playbin) {
	GstBus *bus;

	bus = gst_element_get_bus (cdrom->priv->playbin);
	gst_bus_set_flushing (bus, TRUE);
	gst_object_unref (bus);

    gst_element_set_state (cdrom->priv->playbin, GST_STATE_NULL);
    gst_object_unref (cdrom->priv->playbin);
  }

  if (cdrom->priv->source) {
    gst_element_set_state (cdrom->priv->source, GST_STATE_NULL);
    gst_object_unref (cdrom->priv->source);
  }

  g_free (cdrom->priv);
  cdrom->priv = NULL;

  G_OBJECT_CLASS (gst_cdrom_parent_class)->finalize (obj);
}

static void
gst_cdrom_init (GstCDRom * cdrom)
{
  cdrom->priv = g_new0 (GstCDRomPrivate, 1);

  /* FIXME: initialize other fields too! */
  cdrom->priv->status.volume = 255; /* equivalent to 1.0 in playbin */
  cdrom->priv->status.length = blank_msf;
  cdrom->priv->status.relative = blank_msf;
  cdrom->priv->status.absolute = blank_msf;
  /*
  cdrom->priv->status.cd = 0;
  cdrom->priv->status.audio = 0;
  */
}

static void
gst_cdrom_class_init (GstCDRomClass * klass)
{
  GnomeCDRomClass *gnomecdrom_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gnomecdrom_class = GNOME_CDROM_CLASS (klass);

  gobject_class->finalize = gst_cdrom_finalize;

#if 0
  gnomecdrom_class->close_tray = gst_cdrom_close_tray;
#endif

  gnomecdrom_class->play = gst_cdrom_play;
  gnomecdrom_class->pause = gst_cdrom_pause;
  gnomecdrom_class->stop = gst_cdrom_stop;
  gnomecdrom_class->next = gst_cdrom_next;
  gnomecdrom_class->back = gst_cdrom_back;
  gnomecdrom_class->ffwd = gst_cdrom_ffwd;
  gnomecdrom_class->rewind = gst_cdrom_rewind;
  gnomecdrom_class->get_status = gst_cdrom_get_status;
  gnomecdrom_class->set_device = gst_cdrom_set_device;
  gnomecdrom_class->update_cd = gst_cdrom_update_cd;
  gnomecdrom_class->set_volume = gst_cdrom_set_volume;
  gnomecdrom_class->is_cdrom_device = gst_cdrom_is_cdrom_device;
  gnomecdrom_class->eject = gst_cdrom_eject;

  /* For CDDB */
  gnomecdrom_class->get_cddb_data = gst_cdrom_get_cddb_data;

  GST_DEBUG_CATEGORY_INIT (gnomecd_debug, "gnome-cd", 0, "gnome-cd");
}

static void
gst_cdrom_error_msg (GstCDRom * cdrom, GstMessage * msg, GstBus * bus)
{
  GError *err = NULL;
  gchar *dbg = NULL;

  gst_message_parse_error (msg, &err, &dbg);
  g_warning ("ERROR: %s", err->message);
  g_error_free (err);
  g_free (dbg);

  cdrom->priv->status.audio = GNOME_CDROM_AUDIO_ERROR;
}

static void
gst_cdrom_eos_msg (GstCDRom * cdrom, GstMessage * msg, GstBus * bus)
{
  gboolean was_last = FALSE;

  if (cdrom->priv->cddb_data) {
    was_last = (cdrom->priv->status.track >= cdrom->priv->cddb_data->ntrks);
  }

  GST_DEBUG ("received EOS %s", (was_last) ? "(last track)" : "");

  if (was_last && GNOME_CDROM (cdrom)->playmode == GNOME_CDROM_SINGLE_TRACK) {
    GnomeCDRomMSF start_msf, end_msf;
    start_msf.minute = 0;
    start_msf.second = 0;
    start_msf.frame = 0;

    end_msf.minute = 0;
    end_msf.second = 0;
    end_msf.frame = 0;

    gst_cdrom_play (GNOME_CDROM (cdrom),
		    cdrom->priv->status.track,
		    &start_msf,
		    cdrom->priv->status.track,
		    &end_msf,
		    NULL);
  } else if (was_last) {
    gst_cdrom_set_playbin_state_to_null (cdrom);
    cdrom->priv->status.audio = GNOME_CDROM_AUDIO_COMPLETE;
  } else {
    if (GNOME_CDROM (cdrom)->playmode == GNOME_CDROM_WHOLE_CD) {
      gst_cdrom_next (GNOME_CDROM (cdrom), NULL);
    } else {
      cdrom->priv->status.audio = GNOME_CDROM_AUDIO_COMPLETE;
    }
  }

}

static void
gst_cdrom_state_change_msg (GstCDRom * cdrom, GstMessage * msg, GstBus * bus)
{
  GstState old_state, new_state, pending_state;

  if (msg->src != GST_OBJECT (cdrom->priv->playbin))
    return;

  gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);

  GST_DEBUG ("playbin state: %s", gst_element_state_get_name (new_state));

  /* Note: we'll never get to see NULL here, as the bush is set to
   * flushing in READY=>NULL */
  switch (new_state) {
    case GST_STATE_VOID_PENDING:
    case GST_STATE_NULL:
      g_return_if_reached ();
    case GST_STATE_READY:
      if (cdrom->priv->status.audio != GNOME_CDROM_AUDIO_STOP)
        cdrom->priv->status.audio = GNOME_CDROM_AUDIO_COMPLETE;
      cdrom->priv->status.track = 1;
      nanoseconds_to_msf (0, &cdrom->priv->status.relative);
      break;
    case GST_STATE_PAUSED:
      cdrom->priv->status.audio = GNOME_CDROM_AUDIO_PAUSE;
      break;
    case GST_STATE_PLAYING:
      cdrom->priv->status.audio = GNOME_CDROM_AUDIO_PLAY;
      break;
  }
}

static void
gst_cdrom_tag_msg (GstCDRom * cdrom, GstMessage * msg, GstBus * bus)
{
  GstTagList *tags = NULL;

  gst_message_parse_tag (msg, &tags);

  if (cdrom->priv->tags) {
    gst_tag_list_free (cdrom->priv->tags);
    cdrom->priv->tags = NULL;
  }

  if (tags) {
    guint64 val = 0;
    guint cur_track;

    GST_DEBUG ("got tags: %" GST_PTR_FORMAT, tags);

    gst_cdrom_set_cddb_data_from_tags (cdrom, tags);

    if (gst_tag_list_get_uint (tags, GST_TAG_TRACK_NUMBER, &cur_track)) {
      cdrom->priv->status.track = cur_track;
    } else {
      g_warning ("Could not retrieve track number from tags");
      cdrom->priv->status.track = -1;
    }      
      
    if (gst_tag_list_get_uint64 (tags, GST_TAG_DURATION, &val) && val > 0) {
      nanoseconds_to_msf (val, &cdrom->priv->status.length);
      GST_DEBUG ("Track duration: %02u:%02u",
          cdrom->priv->status.length.minute,
          cdrom->priv->status.length.second);
    } else {
      g_warning ("Could not retrieve track duration from tags");
      cdrom->priv->status.length = blank_msf;
    }      

    cdrom->priv->tags = tags;
  }
}

GnomeCDRom *
gnome_cdrom_new (const char *cdrom_device,
		 GnomeCDRomUpdate update, GError ** error)
{
  const gchar *cur_element;
  GnomeCDRom *ret;
  GstElement *playbin;
  GstElement *audiosink;
  GstCDRom *gstcdrom;
  GstBus *bus;

  cur_element = "gconfaudiosink";
  if (!gst_default_registry_check_feature_version ("gconfaudiosink",
      GST_VERSION_MAJOR, GST_VERSION_MINOR, 0)) {
    goto element_create_error;
  }

  cur_element = "playbin";
  if (!gst_default_registry_check_feature_version ("playbin",
      GST_VERSION_MAJOR, GST_VERSION_MINOR, 0)) {
    goto element_create_error;
  }

  playbin = gst_element_factory_make ("playbin", "playbin");
  audiosink = gst_element_factory_make ("gconfaudiosink", "gconfaudiosink");

  gstcdrom = g_object_new (GST_TYPE_CDROM, NULL);

  gstcdrom->priv->playbin = playbin;

  g_object_set (playbin, "audio-sink", audiosink, NULL);

  g_signal_connect_swapped (playbin, "notify::source",
                            G_CALLBACK (gst_cdrom_notify_source_cb),
                            gstcdrom);

  bus = gst_element_get_bus (playbin);

  gst_bus_add_signal_watch (bus);

  g_signal_connect_swapped (bus, "message::error",
                            G_CALLBACK (gst_cdrom_error_msg),
                            gstcdrom);

  g_signal_connect_swapped (bus, "message::eos",
                            G_CALLBACK (gst_cdrom_eos_msg),
                            gstcdrom);

  g_signal_connect_swapped (bus, "message::tag",
                            G_CALLBACK (gst_cdrom_tag_msg),
                            gstcdrom);

  g_signal_connect_swapped (bus, "message::state-changed",
                            G_CALLBACK (gst_cdrom_state_change_msg),
                            gstcdrom);

  gst_object_unref (bus);

  ret = gnome_cdrom_construct (GNOME_CDROM (gstcdrom),
                               cdrom_device, update,
                               GNOME_CDROM_DEVICE_STATIC,
                               error);

  return ret;

element_create_error:

  if (error) {
    *error = g_error_new (GNOME_CDROM_ERROR,
                          GNOME_CDROM_ERROR_SYSTEM_ERROR,
                          "Failed to create a GStreamer '%s' element. "
                          "Please fix your GStreamer installation.",
                          cur_element);
  }

  return NULL;
}

static gboolean
object_has_property (gpointer object, const gchar *prop_name)
{
  gpointer klass = G_OBJECT_GET_CLASS (G_OBJECT (object));

  if (g_object_class_find_property (G_OBJECT_CLASS (klass), prop_name))
    return  TRUE;

  return FALSE;
}

static void
gst_cdrom_notify_source_cb (GstCDRom * cdrom, GParamSpec * pspec, gpointer foo)
{
  GstObject *src = NULL;

  g_object_get (cdrom->priv->playbin, "source", &src, NULL);

  gst_object_replace ((GstObject **) &cdrom->priv->source, src);

  if (cdrom->priv->device && object_has_property (src, "device")) {
    g_object_set (src, "device", cdrom->priv->device, NULL);
  }

  if (src) {
    GST_DEBUG ("source changed to %s of type %s", GST_OBJECT_NAME (src),
      g_type_name (G_OBJECT_TYPE (src)));

    if (object_has_property (src, "read-speed")) {
      g_object_set (src, "read-speed", 2, NULL);
    }
  } else {
    GST_DEBUG ("source change to NULL");
  }

  gst_object_unref (src);
}


static gboolean
gst_cdrom_is_open (GstCDRom * cdrom)
{
  GstState cur_state;

  gst_element_get_state (cdrom->priv->playbin, &cur_state, NULL, 0);

  return (cur_state >= GST_STATE_PAUSED);
}

static gboolean
gst_cdrom_ensure_open (GstCDRom * cdrom)
{
  GstStateChangeReturn ret;

  if (gst_cdrom_is_open (cdrom)) {
    GST_DEBUG ("playbin is either paused or playing");
    return TRUE;
  }

  GST_DEBUG ("starting up playbin with track 1");

  g_object_set (cdrom->priv->playbin, "uri", "cdda://1", NULL);

  /* FIXME: do we want PLAYING here? (that would start playing at startup
   * regardless of the set preferences though, but then gnome-cd doesn't
   * seem to call our play vfunc() on its own when that is selected either,
   * so what to do?) */
  gst_element_set_state (cdrom->priv->playbin, GST_STATE_PAUSED);

  ret = gst_element_get_state (cdrom->priv->playbin, NULL, NULL, 10*GST_SECOND);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return FALSE;

  return TRUE;
}

static gboolean
gst_cdrom_set_device (GnomeCDRom *gnome_cdrom, const char *dev, GError **err)
{
  GnomeCDRomClass *klass;
  GstCDRom *cdrom = GST_CDROM (gnome_cdrom);

  g_free (cdrom->priv->device);
  cdrom->priv->device = g_strdup (dev);

  GST_DEBUG ("setting device to '%s'", GST_STR_NULL (dev));

  klass = GNOME_CDROM_CLASS (gst_cdrom_parent_class);
  if (klass->set_device != NULL)
    return klass->set_device (gnome_cdrom, dev, err);

  return TRUE;
}

/* FIXME: see if there's anything else we need to do here */

static void
gst_cdrom_update_cd (GnomeCDRom * gnome_cdrom)
{
  GstCDRom *cdrom = GST_CDROM (gnome_cdrom);

  GST_DEBUG ("updating cd ...");

  if (!gst_cdrom_ensure_open (cdrom)) {
    g_warning ("Error opening CD"); /* FIXME? */
    return;
  }

/*
	GstCdparanoiaCDRom *lcd = GST_CDPARANOIA_CDROM (cdrom);
	GstCdparanoiaCDRomPrivate *priv;
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
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

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD_kernel__)
	if (ioctl (cdrom->fd, CDIOREADTOCHEADER, priv->tochdr) < 0) {
#else
	if (ioctl (cdrom->fd, CDROMREADTOCHDR, priv->tochdr) < 0) {
#endif
		g_warning ("Error reading CD header");
		gst_cdparanoia_cdrom_close (lcd);

		return;
	}

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD_kernel__)
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
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD_kernel__)
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
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

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
		if (ioctl (cdrom->fd, CDIOREADTOCENTRY, &tocentry) < 0) {
#else
		if (ioctl (cdrom->fd, CDIOREADTOCENTRYS, &tocentries) < 0) {
#endif
			g_warning ("IOCtl failed");
			continue;
		}

		priv->track_info[i].track = j;
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
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
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD_kernel__)
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
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
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
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
*/
}

/* FIXME: do tray status checking */

static gboolean
gst_cdrom_get_status (GnomeCDRom * gnome_cdrom, GnomeCDRomStatus ** status,
    GError ** error)
{
  GstFormat fmt;
  GstCDRom *cdrom;
  gint64 pos = -1;

  cdrom = GST_CDROM (gnome_cdrom);

  /* only spin up the disc if we don't know the disc
   * details already (e.g. when stopped on purpose) */
  /* FIXME: make sure we reset cdrom->priv->cddb_data when the tray is open! */
  if (cdrom->priv->cddb_data == NULL) {
    GST_DEBUG ("need to get track details");
    if (!gst_cdrom_ensure_open (cdrom)) {
      *status = NULL;
      GST_DEBUG ("ensure_open failed");
      if (error) {
        /* error message is never shown to user, so no need to translate */
        *error = g_error_new (GNOME_CDROM_ERROR,
                              GNOME_CDROM_ERROR_IO,
                              "Could not open CD.");
      }
      return FALSE;
    }
  }

  *status = g_memdup (&cdrom->priv->status, sizeof (GnomeCDRomStatus));

  if (!gst_cdrom_get_cddb_data (gnome_cdrom, NULL, NULL)) {
    (*status)->cd = GNOME_CDROM_STATUS_NO_DISC;
    (*status)->audio = GNOME_CDROM_AUDIO_NOTHING;
    (*status)->track = -1;
    GST_DEBUG ("get_cddb_data failed, returning NO_DISC, AUDIO_NOTHING");
    return TRUE;
  }

  (*status)->cd = GNOME_CDROM_STATUS_OK;

  fmt = GST_FORMAT_TIME;
  if (gst_element_query_position (cdrom->priv->playbin, &fmt, &pos) && pos != -1) {
    nanoseconds_to_msf (pos, &cdrom->priv->status.relative);
    (*status)->relative = cdrom->priv->status.relative;
    (*status)->absolute = blank_msf; /* dunno what the UI needs that for */
  } else {
    GST_DEBUG ("failed to query current position");
    (*status)->relative = blank_msf;
    (*status)->absolute = blank_msf;
  }

  GST_DEBUG ("[%02u] %02u:%02u / %02u:%02u  cd_status=%d, audio_status=%d "
      "playmode=%d", (*status)->track, (*status)->relative.minute,
      (*status)->relative.second, (*status)->length.minute,
      (*status)->length.second, (*status)->cd, (*status)->audio,
      gnome_cdrom->playmode);

  return TRUE;
}

static gboolean
gst_cdrom_play (GnomeCDRom * gnome_cdrom, int start_track,
    GnomeCDRomMSF * start, int finish_track, GnomeCDRomMSF * finish,
    GError ** error)
{
  GstStateChangeReturn ret;
  GstFormat track_format;
  GstCDRom *cdrom;
  guint64 seek_pos;

  cdrom = GST_CDROM (gnome_cdrom);

  GST_DEBUG ("PLAY track %u at %02u:%02u", start_track, start->minute,
      start->second);

  if (!gst_cdrom_is_open (cdrom)) {
    /* FIXME: maybe call ensure_open() here instead? */
    gst_element_set_state (cdrom->priv->playbin, GST_STATE_PAUSED);
    ret = gst_element_get_state (cdrom->priv->playbin, NULL, NULL, 10 * GST_SECOND);
    if (ret != GST_STATE_CHANGE_SUCCESS) {
      /* this would probably block if the open was still blocking, no? */
      /* gst_element_set_state (cdrom->priv->playbin, GST_STATE_NULL); */
      return FALSE;
    }
  }

  track_format = gst_format_get_by_nick ("track");
  if (!gst_element_seek (cdrom->priv->playbin, 1.0, track_format,
      GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, start_track - 1,
      GST_SEEK_TYPE_NONE, -1)) {
    GST_DEBUG ("seek to track %d failed", start_track);
    return FALSE;
  }

  seek_pos = (start->second + (60 * start->minute)) * GST_SECOND;
  if (!gst_cdrom_seek_to_time (cdrom, seek_pos)) {
    GST_DEBUG ("seek to time %" GST_TIME_FORMAT " failed",
        GST_TIME_ARGS (seek_pos));
    return FALSE;
  }

  gst_element_set_state (cdrom->priv->playbin, GST_STATE_PLAYING);
  return TRUE;
}

static gboolean
gst_cdrom_set_cddb_data_from_tags (GstCDRom * cdrom, GstTagList * tags)
{
  GnomeCDRomCDDBData data = { 0, };
  gchar **arr = NULL;
  gchar *id_str = NULL;
  guint i;

  if (cdrom->priv->cddb_data) {
    g_free (cdrom->priv->cddb_data->offsets);
    g_free (cdrom->priv->cddb_data);
    cdrom->priv->cddb_data = NULL;
  }

  if (tags == NULL)
    return FALSE;

  if (!gst_tag_list_get_string (tags, GST_TAG_CDDA_CDDB_DISCID_FULL, &id_str))
    return FALSE;

  GST_DEBUG ("parsing CDDB ID string '%s'", GST_STR_NULL (id_str));

  if (id_str == NULL)
    return FALSE;

  if (sscanf (id_str, "%" G_GINT32_MODIFIER "x ", &data.discid) != 1)
    goto parse_error;

  GST_DEBUG ("id = %08x", data.discid);

  /* we know strchr() will succeed because of the trailing space above */
  arr = g_strsplit (strchr (id_str, ' ') + 1, " ", -1);
  if (arr == NULL || arr[0] == NULL)
    goto parse_error;

  data.ntrks = atoi (arr[0]);
  if (data.ntrks <= 0 || data.ntrks > 100)
    goto parse_error;

  GST_DEBUG ("num_tracks = %u", data.ntrks);

  if (g_strv_length (arr) != data.ntrks + 2)
    goto parse_error;

  data.nsecs = atoi (arr[data.ntrks + 1]);

  data.offsets = g_new0 (gint, data.ntrks);
  for (i = 1; i <= data.ntrks; ++i) {
    data.offsets[i-1] = atoi (arr[i]);
    GST_DEBUG ("offset[%02u] = %6u", i-1, data.offsets[i-1]);
  }

  cdrom->priv->cddb_data = g_memdup (&data, sizeof (GnomeCDRomCDDBData));

  return TRUE;

parse_error:
  GST_DEBUG ("error parsing CDDB ID string");
  g_free (id_str);
  g_strfreev (arr);
  return FALSE;
}

static gboolean
gst_cdrom_get_cddb_data (GnomeCDRom          * gnome_cdrom,
                         GnomeCDRomCDDBData ** p_cddb_data,
                         GError             ** error)
{
  GnomeCDRomCDDBData data = { 0, };
  GstCDRom *cdrom;
  gint i;

  cdrom = GST_CDROM (gnome_cdrom);

  if (cdrom->priv == NULL || cdrom->priv->cddb_data == NULL)
    return FALSE;

  data.ntrks = cdrom->priv->cddb_data->ntrks;
  data.nsecs = cdrom->priv->cddb_data->nsecs;
  data.discid = cdrom->priv->cddb_data->discid;
 
  data.offsets = g_new (gint, cdrom->priv->cddb_data->ntrks);

  for (i = 0; i < data.ntrks; ++i) {
    data.offsets[i] = cdrom->priv->cddb_data->offsets[i];
  }

  if (p_cddb_data) {
    *p_cddb_data = g_memdup (&data, sizeof (GnomeCDRomCDDBData));
  } else {
    g_free (data.offsets);
  }

  return TRUE;
}
