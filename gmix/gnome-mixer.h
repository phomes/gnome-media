/*
 * gnome_mixer.h
 *
 * Copyright (C) 2001, Iain Holmes
 * Authors: Iain Holmes <iain@ximian.com>
 */

#ifndef __GNOME_MIXER_H__
#define __GNOME_MIXER_H__

#include <glib/gerror.h>
#include <gobject/gobject.h>

#include "gnome-fader.h"

G_BEGIN_DECLS

#define GNOME_MIXER_TYPE (gnome_mixer_get_type ())
#define GNOME_MIXER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_MIXER_TYPE, Gnome_Mixer))
#define GNOME_MIXER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_MIXER_TYPE, Gnome_MixerClass))
#define IS_GNOME_MIXER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_MIXER_TYPE))
#define IS_GNOME_MIXER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_MIXER_TYPE))
#define GNOME_MIXER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_MIXER_TYPE, Gnome_MixerClass))

#define GNOME_MIXER_ERROR gnome_mixer_error_quark ()

typedef enum {
	GNOME_MIXER_ERROR_NOT_IMPLEMENTED,
} Gnome_MixerError;

typedef struct _Gnome_Mixer Gnome_Mixer;
typedef struct _Gnome_MixerPrivate Gnome_MixerPrivate;
typedef struct _GnomeMixerClass GnomeMixerClass;
typedef struct _GnomeMixerContents {
	int number_faders;

	GnomeFader **faders;
} GnomeMixerContents;

struct _GnomeMixer {
	GObject object;

	GnomeMixerPrivate *priv;
};

struct _GnomeMixerClass {
	GObjectClass klass;

	GnomeMixerContents *(* get_contents) (GnomeMixer *mixer,
					      GError **error);
};

GQuark gnome_mixer_error_quark (void);
GType gnome_mixer_get_type (void) G_GNUC_CONST;
GnomeMixerContents *gnome_mixer_get_contents (GnomeMixer *mixer,
					      GError **error);

/* This function is not defined in gnome-mixer.c. */
GnomeMixer *gnome_mixer_new (void);

G_END_DECLS

#endif
