/*
 * gnome-fader.c
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gobject/gobject.h>
#include <glib.h>

#include "gnome-fader.h"

static GObjectClass *parent_class = NULL;

struct _GnomeFaderPrivate {
	GnomeFaderType type;

	double left;
	double right;
} GnomeFaderPrivate;

static void
finalize (GObject *object)
{
	GnomeFader *fader;

	fader = GNOME_FADER (object);
	if (fader->priv == NULL) {
		return;
	}

	g_free (fader->priv);
	fader->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
class_init (GnomeFaderClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = finalize;

	parent_class = g_type_class_peel_parent (klass);
}

static void
init (GnomeFader *fader)
{
	fader->priv = g_new0 (GnomeFaderPrivate, 1);
}

/* API */
GType
gnome_fader_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (GnomeFaderClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (GnomeFader), 0, (GInstanceInitFunc) init
		};

		type = g_type_register_static (G_TYPE_OBJECT, "GnomeFader", &info, 0);
	}

	return type;
}

/**
 * gnome_fader_new:
 */
GnomeFader *
gnome_fader_new (GnomeFaderType type,
		 double initial_left,
		 double initial_right)
{
	GnomeFader *fader;
	GnomeFaderPrivate *priv;

	fader = g_object_new (gnome_fader_get_type (), NULL);
	priv = fader->priv;

	priv->type = type;
	priv->left = initial_left;
	priv->right = initial_right;

	return fader;
}

void
gnome_fader_set_levels (GnomeFader *fader,
			double left,
			double right)
{
	GnomeFaderPrivate *priv;

	g_return_if_fail (IS_GNOME_FADER (fader));
	
	priv = fader->priv;

	if (left > -1) {
		priv->left = left;
	}

	if (right > -1) {
		priv->right = right;
	}
}

void
gnome_fader_get_levels (GnomeFader *fader,
			double *left,
			double *right)
{
	GnomeFaderPrivate *priv;

	g_return_if_fail (IS_GNOME_FADER (fader));

	priv = fader->priv;

	if (left != NULL) {
		*left = priv->left;
	}

	if (right != NULL) {
		*right = priv->right;
	}
}

void
gnome_fader_set_type (GnomeFader *fader,
		      GnomeFaderType type)
{
	g_return_if_fail (IS_GNOME_FADER (fader));

	fader->priv->type = type;
}

GnomeFaderType
gnome_fader_get_type (GnomeFader *fader)
{
	g_return_val_if_fail (IS_GNOME_FADER (fader), -1);
	return fader->priv->type;
}

