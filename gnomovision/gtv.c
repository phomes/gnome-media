#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/videodev.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>
#include <gdk/gdkx.h>
#include <gdk_imlib.h>
#include "gtv.h"

static void gtk_tv_class_init(GtkTVClass *tvclass);
static void gtk_tv_init(GtkTV *tv);
static void gtk_tv_realize(GtkTV *tv);
static void gtk_tv_unrealize(GtkTV *tv);
static void gtk_tv_do_clipping(GtkTV *tv, GdkEvent *event);
static void gtk_tv_destroy(GtkWidget *widget);
static void gtk_tv_map(GtkTV *widget);
static void gtk_tv_unmap(GtkTV *widget);
static void gtk_tv_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void gtk_tv_toplevel_configure(GtkWidget *widget,
				      GdkEventConfigure *event,
				      GtkWidget *tv);
static void gtk_tv_configure_event(GtkWidget *widget,
				   GdkEventConfigure *event);
static void gtk_tv_size_allocate(GtkWidget *widget, GtkAllocation *allocation);

guint
gtk_tv_get_type(void)
{
  static guint tv_type = 0;

  if (!tv_type)
    {
      GtkTypeInfo tv_info =
      {
        "GtkTV",
        sizeof (GtkTV),
        sizeof (GtkTVClass),
        (GtkClassInitFunc) gtk_tv_class_init,
        (GtkObjectInitFunc) gtk_tv_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL,
      };

      tv_type = gtk_type_unique (gtk_widget_get_type (), &tv_info);
    }

  return tv_type;
}

static void
gtk_tv_class_init(GtkTVClass *tvclass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(tvclass);

  GTK_OBJECT_CLASS(tvclass)->destroy = (gpointer)gtk_tv_destroy;
  widget_class->unmap = (gpointer)gtk_tv_unmap;
  widget_class->map = (gpointer)gtk_tv_map;
  widget_class->size_request = (gpointer)gtk_tv_size_request;
  widget_class->size_allocate = (gpointer)gtk_tv_size_allocate;
  widget_class->configure_event = (gpointer)gtk_tv_configure_event;
  widget_class->visibility_notify_event = (gpointer)gtk_tv_do_clipping;
  widget_class->expose_event = (gpointer)gtk_tv_do_clipping;
}

static void
gtk_tv_init(GtkTV *tv)
{
  tv->fd = -1;
  tv->toplevel_config_id = -1;

  GTK_WIDGET_SET_FLAGS (GTK_WIDGET(tv), GTK_NO_WINDOW);
  gtk_widget_set_events(tv, GDK_ALL_EVENTS_MASK);
}

GtkWidget*
gtk_tv_new(int video_num)
{
  char buffer[32];
  GtkTV *retval = GTK_TV(gtk_type_new(gtk_tv_get_type()));
  int major, minor, rwidth, bank, ram; /* For getting the info from DGA */

  if(!XF86DGAQueryVersion(GDK_DISPLAY(), &major, &minor))
    goto new_error_exit;

  XF86DGAGetVideoLL(GDK_DISPLAY(), DefaultScreen(GDK_DISPLAY()),
		    (int *)&(retval->vbuf.base), &rwidth, &bank, &ram);
  retval->vbuf.depth = gdk_visual_get_best_depth();
  retval->vbuf.bytesperline = rwidth * (retval->vbuf.depth >> 3 /* / 8 */);
  gdk_window_get_size(GDK_ROOT_PARENT(),
		      &retval->vbuf.width,
		      &retval->vbuf.height);

  g_snprintf(buffer, sizeof(buffer), "/dev/video%d", video_num);
  retval->fd = open(buffer, O_RDONLY);
  if(retval->fd < 0)
    goto new_error_exit;

  if(ioctl(retval->fd, VIDIOCGCAP, &retval->vcap))
    goto new_error_exit;

  retval->use_shm = FALSE;
  if(!retval->vcap.type & VID_TYPE_OVERLAY)
    {
      if(retval->vcap.type & VID_TYPE_CAPTURE)
	retval->use_shm = TRUE;
      else
	goto new_error_exit;
    }
  else
    {
      if(retval->vcap.type & VID_TYPE_CLIPPING)
	{
	  /* Nothing special to do right now */
	}
      else if(retval->vcap.type & VID_TYPE_CHROMAKEY)
	{
	  /* We want the most vile unused colour we can get
	     for our chroma key - this is pretty foul */
	  gdk_color_parse("#FA77A1", &retval->chroma_colour);
	  gdk_imlib_best_color_get(&retval->chroma_colour);
	}
      else
	retval->kill_on_overlap = 1;

      if(retval->vcap.type & VID_TYPE_FRAMERAM)
	retval->clean_display = 1;
    }

  if(ioctl(retval->fd, VIDIOCSFBUF, &retval->vbuf))
    {
      g_warning("Set frame buffer");
      if(!retval->use_shm)
	goto new_error_exit;
    }

  ioctl(retval->fd, VIDIOCGPICT, &retval->vpic);

  if(retval->use_shm)
    {
      /*
       *	Try 24bit RGB, then 565 then 555. Arguably we ought to
       *	try one matching our visual first, then try them in 
       *	quality order
       *
       */
		
      switch(retval->vcap.type & VID_TYPE_MONOCHROME)
	{
	case VID_TYPE_MONOCHROME:
	  retval->vpic.depth=8;
	  retval->vpic.palette=VIDEO_PALETTE_GREY;	/* 8bit grey */
	  if(!ioctl(retval->fd, VIDIOCSPICT, &retval->vpic))
	    break;

	  retval->vpic.depth=6;
	  if(!ioctl(retval->fd, VIDIOCSPICT, &retval->vpic))
	    break;

	  retval->vpic.depth=4;
	  if(ioctl(retval->fd, VIDIOCSPICT, &retval->vpic))
	    goto new_error_exit;

	  break;
	default:
	  retval->vpic.depth=24;
	  retval->vpic.palette=VIDEO_PALETTE_RGB24;
		
	  if(!ioctl(retval->fd, VIDIOCSPICT, &retval->vpic))
	    break;

	  retval->vpic.palette=VIDEO_PALETTE_RGB565;
	  retval->vpic.depth=16;
				
	  if(!ioctl(retval->fd, VIDIOCSPICT, &retval->vpic))
	    break;

	  retval->vpic.palette=VIDEO_PALETTE_RGB555;
	  retval->vpic.depth=15;
				
	  if(ioctl(retval->fd, VIDIOCSPICT, &retval->vpic))
	    /* Unable to find a supported capture format. */
	    goto new_error_exit;
	  break;
	}

      /* Further stuff for monochrome captures */
      if(retval->vcap.type & VID_TYPE_MONOCHROME)
	{
#ifdef FINISHED
	  if(retval->vpic.depth==4)
	    make_palette_grey(retval, 16);
	  else
	    make_palette_grey(retval, 64);
#endif
	}
    }
  else if(retval->vcap.type & VID_TYPE_FRAMERAM)
    {
      retval->vpic.depth = retval->vbuf.depth;
      switch(retval->vpic.depth)
	{
	case 8:
	  retval->vpic.palette=VIDEO_PALETTE_HI240;	/* colour cube */
	  retval->need_colour_cube=1;
	  break;
	case 15:
	  retval->vpic.palette=VIDEO_PALETTE_RGB555;
	  break;
	case 16:
	  retval->vpic.palette=VIDEO_PALETTE_RGB565;
	  break;
	case 24:
	  retval->vpic.palette=VIDEO_PALETTE_RGB24;
	  break;
	case 32:		
	  retval->vpic.palette=VIDEO_PALETTE_RGB32;
	  break;
	default:
	  g_warning("Unsupported X11 depth.\n");
	}

      if(ioctl(retval->fd, VIDIOCSPICT, &retval->vpic)==-1)
	/* Depth mismatch is fatal on overlay */
	goto new_error_exit;
    }

  if(ioctl(retval->fd, VIDIOCGAUDIO, &retval->vaudio) != -1)
    {
      retval->vaudio.flags |= VIDEO_AUDIO_MUTE;
      ioctl(retval->fd, VIDIOCSAUDIO, &retval->vaudio);
    }

  return GTK_WIDGET(retval);

 new_error_exit:
  g_warning("Error creating TV widget.\n");
  gtk_widget_destroy(GTK_WIDGET(retval));
  return NULL;
}

static void
gtk_tv_destroy(GtkWidget *widget)
{
  close(GTK_TV(widget)->fd);
}

static void
gtk_tv_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  GtkTV *tv = GTK_TV(widget);
  requisition->width = tv->vcap.minwidth;
  requisition->height = tv->vcap.minheight;
  widget->requisition = *requisition;
  g_print("gtk_tv_size_request to %d x %d\n", requisition->width,
	  requisition->height);
}

static void
gtk_tv_size_allocate(GtkWidget *widget,
		     GtkAllocation *allocation)
{
  widget->allocation = *allocation;

  gtk_tv_configure_event(widget, NULL); /* Not exactly kosher, but
					   it's better than doing the
					   same thing twice */
}

static void
gtk_tv_toplevel_relay(GtkWidget *widget,
		      GdkEventConfigure *event,
		      GtkWidget *tv)
{
  switch(event->type)
    {
    case GDK_CONFIGURE:
      gtk_tv_configure_event(tv, event);
      break;
    case GDK_VISIBILITY_NOTIFY:
    case GDK_EXPOSE:
      gtk_tv_do_clipping(GTK_WIDGET(tv), (GdkEvent *)event);
      break;
    }
}

static void
gtk_tv_configure_event(GtkWidget *widget,
		       GdkEventConfigure *event)
{
  GtkWidget *tl = gtk_widget_get_toplevel(widget);
  GtkTV *tv;
  tv = GTK_TV(widget);

  g_return_if_fail(widget->parent != NULL);

  if(!GTK_WIDGET_REALIZED(widget->parent))
    gtk_widget_realize(widget->parent);

  gdk_window_get_origin(widget->parent->window, &tv->vwindow.x, &tv->vwindow.y);
  tv->vwindow.x += widget->allocation.x;
  tv->vwindow.y += widget->allocation.y;
  tv->vwindow.width = MAX(MIN(widget->allocation.width, tv->vcap.maxwidth),
			  tv->vcap.minwidth);
  tv->vwindow.height = MAX(MIN(widget->allocation.height, tv->vcap.maxheight),
			   tv->vcap.minheight);

  if(GTK_WIDGET_VISIBLE(tv))
    ioctl(tv->fd, VIDIOCSWIN, &tv->vwindow);
  g_print("[%d] Window goes to %d, %d x %d %d\n",
	  tv->vwindow.x, tv->vwindow.y,
	  widget->allocation.width,
	  widget->allocation.height);  
}


static void
gtk_tv_map(GtkTV *tv)
{
  g_print("Mapping it in with visible = %d\n", tv->visible);
  if(tv->visible == FALSE)
    {
      tv->visible = TRUE;
      ioctl(tv->fd, VIDIOCSWIN, &tv->vwindow);
      ioctl(tv->fd, VIDIOCCAPTURE, &tv->visible);
    }
}

static void
gtk_tv_unmap(GtkTV *tv)
{
  g_print("Unmapping it with visible = %d\n", tv->visible);
  if(tv->visible == TRUE)
    {
      tv->visible = FALSE;
      ioctl(tv->fd, VIDIOCCAPTURE, &tv->visible);
    }
}

static void gtk_tv_do_clipping(GtkTV *tv, GdkEvent *event)
{
  if(event == NULL)
    return;

  switch(event->type)
    {
    case GDK_VISIBILITY_NOTIFY:
      switch(event->visibility.state)
	{
	case GDK_VISIBILITY_UNOBSCURED:
	  tv->visible = TRUE;
	  tv->vwindow.clipcount = 0;
	  break;
	case GDK_VISIBILITY_PARTIAL:
	  /* Do the dew here */
	  tv->vwindow.clipcount = 0;
	  tv->visible = FALSE;
	  break;
	case GDK_VISIBILITY_FULLY_OBSCURED:
	  tv->visible = FALSE;
	  tv->vwindow.clipcount = 0;
	  break;
	}
      ioctl(tv->fd, VIDIOCCAPTURE, &tv->visible);
    default:
    }
}

guint
gtk_tv_get_num_inputs  (GtkTV *tv)
{
  g_return_if_fail(tv != NULL);
  g_return_if_fail(GTK_IS_TV(tv));
  return (guint)tv->vcap.channels;
}

void
gtk_tv_set_input(GtkTV *tv,
		 guint inputnum)
{
  g_return_if_fail(tv != NULL);
  g_return_if_fail(GTK_IS_TV(tv));
  g_return_if_fail(inputnum < gtk_tv_get_num_inputs(tv));
  g_return_if_fail(ioctl(tv->fd, VIDIOCSCHAN, &inputnum) != -1);
  tv->curinput = inputnum;
}

/* Sets the volume for the current input */
void          
gtk_tv_set_sound(GtkTV *tv,
		 gfloat volume, /* between 0 & 1 */
		 gfloat bass,
		 gfloat treble,
		 gint flags /* VIDEO_AUDIO_* */,
		 gint mode /* VIDEO_MODE_* */)
{
  if(volume >= 0)
    tv->vaudio.volume = (__u16)(volume * 65535);
  if(bass >= 0)
    tv->vaudio.bass = (__u16)(bass * 65535);
  if(treble >= 0)
    tv->vaudio.treble = (__u16)(treble * 65535);
  if(flags != -1)
    tv->vaudio.flags = (__u32)flags;
  if(mode != -1)
    tv->vaudio.mode = (__u16)mode;
  tv->vaudio.audio = tv->curinput;
  g_return_if_fail(ioctl(tv->fd, VIDIOCSAUDIO, &tv->vaudio) != -1);
}

void
gtk_tv_set_toplevel(GtkTV *tv)
{
  g_return_if_fail(tv != NULL);
  g_return_if_fail(GTK_IS_TV(tv));
  if(tv->toplevel_config_id != -1)
    {
      gtk_signal_disconnect(GTK_OBJECT(tv->oldtoplevel),
			    tv->toplevel_config_id);
      gtk_signal_disconnect(GTK_OBJECT(tv->oldtoplevel),
			    tv->toplevel_visibility_id);
    }

  tv->oldtoplevel = gtk_widget_get_toplevel(GTK_WIDGET(tv));
  gtk_widget_set_events(tv->oldtoplevel,
			GDK_ALL_EVENTS_MASK);
  tv->toplevel_visibility_id = gtk_signal_connect(GTK_OBJECT(tv->oldtoplevel),
						  "visibility_notify_event",
						  gtk_tv_toplevel_relay,
						  tv);
  tv->toplevel_config_id = gtk_signal_connect(GTK_OBJECT(tv->oldtoplevel),
					      "configure_event",
					      gtk_tv_toplevel_relay,
					      tv);
}
