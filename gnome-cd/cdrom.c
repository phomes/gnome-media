/*
 * cdrom.c
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>
#include <glib.h>

#include "cdrom.h"

static GObjectClass *parent_class = NULL;

enum {
	STATUS_CHANGED,
	LAST_SIGNAL
};

static gulong gnome_cdrom_signals[LAST_SIGNAL] = { 0, };

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

static gboolean
cdrom_get_cddb_data (GnomeCDRom *cdrom,
		     GnomeCDRomCDDBData **data,
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

	/* CDDB stuff */
	klass->get_cddb_data = cdrom_get_cddb_data;

	/* Signals */
	gnome_cdrom_signals[STATUS_CHANGED] = g_signal_new ("status-changed",
							    G_TYPE_FROM_CLASS (klass),
							    G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
							    G_STRUCT_OFFSET (GnomeCDRomClass, status_changed),
							    NULL, NULL,
							    g_cclosure_marshal_VOID__POINTER,
							    G_TYPE_NONE,
							    1, G_TYPE_POINTER);
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

gboolean
gnome_cdrom_get_cddb_data (GnomeCDRom *cdrom,
			   GnomeCDRomCDDBData **data,
			   GError **error)
{
	GnomeCDRomClass *klass;

	klass = GNOME_CDROM_GET_CLASS (cdrom);
	return klass->get_cddb_data (cdrom, data, error);
}

void
gnome_cdrom_free_cddb_data (GnomeCDRomCDDBData *data)
{
	g_free (data->offsets);
	g_free (data);
}

void
gnome_cdrom_status_changed (GnomeCDRom *cdrom,
			    GnomeCDRomStatus *new_status)
{
	g_signal_emit (G_OBJECT (cdrom), gnome_cdrom_signals[STATUS_CHANGED], 0, new_status);
}

static gboolean
msf_equals (GnomeCDRomMSF *msf1,
	    GnomeCDRomMSF *msf2)
{
	if (msf1 == msf2) {
		return TRUE;
	}

	if (msf1->minute == msf2->minute &&
	    msf1->second == msf2->second &&
	    msf1->frame == msf2->frame) {
		return TRUE;
	}

	return FALSE;
}

gboolean
gnome_cdrom_status_equal (GnomeCDRomStatus *status1,
			  GnomeCDRomStatus *status2)
{
	if (status1 == status2) {
		return TRUE;
	}

	if (status1->cd != status2->cd) {
		return FALSE;
	}

	if (status1->audio != status2->audio) {
		return FALSE;
	}

	if (status1->track != status2->track) {
		return FALSE;
	}

	if (msf_equals (&status1->relative, &status2->relative) &&
	    msf_equals (&status1->absolute, &status2->absolute)) {
		return TRUE;
	}

	return FALSE;
}


GnomeCDRomStatus *
gnome_cdrom_copy_status (GnomeCDRomStatus *original)
{
	GnomeCDRomStatus *status;

	status = g_new (GnomeCDRomStatus, 1);
	status->cd = original->cd;
	status->audio = original->audio;
	status->track = status->track;

	ASSIGN_MSF (status->relative, original->relative);
	ASSIGN_MSF (status->absolute, original->absolute);

	return status;
}
