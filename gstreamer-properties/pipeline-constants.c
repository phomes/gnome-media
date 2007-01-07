/* -*- mode: c; style: linux -*- */
/* -*- c-basic-offset: 2 -*- */

/* pipeline-constants.c
 * Copyright (C) 2002 Jan Schmidt
 *
 * Written by: Jan Schmidt <thaytan@mad.scientist.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gstreamer-properties-structs.h"
#include <gtk/gtk.h>
#include <bonobo/bonobo-i18n.h>

/* Test specified inputs for pipelines */
/* static const gchar audiosink_test_pipe[] = "afsrc location=\"" TEST_MEDIA_FILE "\""; FIXME*/
static gchar audiosink_test_pipe[] = "audiotestsrc wave=sine freq=512";

/* ffmpegcolorspace is the ripped colorspace element in gst-plugins */
static gchar videosink_test_pipe[] = "videotestsrc";

static gchar GSTPROPS_KEY_DEFAULT_VIDEOSINK[] = "default/videosink";
static gchar GSTPROPS_KEY_DEFAULT_VIDEOSRC[] = "default/videosrc";
static gchar GSTPROPS_KEY_DEFAULT_AUDIOSINK[] = "default/audiosink";
static gchar GSTPROPS_KEY_DEFAULT_AUDIOSRC[] = "default/audiosrc";

extern GSTPPipelineDescription audiosink_pipelines[];
extern GSTPPipelineDescription videosink_pipelines[];
extern GSTPPipelineDescription audiosrc_pipelines[];
extern GSTPPipelineDescription videosrc_pipelines[];

GSTPPipelineDescription audiosink_pipelines[] = {
  {PIPE_TYPE_AUDIOSINK, 0, N_("Autodetect"), "autoaudiosink", FALSE,
      TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
  {PIPE_TYPE_AUDIOSINK, 0, N_"ALSA - Advanced Linux Sound Architecture"),
      "alsasink", FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
#if 0
  {PIPE_TYPE_AUDIOSINK, 0,
        "ALSA - Advanced Linux Sound Architecture (Default Device)",
      "alsasink", FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
  {PIPE_TYPE_AUDIOSINK, 0,
        "ALSA - Advanced Linux Sound Architecture (Sound Card #1 Direct)",
        "alsasink device=hw:0", FALSE, TEST_PIPE_SUPPLIED,
      audiosink_test_pipe, FALSE},
  {PIPE_TYPE_AUDIOSINK, 0,
        "ALSA - Advanced Linux Sound Architecture (Sound Card #1 DMix)",
        "alsasink device=dmix:0", FALSE, TEST_PIPE_SUPPLIED,
      audiosink_test_pipe, FALSE},
#endif
  {PIPE_TYPE_AUDIOSINK, 0, N_("Artsd - ART Sound Daemon"),
      "artsdsink", FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
  {PIPE_TYPE_AUDIOSINK, 0, N_("ESD - Enlightenment Sound Daemon"),
      "esdsink", FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
#if 0                           /* Disabled this until it works */
  {PIPE_TYPE_AUDIOSINK, 0, "Jack", "jackbin.( jacksink )", FALSE,
      TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
#endif
  {PIPE_TYPE_AUDIOSINK, 0, N_("OSS - Open Sound System"),
      "osssink", FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, TRUE},
  {PIPE_TYPE_AUDIOSINK, 0, N_("Pulse - PulseAudio Sound Server"),
      "pulsesink", FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
  {PIPE_TYPE_AUDIOSINK, 0, N_("Custom"), NULL, TRUE, TEST_PIPE_SUPPLIED,
      audiosink_test_pipe, TRUE}
};

GSTPPipelineDescription videosink_pipelines[] = {
  {PIPE_TYPE_VIDEOSINK, 0, N_("Autodetect"), "autovideosink", FALSE,
      TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
#if 0
  /*
   * aasink is disabled because it is not a serious alternative.
   */
  {PIPE_TYPE_VIDEOSINK, 0, "Ascii Art - X11", "aasink driver=0", FALSE,
      TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
  {PIPE_TYPE_VIDEOSINK, 0, "Ascii Art - console", "aasink driver=1", FALSE,
      TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
#endif
#if 0
  /* Leaving this one disabled, because of a bug in cacasink that
   * pops up a window in NULL state
   */
  {PIPE_TYPE_VIDEOSINK, 0, "Colour Ascii Art", "cacasink", FALSE,
      TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
#endif
  {PIPE_TYPE_VIDEOSINK, 0, N_("SDL - Simple DirectMedia Layer"), "sdlvideosink",
      FALSE, TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
  {PIPE_TYPE_VIDEOSINK, 0, N_("X Window System (No Xv)"),
      "ximagesink", FALSE, TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
  {PIPE_TYPE_VIDEOSINK, 0, N_("X Window System (X11/XShm/Xv)", "xvimagesink"), FALSE,
      TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
  {PIPE_TYPE_VIDEOSINK, 0, N_("Custom"), NULL, TRUE, TEST_PIPE_SUPPLIED,
      videosink_test_pipe, TRUE}
};

GSTPPipelineDescription audiosrc_pipelines[] = {
  {PIPE_TYPE_AUDIOSRC, 0, N_("ALSA - Advanced Linux Sound Architecture"),
      "alsasrc", FALSE, TEST_PIPE_AUDIOSINK, NULL, FALSE},
  {PIPE_TYPE_AUDIOSRC, 0, N_("ESD - Enlightenment Sound Daemon", "esdmon"),
      FALSE, TEST_PIPE_AUDIOSINK, NULL, FALSE},
#if 0                           /* Disabled this until it works */
  {PIPE_TYPE_AUDIOSRC, 0, "Jack", "jackbin{ jacksrc }", FALSE,
        TEST_PIPE_AUDIOSINK,
      NULL, FALSE},
#endif
  {PIPE_TYPE_AUDIOSRC, 0, N_("OSS - Open Sound System"), "osssrc", FALSE,
      TEST_PIPE_AUDIOSINK, NULL, FALSE},
  {PIPE_TYPE_AUDIOSRC, 0, N_("Pulse - PulseAudio Sound Server", "pulsesrc"), FALSE,
      TEST_PIPE_AUDIOSINK, NULL, FALSE},
  /* Note: using triangle instead of sine for test sound so we
   * can test the vorbis encoder as well (otherwise it'd compress too well) */
  {PIPE_TYPE_AUDIOSRC, 0, N_("Test Sound"), "audiotestsrc wave=triangle is-live=true", FALSE,
      TEST_PIPE_AUDIOSINK, NULL, FALSE},
  {PIPE_TYPE_AUDIOSRC, 0, N_("Silence"), "audiotestsrc wave=silence is-live=true", FALSE,
      TEST_PIPE_AUDIOSINK, NULL, FALSE},
  {PIPE_TYPE_AUDIOSRC, 0, N_("Custom"), NULL, TRUE, TEST_PIPE_AUDIOSINK, NULL,
      TRUE}
};

GSTPPipelineDescription videosrc_pipelines[] = {
  {PIPE_TYPE_VIDEOSRC, 0, N_("MJPEG (e.g. Zoran v4l device)"), "v4lmjpegsrc", FALSE,
      TEST_PIPE_VIDEOSINK, NULL, FALSE},
  {PIPE_TYPE_VIDEOSRC, 0, N_("QCAM"), "qcamsrc", FALSE, TEST_PIPE_VIDEOSINK,
      NULL, FALSE},
  {PIPE_TYPE_VIDEOSRC, 0, N_("Test Input"), "videotestsrc is-live=true", FALSE,
      TEST_PIPE_VIDEOSINK, NULL, FALSE},
  {PIPE_TYPE_VIDEOSRC, 0, N_("Video for Linux (v4l)"), "v4lsrc", FALSE,
      TEST_PIPE_VIDEOSINK, NULL, FALSE},
  {PIPE_TYPE_VIDEOSRC, 0, N_("Video for Linux 2 (v4l2)"), "v4l2src", FALSE,
      TEST_PIPE_VIDEOSINK, NULL, FALSE},
  {PIPE_TYPE_VIDEOSRC, 0, N_("Custom"), NULL, TRUE, TEST_PIPE_VIDEOSINK, NULL,
      TRUE}
};

GSTPPipelineEditor pipeline_editors[] = {
  /* audiosink pipelines */
  {
        G_N_ELEMENTS (audiosink_pipelines),
        (GSTPPipelineDescription *) (audiosink_pipelines), 0,
        GSTPROPS_KEY_DEFAULT_AUDIOSINK,
        "audiosink_optionmenu", "audiosink_pipeline_entry",
        "audiosink_test_button",
      NULL, NULL, NULL},
  /* videosink pipelines */
  {
        G_N_ELEMENTS (videosink_pipelines),
        (GSTPPipelineDescription *) (videosink_pipelines), 0,
        GSTPROPS_KEY_DEFAULT_VIDEOSINK,
        "videosink_optionmenu", "videosink_pipeline_entry",
        "videosink_test_button",
      NULL, NULL, NULL},
  /* videosrc pipelines */
  {
        G_N_ELEMENTS (videosrc_pipelines),
        (GSTPPipelineDescription *) (videosrc_pipelines), 0,
        GSTPROPS_KEY_DEFAULT_VIDEOSRC,
        "videosrc_optionmenu", "videosrc_pipeline_entry",
        "videosrc_test_button",
      NULL, NULL, NULL},
  /* audiosrc pipelines */
  {
        G_N_ELEMENTS (audiosrc_pipelines),
        (GSTPPipelineDescription *) (audiosrc_pipelines), 0,
        GSTPROPS_KEY_DEFAULT_AUDIOSRC,
        "audiosrc_optionmenu", "audiosrc_pipeline_entry",
        "audiosrc_test_button",
      NULL, NULL, NULL}
};

gint pipeline_editors_count = G_N_ELEMENTS (pipeline_editors);
