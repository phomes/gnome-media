/*
 * display.c
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#include "gnome-cd.h"
#include "display.h"

#define X_OFFSET 2
#define Y_OFFSET 2

static GtkDrawingAreaClass *parent_class = NULL;

struct _CDDisplayPrivate {
	GdkColor red;
	GdkColor blue;

	GnomeCDText *layout[NUMBER_OF_DISPLAY_LINES];
	int max_width;
	int height;
};

static char *default_text[NUMBER_OF_DISPLAY_LINES] = {
	" ",
	"---------------",
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
	int i;
	
	disp = CD_DISPLAY (drawing_area);
	priv = disp->priv;

	context = pango_layout_get_context (priv->layout[0]->layout);
	base_dir = pango_context_get_base_dir (context);

	for (i = 0; i < NUMBER_OF_DISPLAY_LINES; i++) {
		PangoRectangle rect;

		pango_layout_set_alignment (priv->layout[i]->layout,
					    base_dir == PANGO_DIRECTION_LTR ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);
		pango_layout_set_width (priv->layout[i]->layout, allocation->width * 1000);
		pango_layout_get_extents (priv->layout[i]->layout, NULL, &rect);
		priv->layout[i]->height = rect.height / 1000;
	}

	GTK_WIDGET_CLASS (parent_class)->size_allocate (drawing_area, allocation);
}

static void
size_request (GtkWidget *widget,
	      GtkRequisition *requisition)
{
	CDDisplay *disp;
	CDDisplayPrivate *priv;

	disp = CD_DISPLAY (widget);
	priv = disp->priv;

	requisition->width = priv->max_width;
	requisition->height = priv->height;
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

	/* Cover the area with a black square */
	gdk_draw_rectangle (drawing_area->window,
			    drawing_area->style->black_gc, TRUE,
			    area->x, area->y,
			    area->width, area->height);

	gdk_gc_set_clip_rectangle (drawing_area->style->text_gc[GTK_STATE_NORMAL], area);

	context = pango_layout_get_context (priv->layout[0]->layout);
	base_dir = pango_context_get_base_dir (context);

	for (i = 0; i < NUMBER_OF_DISPLAY_LINES &&
		     height < area->y + area->height + Y_OFFSET; i++) {
		if (height + priv->layout[i]->height >= Y_OFFSET + area->y) {
			pango_layout_set_alignment (priv->layout[i]->layout,
						    base_dir == PANGO_DIRECTION_LTR ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);
			gdk_draw_layout_with_colors (drawing_area->window,
						     drawing_area->style->white_gc,
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

	for (i = 0; i < NUMBER_OF_DISPLAY_LINES; i++) {
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
	gdk_color_alloc (cmap, &disp->priv->red);

	disp->priv->blue.red = 0;
	disp->priv->blue.green = 0;
	disp->priv->blue.blue = 65535;
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

static void
init (CDDisplay *disp)
{
	int i;
	CDDisplayPrivate *priv;

	disp->priv = g_new0 (CDDisplayPrivate, 1);
	priv = disp->priv;

	GTK_WIDGET_UNSET_FLAGS (disp, GTK_NO_WINDOW);

	for (i = 0; i < NUMBER_OF_DISPLAY_LINES; i++) {
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

	gtk_widget_queue_resize (GTK_WIDGET (disp));
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
		     int line,
		     const char *new_str)
{
	CDDisplayPrivate *priv;
	GnomeCDText *text;
	PangoRectangle rect;
	int height, max_width;
	
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
