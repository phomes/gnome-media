/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * display.c
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#include <libgnome/gnome-util.h>
#include "gnome-cd.h"
#include "display.h"

#define X_OFFSET 2
#define Y_OFFSET 2

static GtkDrawingAreaClass *parent_class = NULL;

#define LEFT 0
#define RIGHT 1
#define TOP 2
#define BOTTOM 3

#define TOPLEFT 0
#define TOPRIGHT 1
#define BOTTOMLEFT 2
#define BOTTOMRIGHT 3

typedef struct _CDImage {
	GdkPixbuf *pixbuf;
	GdkRectangle rect;
} CDImage;

struct _CDDisplayPrivate {
	GdkColor red;
	GdkColor blue;

	GnomeCDText *layout[CD_DISPLAY_END];
	int max_width;
	int height;
	int need_height;

	CDImage *corners[4];
	CDImage *straights[4];
	CDImage *middle;
};

static char *default_text[CD_DISPLAY_END] = {
	" ",
	" ",
	" ",
	" "
};

static void
free_cd_text (GnomeCDText *text)
{
	g_free (text->text);
	g_object_unref (text->layout);

	g_free (text);
}

static void
size_allocate (GtkWidget *drawing_area,
	       GtkAllocation *allocation)
{
	CDDisplay *disp;
	CDDisplayPrivate *priv;
	PangoContext *context;
	PangoDirection base_dir;
	int i, mod;
	
	disp = CD_DISPLAY (drawing_area);
	priv = disp->priv;

	context = pango_layout_get_context (priv->layout[0]->layout);
	base_dir = pango_context_get_base_dir (context);

	for (i = 0; i < CD_DISPLAY_END; i++) {
		PangoRectangle rect;

		pango_layout_set_alignment (priv->layout[i]->layout,
					    base_dir == PANGO_DIRECTION_LTR ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);
		pango_layout_set_width (priv->layout[i]->layout, allocation->width * 1000);
		pango_layout_get_extents (priv->layout[i]->layout, NULL, &rect);
		priv->layout[i]->height = rect.height / 1000;
	}

	/* Resize and position pixbufs */
	priv->corners[TOPRIGHT]->rect.x = allocation->width - 16;
	priv->corners[BOTTOMLEFT]->rect.y = allocation->height - 16;
	priv->corners[BOTTOMRIGHT]->rect.x = allocation->width - 16;
	priv->corners[BOTTOMRIGHT]->rect.y = allocation->height - 16;

	priv->straights[BOTTOM]->rect.y = allocation->height - 16;
	priv->straights[RIGHT]->rect.x = allocation->width - 16;

	priv->straights[TOP]->rect.width = allocation->width - 32;
	priv->straights[BOTTOM]->rect.width = allocation->width - 32;
	priv->straights[LEFT]->rect.height = allocation->height - 32;
	priv->straights[RIGHT]->rect.height = allocation->height - 32;

	priv->middle->rect.width = allocation->width - 32;
	priv->middle->rect.height = allocation->height - 32;
	
	GTK_WIDGET_CLASS (parent_class)->size_allocate (drawing_area, allocation);
}

static void
size_request (GtkWidget *widget,
	      GtkRequisition *requisition)
{
	CDDisplay *disp;
	CDDisplayPrivate *priv;
	int i, height = 0, width = 0, mod;
	
	disp = CD_DISPLAY (widget);
	priv = disp->priv;

	GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
	
	for (i = 0; i < CD_DISPLAY_END; i++) {
		PangoRectangle rect;
		
		pango_layout_get_extents (priv->layout[i]->layout, NULL, &rect);
		height += (rect.height / 1000);
		
		width = MAX (width, rect.width / 1000);
	}

	requisition->width = width + (2 * X_OFFSET);

	requisition->height = height + (2 * Y_OFFSET);
}

static gboolean
expose_event (GtkWidget *drawing_area,
	      GdkEventExpose *event)
{
	CDDisplay *disp;
	CDDisplayPrivate *priv;
	PangoContext *context;
	PangoDirection base_dir;
	GdkRectangle *area;
	int height = 0;
	int i;

	disp = CD_DISPLAY (drawing_area);
	priv = disp->priv;

	area = &event->area;

	/* Check corners and draw them */
	for (i = 0; i < 4; i++) {
		GdkRectangle inter;
		
		if (gdk_rectangle_intersect (area, &priv->corners[i]->rect, &inter) == TRUE) {
			GdkGC *gc;

			gc = gdk_gc_new (drawing_area->window);
			gdk_pixbuf_render_to_drawable (priv->corners[i]->pixbuf,
						       drawing_area->window, gc,
						       inter.x - priv->corners[i]->rect.x,
						       inter.y - priv->corners[i]->rect.y,
						       inter.x, inter.y,
						       inter.width, inter.height,
						       GDK_RGB_DITHER_NORMAL,
						       0, 0);
			gdk_gc_unref (gc);
		}
	}

	for (i = TOP; i <= BOTTOM; i++) {
		GdkRectangle inter;

		if (gdk_rectangle_intersect (area, &priv->straights[i]->rect, &inter) == TRUE) {
			GdkGC *gc;
			GdkPixbuf *line;
			
			gc = gdk_gc_new (drawing_area->window);

			/* Rescale pixbuf */
			line = gdk_pixbuf_scale_simple (priv->straights[i]->pixbuf,
							priv->straights[i]->rect.width,
							priv->straights[i]->rect.height,
							GDK_INTERP_BILINEAR);

			/* Draw */
			gdk_pixbuf_render_to_drawable (line, drawing_area->window, gc,
						       inter.x - priv->straights[i]->rect.x,
						       inter.y - priv->straights[i]->rect.y,
						       inter.x + 16, inter.y,
						       inter.width, inter.height,
						       GDK_RGB_DITHER_NORMAL,
						       0, 0);

			gdk_pixbuf_unref (line);
			gdk_gc_unref (gc);
		}
	}

	for (i = LEFT; i <= RIGHT; i++) {
		GdkRectangle inter;
		if (gdk_rectangle_intersect (area, &priv->straights[i]->rect, &inter) == TRUE) {
			GdkGC *gc;
			int repeats, extra_s, extra_end, d, j;

			d = inter.y / 16;
			extra_s = inter.y - (d * 16);
			
			gc = gdk_gc_new (drawing_area->window);
			if (extra_s > 0) {
				gdk_pixbuf_render_to_drawable (priv->straights[i]->pixbuf,
							       drawing_area->window, gc,
							       inter.x - priv->straights[i]->rect.x,
							       16 - extra_s,
							       inter.x, inter.y + 16,
							       inter.width, extra_s,
							       GDK_RGB_DITHER_NORMAL,
							       0, 0);
			}

			repeats = (inter.height - extra_s) / 16;
			extra_end = (inter.height - extra_s) % 16;
			
			for (j = 0; j < repeats; j++) {
				gdk_pixbuf_render_to_drawable (priv->straights[i]->pixbuf,
							       drawing_area->window, gc,
							       inter.x - priv->straights[i]->rect.x,
							       0,
							       inter.x,
							       inter.y + 16 + (j * 16) + extra_s,
							       inter.width, 16,
							       GDK_RGB_DITHER_NORMAL,
							       0, 0);
			}

			if (extra_end > 0) {
				gdk_pixbuf_render_to_drawable (priv->straights[i]->pixbuf,
							       drawing_area->window, gc,
							       inter.x - priv->straights[i]->rect.x,
							       0,
							       inter.x,
							       inter.y + 16 + (repeats * 16) + extra_s,
							       inter.width, extra_end,
							       GDK_RGB_DITHER_NORMAL,
							       0, 0);
			}

			gdk_gc_unref (gc);
		}
	}
							       
	/* Do the middle - combination of the above */
	{
		GdkRectangle inter;
		if (gdk_rectangle_intersect (area, &priv->middle->rect, &inter) == TRUE) {
			GdkGC *gc;
			GdkPixbuf *line;
			int repeats, extra_s, extra_end, j, d;

			/* Rescale pixbuf - Keep height at 16 */
			line = gdk_pixbuf_scale_simple (priv->middle->pixbuf,
							priv->middle->rect.width,
							16, GDK_INTERP_BILINEAR);

			d = inter.y / 16;
			extra_s = inter.y - (d * 16);
			
			gc = gdk_gc_new (drawing_area->window);
			if (extra_s > 0) {
				gdk_pixbuf_render_to_drawable (line, drawing_area->window, gc,
							       inter.x - priv->middle->rect.x,
							       16 - extra_s,
							       inter.x + 16, inter.y + 16,
							       inter.width, extra_s,
							       GDK_RGB_DITHER_NORMAL,
							       0, 0);
			}

			repeats = (inter.height - extra_s) / 16;
			extra_end = (inter.height - extra_s) % 16;
			
			for (j = 0; j < repeats; j++) {
				gdk_pixbuf_render_to_drawable (line, drawing_area->window, gc,
							       inter.x - priv->middle->rect.x,
							       0,
							       inter.x + 16,
							       inter.y + 16 + (j * 16) + extra_s,
							       inter.width, 16,
							       GDK_RGB_DITHER_NORMAL,
							       0, 0);
			}

			if (extra_end > 0) {
				gdk_pixbuf_render_to_drawable (line, drawing_area->window, gc,
							       inter.x - priv->middle->rect.x,
							       0,
							       inter.x + 16,
							       inter.y + 16 + (repeats * 16) + extra_s,
							       inter.width, extra_end,
							       GDK_RGB_DITHER_NORMAL,
							       0, 0);
			}

			gdk_pixbuf_unref (line);
			gdk_gc_unref (gc);
		}
	}
	
	gdk_gc_set_clip_rectangle (drawing_area->style->text_gc[GTK_STATE_NORMAL], area);

	context = pango_layout_get_context (priv->layout[0]->layout);
	base_dir = pango_context_get_base_dir (context);

	for (i = 0; i < CD_DISPLAY_END &&
		     height < area->y + area->height + Y_OFFSET; i++) {
		if (height + priv->layout[i]->height >= Y_OFFSET + area->y) {
			pango_layout_set_alignment (priv->layout[i]->layout,
						    base_dir == PANGO_DIRECTION_LTR ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);

			gdk_draw_layout_with_colors (drawing_area->window,
						     drawing_area->style->black_gc,
						     X_OFFSET, 
						     Y_OFFSET + height,
						     priv->layout[i]->layout,
						     &priv->red, NULL);
		}
		height += priv->layout[i]->height;
	}

	gdk_gc_set_clip_rectangle (drawing_area->style->text_gc[GTK_STATE_NORMAL], NULL);

	return TRUE;
}

static void
finalize (GObject *object)
{
	CDDisplay *disp;
	CDDisplayPrivate *priv;
	int i;

	disp = CD_DISPLAY (object);
	priv = disp->priv;

	if (priv == NULL) {
		return;
	}

	for (i = 0; i < CD_DISPLAY_END; i++) {
		free_cd_text (priv->layout[i]);
	}

	g_free (priv);

	disp->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
realize (GtkWidget *widget)
{
	CDDisplay *disp;
	GdkColormap *cmap;

	disp = CD_DISPLAY (widget);

	cmap = gtk_widget_get_colormap (widget);
	disp->priv->red.red = 65535;
	disp->priv->red.green = 0;
	disp->priv->red.blue = 0;
	disp->priv->red.pixel = 0;
	gdk_color_alloc (cmap, &disp->priv->red);

	disp->priv->blue.red = 0;
	disp->priv->blue.green = 0;
	disp->priv->blue.blue = 65535;
	disp->priv->blue.pixel = 0;
	gdk_color_alloc (cmap, &disp->priv->blue);

	GTK_WIDGET_CLASS (parent_class)->realize (widget);
}

static void
class_init (CDDisplayClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS (klass);
	widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = finalize;

	widget_class->size_allocate = size_allocate;
	widget_class->size_request = size_request;
	widget_class->expose_event = expose_event;
  	widget_class->realize = realize;

	parent_class = g_type_class_peek_parent (klass);
}

static CDImage *
cd_image_new (const char *filename,
	      int x,
	      int y)
{
	CDImage *image;
	char *fullname;
	
	image = g_new (CDImage, 1);

	fullname = gnome_pixmap_file (filename);
	if (fullname == NULL) {
		g_warning ("Error loading %s", filename);
		g_free (image);
		return NULL;
	}
	
	image->pixbuf = gdk_pixbuf_new_from_file (fullname, NULL);
	g_free (fullname);
	
	if (image->pixbuf == NULL) {
		g_warning ("Error loading %s", filename);
		g_free (image);
		return NULL;
	}
	
	image->rect.x = 0;
	image->rect.y = 0;
	image->rect.width = gdk_pixbuf_get_width (image->pixbuf);
	image->rect.height = gdk_pixbuf_get_height (image->pixbuf);

	return image;
}

static void
init (CDDisplay *disp)
{
	int i;
	CDDisplayPrivate *priv;

	disp->priv = g_new0 (CDDisplayPrivate, 1);
	priv = disp->priv;

	GTK_WIDGET_UNSET_FLAGS (disp, GTK_NO_WINDOW);

	for (i = 0; i < CD_DISPLAY_END; i++) {
		PangoRectangle rect;
		
		priv->layout[i] = g_new (GnomeCDText, 1);
		
		priv->layout[i]->text = g_strdup (default_text[i]);
		priv->layout[i]->length = strlen (default_text[i]);
		priv->layout[i]->layout = gtk_widget_create_pango_layout (GTK_WIDGET (disp), priv->layout[i]->text);
		pango_layout_set_text (priv->layout[i]->layout,
				       priv->layout[i]->text,
				       priv->layout[i]->length);
		
		pango_layout_get_extents (priv->layout[i]->layout, NULL, &rect);
		priv->layout[i]->height = rect.height / 1000;
		
		priv->max_width = MAX (priv->max_width, rect.width / 1000);
		priv->height += priv->layout[i]->height;
	}

	priv->height += (Y_OFFSET * 2);
	priv->max_width += (X_OFFSET * 2);
	gtk_widget_queue_resize (GTK_WIDGET (disp));

	/* Load pixbufs */
	priv->corners[TOPLEFT] = cd_image_new ("gnome-cd/lcd-theme/top-left.png", 0, 0);
	priv->corners[TOPRIGHT] = cd_image_new ("gnome-cd/lcd-theme/top-right.png", 48, 0);
	priv->corners[BOTTOMLEFT] = cd_image_new ("gnome-cd/lcd-theme/bottom-left.png", 0, 48);
	priv->corners[BOTTOMRIGHT] = cd_image_new ("gnome-cd/lcd-theme/bottom-right.png", 48, 48);

	priv->straights[LEFT] = cd_image_new ("gnome-cd/lcd-theme/middle-left.png", 0, 16);
	priv->straights[RIGHT] = cd_image_new ("gnome-cd/lcd-theme/middle-right.png", 48, 16);
	priv->straights[TOP] = cd_image_new ("gnome-cd/lcd-theme/top.png", 16, 0);
	priv->straights[BOTTOM] = cd_image_new ("gnome-cd/lcd-theme/bottom.png", 16, 48);

	priv->middle = cd_image_new ("gnome-cd/lcd-theme/middle.png", 16, 16);
}

GType
cd_display_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (CDDisplayClass), NULL, NULL,
			(GClassInitFunc) class_init, NULL, NULL,
			sizeof (CDDisplay), 0, (GInstanceInitFunc) init,
		};

		type = g_type_register_static (GTK_TYPE_DRAWING_AREA, "CDDisplay", &info, 0);
	}

	return type;
}

CDDisplay *
cd_display_new (void)
{
	CDDisplay *disp;

	disp = g_object_new (cd_display_get_type (), NULL);
	return CD_DISPLAY (disp);
}

const char *
cd_display_get_line (CDDisplay *disp,
		     int line)
{
	CDDisplayPrivate *priv;
	GnomeCDText *text;

	priv = disp->priv;
	text = priv->layout[line];

	return text->text;
}

void
cd_display_set_line (CDDisplay *disp,
		     CDDisplayLine line,
		     const char *new_str)
{
	CDDisplayPrivate *priv;
	GnomeCDText *text;
	PangoRectangle rect;
	int height;

	g_return_if_fail (disp != NULL);
	g_return_if_fail (new_str != NULL);

	priv = disp->priv;
	
	text = priv->layout[line];
	height = priv->height - text->height;
	
	g_free (text->text);
	text->text = g_strdup (new_str);
	text->length = strlen (new_str);
	pango_layout_set_text (text->layout, text->text, text->length);
	pango_layout_get_extents (text->layout, NULL, &rect);
	text->height = rect.height / 1000;
	
	priv->height = height + text->height;
	priv->max_width = MAX (priv->max_width, rect.width / 1000);

	gtk_widget_queue_resize (GTK_WIDGET (disp));
}

void
cd_display_clear (CDDisplay *disp)
{
	CDDisplayPrivate *priv;
	CDDisplayLine line;
	int height, max_width = 0;

	g_return_if_fail (disp != NULL);

	priv = disp->priv;
	for (line = CD_DISPLAY_LINE_TIME; line < CD_DISPLAY_END; line++) {
		GnomeCDText *text;
		PangoRectangle rect;

		text = priv->layout[line];
		height = priv->height - text->height;

		g_free (text->text);
		text->text = g_strdup (" ");
		text->length = 1;
		pango_layout_set_text (text->layout, text->text, 1);
		pango_layout_get_extents (text->layout, NULL, &rect);
		text->height = rect.height / 1000;

		priv->height = height + text->height;
		max_width = MAX (max_width, rect.width / 1000);
	}

	priv->max_width = max_width;
	gtk_widget_queue_resize (GTK_WIDGET (disp));
}
