/*
 * cdrom.c
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gobject/gobject.h>
#include <glib.h>

#include "cdrom.h"

static GObjectClass *parent_class = NULL;

struct _GnomeCDRomPrivate {
	int dummy;
};

GQuark
gnome_cdrom_error_quark (void)
{
	static GQuark quark = 0;
	if (quark == 0) {
		quark = g_quark_from_static_string ("gnome-cdrom-error-quark");
	}

	return quark;
}

static void
finalize (GObject *object)
{
	GnomeCDRom *cdrom;

	cdrom = GNOME_CDROM (object);
	if (cdrom->priv == NULL) {
		return;
	}

	g_free (cdrom->priv);
	cdrom->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
cdrom_eject (GnomeCDRom *cdrom,
	     GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__, 
				      G_OBJECT_TYPE_NAME (cdrom));
	}

	return FALSE;
}

static gboolean
cdrom_next (GnomeCDRom *cdrom,
	    GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__,
				      G_OBJECT_TYPE_NAME (cdrom));
	}

	return FALSE;

}

static gboolean
cdrom_ffwd (GnomeCDRom *cdrom,
	    GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__,
				      G_OBJECT_TYPE_NAME (cdrom));
	}

	return FALSE;
}

static gboolean
cdrom_play (GnomeCDRom *cdrom,
	    int track,
	    GnomeCDRomMSF *pos,
	    GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__,
				      G_OBJECT_TYPE_NAME (cdrom));
	}

	return FALSE;
}

static gboolean
cdrom_pause (GnomeCDRom *cdrom,
	     GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__,
				      G_OBJECT_TYPE_NAME (cdrom));
	}

	return FALSE;
}

static gboolean
cdrom_stop (GnomeCDRom *cdrom,
	    GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__, 
				      G_OBJECT_TYPE_NAME (cdrom));
	}

	return FALSE;
}

static gboolean
cdrom_rewind (GnomeCDRom *cdrom,
	      GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__,
				      G_OBJECT_TYPE_NAME (cdrom));
	}

	return FALSE;
}

static gboolean
cdrom_back (GnomeCDRom *cdrom,
	    GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__,
				      G_OBJECT_TYPE_NAME (cdrom));
	}

	return FALSE;
}

static gboolean
cdrom_get_status (GnomeCDRom *cdrom,
		  GnomeCDRomStatus **status,
		  GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__,
				      G_OBJECT_TYPE_NAME (cdrom));
	}

	*status = NULL;
	return FALSE;
}

static gboolean
cdrom_close_tray (GnomeCDRom *cdrom,
		  GError **error)
{
	if (error) {
		*error = g_error_new (GNOME_CDROM_ERROR,
				      GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
				      "%s has not been implemented in %s",
				      __FUNCTION__,
				      G_OBJECT_TYPE_NAME (cdrom));
	}

	return FALSE;
}

static void
class_init (GnomeCDRomClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = finalize;

	klass->eject = cdrom_eject;
	klass->next = cdrom_next;
	klass->ffwd = cdrom_ffwd;
	klass->play = cdrom_play;
	klass->pause = cdrom_pause;
	klass->stop = cdrom_stop;
	klass->rewind = cdrom_rewind;
	klass->back = cdrom_back;
	klass->get_status = cdrom_get_status;
	klass->close_tray = cdrom_close_tray;

	parent_class = g_type_class_peek_parent (klass);
}

static void
init (GnomeCDRom *cdrom)
{
	cdrom->priv = g_new (GnomeCDRomPrivate, 1);
}

/* API */
GType
gnome_cdrom_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (GnomeCDRomClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (GnomeCDRom), 0, (GInstanceInitFunc) init,
		};

		type = g_type_register_static (G_TYPE_OBJECT, "GnomeCDRom", &info, 0);
	}

	return type;
}

gboolean
gnome_cdrom_eject (GnomeCDRom *cdrom,
		   GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->eject (cdrom, error);
}

gboolean
gnome_cdrom_next (GnomeCDRom *cdrom,
		  GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->next (cdrom, error);
}

gboolean
gnome_cdrom_fast_forward (GnomeCDRom *cdrom,
			  GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->ffwd (cdrom, error);
}

gboolean
gnome_cdrom_play (GnomeCDRom *cdrom,
		  int track,
		  GnomeCDRomMSF *position,
		  GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->play (cdrom, track, position, error);
}

gboolean
gnome_cdrom_pause (GnomeCDRom *cdrom,
		   GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->pause (cdrom, error);
}

gboolean
gnome_cdrom_stop (GnomeCDRom *cdrom,
		  GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->stop (cdrom, error);
}

gboolean
gnome_cdrom_rewind (GnomeCDRom *cdrom,
		    GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->rewind (cdrom, error);
}

gboolean
gnome_cdrom_back (GnomeCDRom *cdrom,
		  GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->back (cdrom, error);
}

gboolean
gnome_cdrom_get_status (GnomeCDRom *cdrom,
			GnomeCDRomStatus **status,
			GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->get_status (cdrom, status, error);
}

gboolean
gnome_cdrom_close_tray (GnomeCDRom *cdrom,
			GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->close_tray (cdrom, error);
}

