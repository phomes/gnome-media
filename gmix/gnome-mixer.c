/*
 * gnome_mixer.c:
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gobject/gobject.h>
#include <glib.h>

#include "gnome-mixer.h"

static GObjectClass *parent_class = NULL;

struct _GnomeMixerPrivate {
	int dummy;
};

GQuark
gnome_mixer_error_quark (void)
{
	static GQuark quark = 0;
	if (quark == 0) {
		quark = g_quark_from_static_string ("gmix-error-quark");
	}

	return quark;
}

static void
finalize (GObject *object)
{
	GnomeMixer *gnome_mixer;

	gnome_mixer = GNOME_MIXER (object);
	if (gnome_mixer->priv == NULL) {
		return;
	}

	g_free (gnome_mixer->priv);
	gnome_mixer->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GnomeMixerContents *
get_contents (GnomeMixer *gnome_mixer,
	      GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_MIXER_ERROR,
				      GNOME_MIXER_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__,
				      G_OBJECT_TYPE_NAME (gnome_mixer));
	}
	
	return NULL;
}

static void
class_init (GnomeMixerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
init (GnomeMixer *mixer)
{
	mixer->priv = g_new (GnomeMixerPrivate, 1);
}

/* API */
GType
gnome_mixer_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (GnomeMixerClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (GnomeMixer), 0, (GInstanceInitFunc) init
		};

		type = g_type_register_static (G_TYPE_OBJECT, "Gnome-Mixer", &info, 0);
	}

	return type;
}

Gnome_MixerContents *
gnome_mixer_get_contents (Gnome_Mixer *mixer,
		    GError **error)
{
	GnomeMixerClass *klass;

	g_return_val_if_fail (IS_GNOME_MIXER (mixer), NULL);
	klass = GNOME_MIXER_GET_CLASS (mixer);
	return klass->get_contents (mixer, error);
}
