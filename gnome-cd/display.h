/* 
 * display.h
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <glib-object.h>
#include <gtk/gtkdrawingarea.h>

G_BEGIN_DECLS

#define CD_DISPLAY_TYPE (cd_display_get_type ())
#define CD_DISPLAY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CD_DISPLAY_TYPE, CDDisplay))
#define CD_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CD_DISPLAY_TYPE, CDDisplayClass))

typedef enum {
	CD_DISPLAY_LINE_TIME,
	CD_DISPLAY_LINE_INFO,
	CD_DISPLAY_LINE_ARTIST,
	CD_DISPLAY_LINE_ALBUM,
/*  	CD_DISPLAY_LINE_TRACK, */
	CD_DISPLAY_END
} CDDisplayLine;

typedef struct _CDDisplay CDDisplay;
typedef struct _CDDisplayPrivate CDDisplayPrivate;
typedef struct _CDDisplayClass CDDisplayClass;

struct _CDDisplay {
	GtkDrawingArea drawing_area;

	CDDisplayPrivate *priv;
};

struct _CDDisplayClass {
	GtkDrawingAreaClass parent_class;
};

GType cd_display_get_type (void);
CDDisplay *cd_display_new (void);
const char *cd_display_get_line (CDDisplay *disp,
				 int line);
void cd_display_set_line (CDDisplay *disp,
			  CDDisplayLine line,
			  const char *str);
void cd_display_clear (CDDisplay *disp);

G_END_DECLS

#endif

 
