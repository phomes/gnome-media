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
#include <gnome.h>
#include <errno.h>
#include <string.h>

static void gtk_tv_class_init(GtkTVClass *tvclass);
static void gtk_tv_init(GtkTV *tv);
static void gtk_tv_do_clipping(GtkTV *tv, GdkEvent *event,
			       gboolean is_our_window);
static void gtk_tv_destroy(GtkWidget *widget);
static void gtk_tv_map(GtkTV *widget);
static void gtk_tv_unmap(GtkTV *widget);
static void gtk_tv_show(GtkTV *widget);
static void gtk_tv_hide(GtkTV *widget);
static void gtk_tv_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void gtk_tv_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static gboolean gtk_tv_rootwin_event(GtkWidget *widget,
				     GdkEvent *event,
				     GtkTV *tv);
static gboolean gtk_tv_toplevel_relay(GtkWidget *widget,
				      GdkEvent *event,
				      GtkWidget *tv);
static void gtk_tv_configure_event(GtkWidget *widget,
				   GdkEventConfigure *event);

GtkWidgetClass *parent_class;

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

  parent_class = gtk_type_parent_class(gtk_tv_get_type());

  GTK_OBJECT_CLASS(tvclass)->destroy = (gpointer)gtk_tv_destroy;
  widget_class->unmap = (gpointer)gtk_tv_unmap;
  widget_class->map = (gpointer)gtk_tv_map;
  widget_class->hide = (gpointer)gtk_tv_hide;
  widget_class->show = (gpointer)gtk_tv_show;
  widget_class->size_request = (gpointer)gtk_tv_size_request;
  widget_class->size_allocate = (gpointer)gtk_tv_size_allocate;
}

static void
gtk_tv_init(GtkTV *tv)
{
  tv->fd = -1;
  tv->toplevel_config_id = -1;
  tv->blocking_events = FALSE;

  GTK_WIDGET_SET_FLAGS (tv, GTK_NO_WINDOW);

  tv->rootwin = gnome_rootwin_new();
  gtk_signal_connect(GTK_OBJECT(tv->rootwin), "event",
		     GTK_SIGNAL_FUNC(gtk_tv_rootwin_event), tv);
  gtk_widget_set_events(tv->rootwin, gtk_widget_get_events(tv->rootwin)
			|GDK_STRUCTURE_MASK
			|GDK_EXPOSURE_MASK
			|GDK_SUBSTRUCTURE_MASK);
  gtk_widget_realize(tv->rootwin);
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

  retval->visible = 0;

  if(ioctl(retval->fd, VIDIOCCAPTURE, &retval->visible))
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

  if(ioctl(retval->fd, VIDIOCGPICT, &retval->vpic))
    {
      g_warning("VIDIOCGPICT\n");
    }

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
      if(ioctl(retval->fd, VIDIOCSAUDIO, &retval->vaudio))
	g_warning("VIDIOCSAUDIO failed\n");
    }

  {
    int i;
    for(i = 0; i < retval->vcap.channels; i++)
      {
	retval->vtuner.tuner = i;
	ioctl(retval->fd, VIDIOCGTUNER, &retval->vtuner);
	retval->vtuner.tuner = i;
	retval->vtuner.mode = VIDEO_MODE_AUTO;
	ioctl(retval->fd, VIDIOCSTUNER, &retval->vtuner);
      }
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
  gtk_widget_destroy(GTK_TV(widget)->rootwin);
  close(GTK_TV(widget)->fd);
}

static void
gtk_tv_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
  GtkTV *tv = GTK_TV(widget);
  requisition->width = tv->vcap.minwidth;
  requisition->height = tv->vcap.minheight;
  widget->requisition = *requisition;
#ifdef DEBUG_GTV
  g_print("gtk_tv_size_request to %d x %d\n", requisition->width,
	  requisition->height);
#endif
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

static gboolean
gtk_tv_toplevel_relay(GtkWidget *widget,
		      GdkEvent *event,
		      GtkWidget *tv)
{
  if(GTK_TV(tv)->blocking_events)
    return FALSE;

  switch(event->type)
    {
    case GDK_EXPOSE:
      gdk_window_clear_area(event->expose.window,
			    event->expose.area.x,
			    event->expose.area.y,
			    event->expose.area.width,
			    event->expose.area.height);
    case GDK_VISIBILITY_NOTIFY:
#ifdef DEBUG_GTV
	g_print("Toplevel got visibility notify\n");
#endif
      gtk_tv_do_clipping(GTK_TV(tv), event, TRUE);
      if(ioctl(GTK_TV(tv)->fd, VIDIOCSWIN, &GTK_TV(tv)->vwindow))
	g_warning("VIDIOCSWIN failed in toplevel_relay visibility\n");
      break;
    case GDK_CONFIGURE:
      gtk_tv_configure_event(GTK_WIDGET(tv), (GdkEventConfigure *)event);
      return FALSE;
      break;
    default:
#ifdef DEBUG_GTV
      g_print("toplevel: Don't know how to handle event %d\n", event->type);
#endif
      return FALSE;
    }

  return TRUE;
}

static gboolean
gtk_tv_rootwin_event(GtkWidget *widget,
		     GdkEvent *event,
		     GtkTV *tv)
{
  if(tv->blocking_events)
    return FALSE;

  switch(event->type)
    {
    case GDK_MAP:
    case GDK_UNMAP:
    case GDK_DESTROY:
      gtk_tv_do_clipping(GTK_TV(tv), (GdkEvent *)event, FALSE);
      if(ioctl(GTK_TV(tv)->fd, VIDIOCSWIN, &GTK_TV(tv)->vwindow))
	g_warning("VIDIOCSWIN failed in rootwin_event\n");
      break;
    case GDK_CONFIGURE:
      {
	GdkRectangle r;
	gint x, y;

	gdk_window_get_origin(tv->oldtoplevel->window,
			      &x, &y);
	r.x = x; r.y = y;
	gdk_window_get_size(tv->oldtoplevel->window,
			    &x, &y);
	r.width = x; r.height = y;

	r.x = MAX(event->configure.x, r.x);
	r.y = MAX(event->configure.y, r.y);
	r.width = MAX(event->configure.x + event->configure.width,
		      r.x + r.width) - r.x;
	r.height = MAX(event->configure.y + event->configure.height,
		       r.y + r.height) - r.y;

	r.x-=20; r.y-=20; r.width+=20; r.height+=20;

#ifdef DEBUG_GTV
	g_print("Clearing area (%d, %d) +%d +%d\n",
		(int)r.x, (int)r.y, (int)r.width, (int)r.height);
#endif
	gdk_window_clear_area(GDK_ROOT_PARENT(),
			      r.x, r.y, r.width, r.height);
      }
      gtk_tv_do_clipping(GTK_TV(tv), (GdkEvent *)event, FALSE);
      gtk_tv_configure_event(GTK_WIDGET(tv), (GdkEventConfigure *)event);
      break;
    case GDK_EXPOSE:
      /* xrefresh in miniature ;-) */
      gdk_window_clear_area(event->expose.window,
			    event->expose.area.x,
			    event->expose.area.y,
			    event->expose.area.width,
			    event->expose.area.height);
      break;
    default:
#ifdef DEBUG_GTV
      g_print("rootwin: Don't know how to handle event %d\n", event->type);
#endif
      return FALSE;
    }

  return TRUE;
}

static void
gtk_tv_configure_event(GtkWidget *widget,
		       GdkEventConfigure *event)
{
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

  if(ioctl(tv->fd, VIDIOCSWIN, &tv->vwindow))
    g_warning("VIDIOCSWIN failed in configure_event\n");
}

static void
gtk_tv_show(GtkTV *tv)
{
  tv->blocking_events = 0;
  if(parent_class->show)
    parent_class->show(tv);
}

static void
gtk_tv_hide(GtkTV *tv)
{
  tv->blocking_events = 1;
  if(parent_class->hide)
    parent_class->hide(tv);
}

static void
gtk_tv_map(GtkTV *tv)
{
  if(tv->visible == FALSE)
    {
#ifdef DEBUG_GTV
      g_print("Showing TV\n");
#endif
      tv->visible = TRUE;
      if(ioctl(tv->fd, VIDIOCSWIN, &tv->vwindow))
	g_warning("VIDIOCSWIN failed in map\n");
      if(ioctl(tv->fd, VIDIOCCAPTURE, &tv->visible))
	g_warning("VIDIOCCAPTURE failed in map\n");
    }
  if(parent_class->map)
    parent_class->map(tv);
}

static void
gtk_tv_unmap(GtkTV *tv)
{
  if(tv->visible == TRUE)
    {
#ifdef DEBUG_GTV
      g_print("Showing TV\n");
#endif
      tv->visible = FALSE;
      if(ioctl(tv->fd, VIDIOCCAPTURE, &tv->visible))
	g_warning("VIDIOCCAPTURE failed in unmap\n");
    }
  if(parent_class->unmap)
    parent_class->unmap(tv);
}

/* Copied almost directly from xtvscreen */
static int getclip(GtkTV *w)
{
  int ncr=0;
  int x,y,x2,y2,wx,wy;
  uint ww,wh;
  struct video_window vwin;
  Display *disp;
  XWindowAttributes wts;
  Window parent,win,parent2, root, rroot, *children, swin;
  uint nchildren, i;
  struct video_clip *cr; 
  GtkWidget *widpar;

#ifdef DEBUG_GTV
  g_print("getclip()\n");
#endif

  if (!GTK_WIDGET_REALIZED(w))
    return;

  disp=GDK_DISPLAY();
  vwin=w->vwindow;

  widpar=(GtkWidget *) w;
  while(widpar->parent) widpar=widpar->parent;
  if((win=GDK_WINDOW_XWINDOW(widpar->window))==None) return;

  XQueryTree(disp,win, &rroot, &parent, &children, &nchildren);
  if(nchildren) XFree((char *)children);

  cr=w->clips;

  wx=vwin.x; wy=vwin.y;
  wh=vwin.height; ww=vwin.width;
    
  root=DefaultRootWindow(disp);
  swin=win;
  while (parent!=root) {
    swin=parent;
    XQueryTree(disp, swin, &rroot, &parent, &children, &nchildren);
    if (nchildren)
      XFree((char *) children);
  }
  XQueryTree(disp, root, &rroot, &parent2, &children, &nchildren);
  for (i=0; i<nchildren;i++) 
    if (children[i]==swin) break;
  i++;
  for (; i<nchildren; i++) {
    
    XGetWindowAttributes(disp, children[i], &wts);
    if (!(wts.map_state & IsViewable))  continue;
    
    x=wts.x-vwin.x; y=wts.y-vwin.y;
    x2=x+wts.width+2*wts.border_width-1;
    y2=y+wts.height+2*wts.border_width-1;
    if ((x2 < 0) || (x >= (int)ww) ||
	(y2 < 0) || (y >= (int)wh))
      continue;

    if (x<0)
      x=0;
    if (y<0)
      y=0;
    if (x2>=(int)ww)
      x2=ww-1;
    if (y2>=(int)wh)
      y2=wh-1;
#ifdef DEBUG_GTV
    g_print("Adding clip #%d at (%d, %d) + %dx%d)\n",
	    ncr, x, y, x2-x, y2-y);
#endif
    cr[ncr].x = x;
    cr[ncr].y = y;
    cr[ncr].width = x2-x;
    cr[ncr++].height = y2-y;
  }
  XFree((char *) children);

  w->vwindow.clipcount = ncr;
  w->vwindow.clips = w->clips;
  
  return ncr;
}

#ifdef X11_IS_PERFECT /* until that day, I think you'll just have to
			 live w/o clipping ;-) */

/* This needs improvement - it should know how to add 
   more clipping area(s) if the overlap between the exposed area
   and clipping area(s) is not perfect */

static void
gtk_tv_clipping_expose(GtkTV *tv,
		       GdkEventExpose *event)
{
  int i;
  GdkRegion *rclip, *rempty, *rexpose, *rtmp, *rtmp2, *rtmp3;
  GdkRectangle rect, area = event->area;
  
  rempty = gdk_region_new();

  rexpose = gdk_region_union_with_rect(rempty, &area);


  for(i = 0; i < tv->vwindow.clipcount; i++)
    {
      rect.x = tv->clips[i].x;
      rect.y = tv->clips[i].y;
      rect.width = tv->clips[i].width;
      rect.height = tv->clips[i].height;

      rtmp = gdk_region_union_with_rect(rempty, &rect);

      if(gdk_region_rect_in(rtmp, &area) != GDK_OVERLAP_RECTANGLE_OUT)
	{
	  rtmp2 = gdk_region_union_with_rect(rempty, &area);
	  rtmp3 = gdk_regions_subtract(rtmp, rtmp2);
	  gdk_region_get_clipbox(rtmp3, &rect);
	  tv->clips[i].x = rect.x;
	  tv->clips[i].y = rect.y;
	  tv->clips[i].width = rect.width;
	  tv->clips[i].height = rect.height;
	  gdk_region_destroy(rtmp2);
	  gdk_region_destroy(rtmp3);
	}

      gdk_region_destroy(rtmp);
    }

  gdk_region_destroy(rexpose);
  gdk_region_destroy(rempty);
  ioctl(tv->fd, VIDIOCSWIN, &tv->vwindow);
}

static void
gtk_tv_clipping_partialvis(GtkTV *tv, GdkEventVisibility *event)
{
  Window *childrenlist, *boguswl, ourwin, parentwin, bogus;
  unsigned int nchildren, nc2;
  gint ourx, oury, ourwidth, ourheight, ourdepth;

  int xret, yret;
  unsigned int widthret, heightret, tmpui;

  int i, j;

  GdkRegion *ourregion, *t;
  GdkRectangle owrect, arect;
  GList *parentlist = NULL;
  GtkAllocation *a;

  /* end vars */

  tv->vwindow.clipcount = 0; /* Reset the clipcount */

  gdk_window_get_origin(GTK_WIDGET(tv)->parent->window, &ourx, &oury);
  a = &(GTK_WIDGET(tv)->allocation);
  owrect.x = ourx + a->x;
  owrect.y = oury + a->y;
  owrect.width = a->width;
  owrect.height = a->height;

  t = gdk_region_new();
  ourregion = gdk_region_union_with_rect(t, &owrect);

  /* Now we have a region and/or rectangle for the
     window. We need to find all windows that could overlap it */
  
  ourwin = GDK_WINDOW_XWINDOW(GTK_WIDGET(tv)->window);

#ifdef DEBUG_GTV
  g_print("We ourself are %#x (on root %#x) at (%d, %d) x %d %d\n", ourwin,
	  GDK_ROOT_WINDOW(),
	  owrect.x, owrect.y, owrect.width, owrect.height);
#endif

  /* The idea here is to find out what the real
     toplevel window is for our toplevel (i.e. getting around
     window manager stuff) */
  while(1)
    {
      XQueryTree(GDK_DISPLAY(), ourwin, &bogus, &parentwin,
		 &boguswl, &nc2);
      
      if(nc2 > 0)
	XFree(boguswl);
      
      if(parentwin == None || parentwin == ourwin)
	{
#ifdef DEBUG_GTV
	  g_print("Breaking because parentwin = %#x\n",
		  parentwin);
#endif
	  break;
	}

#ifdef DEBUG_GTV
      g_print("Appended %#x to parentlist\n", ourwin);
#endif
      parentlist = g_list_append(parentlist, (gpointer)ourwin);

      ourwin = parentwin;
    }

  parentlist = g_list_append(parentlist, (gpointer)GDK_ROOT_WINDOW());

  for(j = 0; j < g_list_length(parentlist); j++)
    {
      XQueryTree(GDK_DISPLAY(), (Window)(g_list_nth(parentlist, j)->data),
		 &bogus, &bogus,
		 &childrenlist, &nchildren);

#ifdef DEBUG_GTV
      g_print("Checking children of window %#x - has %d children\n",
	      g_list_nth(parentlist, j)->data,
	      nchildren);
#endif
      
      for(i = 0; i < nchildren; i++)
	{
	  if(g_list_find(parentlist, (gpointer)childrenlist[i]))
	    {
#ifdef DEBUG_GTV
	      g_print("Skipping window %#x - it's our parent\n",
		      childrenlist[i]);
#endif
	      continue;
	    }

	  ourwidth = 0; ourheight = 0;

	  XGetGeometry(GDK_DISPLAY(), childrenlist[i], &bogus,
		       &ourx, &oury, &ourwidth,
		       &ourheight, &ourdepth, &ourdepth);

	  if(ourwidth == 0 && ourheight == 0)
	    g_warning("XGetGeometry didn't change width or height!\n");

	  XTranslateCoordinates(GDK_DISPLAY(), childrenlist[i],
				GDK_ROOT_WINDOW(),
				0, 0, &ourx, &oury, &bogus);

	  arect.x = ourx; arect.y = oury;
	  arect.width = ourwidth; arect.height = ourheight;
	  
	  if(gdk_region_rect_in(ourregion, &arect) != GDK_OVERLAP_RECTANGLE_OUT)
	    {
	      if(arect.x < owrect.x)
		arect.x = owrect.x;
	      if(arect.y < owrect.y)
		arect.y = owrect.y;
	      if(arect.width > owrect.width)
		arect.width = owrect.width;
	      if(arect.height > owrect.height)
		arect.height = owrect.height;

#ifdef DEBUG_GTV
	      g_print("Window %#x is inside our region - clipping off (%d, %d) - %d %d\n",
		      childrenlist[i],
		      arect.x, arect.y, arect.width, arect.height);
#endif
		      
	      tv->clips[tv->vwindow.clipcount].x = arect.x;
	      tv->clips[tv->vwindow.clipcount].y = arect.y;
	      tv->clips[tv->vwindow.clipcount].width = arect.width;
	      tv->clips[tv->vwindow.clipcount++].height = arect.height;
	    }
	}
      
      if(nchildren > 0)
	XFree(childrenlist);
    }
  gdk_region_destroy(t);
  gdk_region_destroy(ourregion);
#ifdef DEBUG_GTV
  g_print("           TOTAL CLIPS: %d\n", tv->vwindow.clipcount);
#endif
  tv->vwindow.clips = tv->clips;
  tv->clips[0].x = 1;
  tv->clips[0].y = 1;
  tv->clips[0].width = 200;
  tv->clips[0].height = 200;
  ioctl(tv->fd, VIDIOCSWIN, &tv->vwindow);
}
#endif /* X11_IS_PERFECT */

static void
gtk_tv_do_clipping(GtkTV *tv, GdkEvent *event, gboolean is_our_window)
{
  if(event == NULL)
    return;

  switch(event->type)
    {
    case GDK_VISIBILITY_NOTIFY:
#ifdef DEBUG_GTV
      g_print("do_clipping on vis with state = %d\n",
	      event->visibility.state);
#endif
	      
      switch(event->visibility.state)
	{
	case GDK_VISIBILITY_UNOBSCURED:
	  tv->visible = TRUE;
	  tv->vwindow.clipcount = 0;
	  break;
	case GDK_VISIBILITY_PARTIAL:
	  /* Do the dew here */
	  tv->visible = TRUE;
	  getclip(tv);
	  break;
	case GDK_VISIBILITY_FULLY_OBSCURED:
	  tv->visible = FALSE;
	  tv->vwindow.clipcount = 0;
	  break;
	}
      if(ioctl(tv->fd, VIDIOCSWIN, &tv->vwindow))
	g_warning("VIDIOCSWIN failed in do_clipping vis\n");
      if(ioctl(tv->fd, VIDIOCCAPTURE, &tv->visible))
	g_warning("VIDIOCCAPTURE failed in do_clipping vis\n");
      break;
    case GDK_EXPOSE:
    case GDK_MAP:
    case GDK_UNMAP:
    case GDK_CONFIGURE:
      getclip(tv);
      if(ioctl(tv->fd, VIDIOCSWIN, &tv->vwindow))
	g_warning("VIDIOCSWIN failed in do_clipping expose etc.\n");
      break;
    default:
#ifdef DEBUG_GTV
      g_print("do_clipping couldn't handle event type %d\n",
	      event->type);
#endif
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
  g_return_if_fail(ioctl(tv->fd, VIDIOCSCHAN, &inputnum) == 0);
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
  g_return_if_fail(ioctl(tv->fd, VIDIOCSAUDIO, &tv->vaudio) == 0);
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
			GDK_VISIBILITY_NOTIFY_MASK
			|GDK_STRUCTURE_MASK
			|GDK_EXPOSURE_MASK
			|gtk_widget_get_events(tv->oldtoplevel));
#if 0
  tv->toplevel_visibility_id = gtk_signal_connect(GTK_OBJECT(tv->oldtoplevel),
						  "visibility_notify_event",
						  GTK_SIGNAL_FUNC(gtk_tv_toplevel_relay),
						  tv);
#endif
  tv->toplevel_config_id = gtk_signal_connect(GTK_OBJECT(tv->oldtoplevel),
					      "event",
					      GTK_SIGNAL_FUNC(gtk_tv_toplevel_relay),
					      tv);
}

GdkImlibImage *
gdk_get_RGB_from_16(GdkWindow *w, int xx, int yy, int width, int height)
{
   GdkImlibImage *im;
   GdkImage *i;
   gint ww,hh;
   gint x,y;
   unsigned char *data,*ptr;
   unsigned short *ptr2,val;
   
   if (!w) { g_print("no window to get from\n"); return NULL; }
   ww=width;hh=height;
   i=gdk_image_get(w,xx,yy,ww,hh);
   if (!i) { g_print("gdk_image_get failed\n"); return NULL; }
   data=(unsigned char *)g_malloc(ww*hh*3);
   if (!data)
     {
	g_print("malloc failed for %dx%d\n",ww,hh);
	gdk_image_destroy(i);
	return NULL;
     }
   ptr=data;
   ptr2=(unsigned short *)i->mem;
   for(y=0;y<hh;y++)
     {
	for(x=0;x<ww;x++)
	  {
	     val=*ptr2++;
	     *ptr++=(val&0xf800)>>8;
	     *ptr++=(val&0x07e0)>>3;
	     *ptr++=(val&0x001f)<<3;
	  }
     }
   gdk_image_destroy(i);
   im=gdk_imlib_create_image_from_data(data,NULL,ww,hh);
   g_free(data);
   g_print("create_image_from_data(%p,NULL,%d,%d) = %p\n",
	   data,ww,hh,im);
   return im;
}

GdkImlibImage *
gtk_tv_grab_image(GtkTV *tv,
		  gint width, gint height)
{
  GdkImlibImage *retval;
  gpointer membuf;
  int abool;
  GtkWidget *w = GTK_WIDGET(tv);

  g_return_if_fail(tv != NULL);
  g_return_if_fail(GTK_IS_TV(tv));

  abool = 0;
  ioctl(tv->fd, VIDIOCCAPTURE, &abool);

  usleep(50000);

  retval = gdk_get_RGB_from_16(w->window,w->allocation.x,w->allocation.y,
			       w->allocation.width,w->allocation.height);

  abool = tv->visible;
  ioctl(tv->fd, VIDIOCCAPTURE, &abool);

  g_print("gtk_tv_grab_image returns %p\n", retval);

  return retval;

  g_return_if_fail(tv != NULL);
  g_return_if_fail(GTK_IS_TV(tv));

  tv->cbuf.base = g_malloc0(width * height * 3 /* 24bpp */);
  membuf = g_malloc0(width * height * 3 /* 24bpp */);
  tv->cbuf.width = MIN(MAX(width, tv->vcap.minwidth),
		       tv->vcap.maxwidth);
  tv->cbuf.height = MIN(MAX(height, tv->vcap.minheight),
			tv->vcap.maxheight);
  memset(&tv->cwin, 0, sizeof(tv->cwin));
  tv->cwin.width = tv->cbuf.width;
  tv->cwin.height = tv->cbuf.height;

  tv->cbuf.depth = 24;
  tv->cbuf.bytesperline = width * 3;

  tv->cpic = tv->vpic;
  tv->cpic.depth = 24;
  tv->cpic.palette = VIDEO_PALETTE_RGB24;

  abool = 0;
  g_return_val_if_fail(
		       ioctl(tv->fd, VIDIOCCAPTURE, &abool) == 0,
		       NULL);
  g_return_val_if_fail(
		       ioctl(tv->fd, VIDIOCSFBUF, &tv->cbuf) == 0,
		       NULL);
  g_return_val_if_fail(
		       ioctl(tv->fd, VIDIOCSPICT, &tv->cpic) == 0,
		       NULL);
  g_return_val_if_fail(
		       ioctl(tv->fd, VIDIOCSWIN, &tv->cwin) == 0,
		       NULL);
  abool = 1;
  g_return_val_if_fail(
		       ioctl(tv->fd, VIDIOCCAPTURE, &abool) == 0,
		       NULL);

#ifdef DEBUG_GTV
  g_print("Attempting to read %d bytes\n",
	  tv->cbuf.width * tv->cbuf.height * 3);
#endif

  // #define LOOPIT
  #define SLEEPIT
  // #define READIT
#if defined(LOOPIT)
  while(((unsigned char *)tv->cbuf.base)[10] == 0
	&& ((unsigned char *)tv->cbuf.base)[tv->cbuf.width * tv->cbuf.height] == 0)
    /**/;
#elif defined(SLEEPIT)
  sleep(1);
#elif defined(READIT)
  read(tv->fd, membuf, tv->cbuf.width * tv->cbuf.height * 3);
#endif

  abool = 0;
  ioctl(tv->fd, VIDIOCCAPTURE, &abool);
  ioctl(tv->fd, VIDIOCSPICT, &tv->vpic);
  ioctl(tv->fd, VIDIOCSFBUF, &tv->vbuf);
  ioctl(tv->fd, VIDIOCSWIN, &tv->vwindow);
  abool = 1;
  ioctl(tv->fd, VIDIOCCAPTURE, &abool);

  retval = gdk_imlib_create_image_from_data(
#ifdef READIT
					    membuf,
#else
					    tv->cbuf.base,
#endif
					    NULL,
					    (int)tv->cbuf.width,
					    (int)tv->cbuf.height);

  g_free(membuf);
  g_free(tv->cbuf.base);

  return retval;
}

void
gtk_tv_set_format(GtkTV *tv, GtkTVFormat fmt)
{
  gint tmpin;

  g_return_if_fail(tv != NULL);
  g_return_if_fail(GTK_IS_TV(tv));

  /* Bad hack for bttv */
  tmpin = tv->curinput;
  gtk_tv_set_input(GTK_TV(tv), 0);

  tv->vtuner.tuner = 0;
  g_return_if_fail(ioctl(tv->fd, VIDIOCGTUNER, &tv->vtuner) == 0);
  tv->vtuner.tuner = 0;
  tv->vtuner.mode = fmt;
  g_return_if_fail(ioctl(tv->fd, VIDIOCSTUNER, &tv->vtuner) == 0);

  gtk_tv_set_input(GTK_TV(tv), tmpin);
}
