/*
 * cdrom.h
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifndef __CDROM_H__
#define __CDROM_H__

#include <glib/gerror.h>
#include <gobject/gobject.h>

G_BEGIN_DECLS

#define GNOME_CDROM_TYPE (gnome_cdrom_get_type ())
#define GNOME_CDROM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_CDROM_TYPE, GnomeCDRom))
#define GNOME_CDROM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_CDROM_TYPE, GnomeCDRomClass))
#define IS_GNOME_CDROM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_CDROM_TYPE))
#define IS_GNOME_CDROM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_CDROM_TYPE))
#define GNOME_CDROM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_CDROM_TYPE, GnomeCDRomClass))

#define GNOME_CDROM_ERROR gnome_cdrom_error_quark ()

typedef enum {
	GNOME_CDROM_ERROR_NOT_IMPLEMENTED,
	GNOME_CDROM_ERROR_NOT_OPENED,
	GNOME_CDROM_ERROR_SYSTEM_ERROR,
	GNOME_CDROM_ERROR_IO
} GnomeCDRomError;

typedef struct _GnomeCDRom GnomeCDRom;
typedef struct _GnomeCDRomPrivate GnomeCDRomPrivate;
typedef struct _GnomeCDRomClass GnomeCDRomClass;

typedef enum _GnomeCDRomDriveStatus {
	GNOME_CDROM_STATUS_OK,
	GNOME_CDROM_STATUS_NO_DISC,
	GNOME_CDROM_STATUS_TRAY_OPEN,
	GNOME_CDROM_STATUS_DRIVE_NOT_READY
} GnomeCDRomDriveStatus;

typedef enum _GnomeCDRomAudioStatus {
	GNOME_CDROM_AUDIO_NOTHING,
	GNOME_CDROM_AUDIO_PLAY,
	GNOME_CDROM_AUDIO_PAUSE,
	GNOME_CDROM_AUDIO_COMPLETE,
	GNOME_CDROM_AUDIO_STOP,
	GNOME_CDROM_AUDIO_ERROR
} GnomeCDRomAudioStatus;

typedef struct _GnomeCDRomMSF {
	int minute;
	int second;
	int frame;
} GnomeCDRomMSF;

typedef struct _GnomeCDRomStatus {
	GnomeCDRomDriveStatus cd;
	GnomeCDRomAudioStatus audio;
	int track;
	GnomeCDRomMSF relative;
	GnomeCDRomMSF absolute;
} GnomeCDRomStatus;

struct _GnomeCDRom {
	GObject object;

	GnomeCDRomPrivate *priv;
};

struct _GnomeCDRomClass {
	GObjectClass klass;

	gboolean (*eject) (GnomeCDRom *cdrom,
			   GError **error);
	gboolean (*next) (GnomeCDRom *cdrom,
			  GError **error);
	gboolean (*ffwd) (GnomeCDRom *cdrom,
			  GError **error);
	gboolean (*play) (GnomeCDRom *cdrom,
			  int track,
			  GnomeCDRomMSF *position,
			  GError **error);
	gboolean (*pause) (GnomeCDRom *cdrom,
			   GError **error);
	gboolean (*stop) (GnomeCDRom *cdrom,
			  GError **error);
	gboolean (*rewind) (GnomeCDRom *cdrom,
			    GError **error);
	gboolean (*back) (GnomeCDRom *cdrom,
			  GError **error);
	gboolean (*get_status) (GnomeCDRom *cdrom,
				GnomeCDRomStatus **status,
				GError **error);
	gboolean (*close_tray) (GnomeCDRom *cdrom,
				GError **error);
};

GQuark gnome_cdrom_error_quark (void);

GType gnome_cdrom_get_type (void) G_GNUC_CONST;
gboolean gnome_cdrom_eject (GnomeCDRom *cdrom,
			    GError **error);
gboolean gnome_cdrom_next (GnomeCDRom *cdrom,
			   GError **error);
gboolean gnome_cdrom_fast_forward (GnomeCDRom *cdrom,
				   GError **error);
gboolean gnome_cdrom_play (GnomeCDRom *cdrom,
			   int track,
			   GnomeCDRomMSF *position,
			   GError **error);
gboolean gnome_cdrom_pause (GnomeCDRom *cdrom,
			    GError **error);
gboolean gnome_cdrom_pause (GnomeCDRom *cdrom,
			    GError **error);
gboolean gnome_cdrom_stop (GnomeCDRom *cdrom,
			   GError **error);
gboolean gnome_cdrom_rewind (GnomeCDRom *cdrom,
			     GError **error);
gboolean gnome_cdrom_back (GnomeCDRom *cdrom,
			   GError **error);
gboolean gnome_cdrom_get_status (GnomeCDRom *cdrom,
				 GnomeCDRomStatus **status,
				 GError **error);
gboolean gnome_cdrom_close_tray (GnomeCDRom *cdrom,
				 GError **error);

/* This function is not defined in cdrom.c.
   It is defined in each of the architecture specific files. It is defined here
   so that the main program does not need to know about the architecture
   specific headers */
GnomeCDRom *gnome_cdrom_new (const char *cdrom_device,
			     GError **error);

G_END_DECLS

#endif
