#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>
#include <gdk/gdktypes.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gnome.h>
#include <gtk/gtkeventbox.h>

#include <linux/types.h>
#include <linux/videodev.h>

#include "dummy.xpm"
#include "capturer.xpm"
#include "audio.xpm"
#include "quit.xpm"

#include "channels.h"

#include "gnomovision.h"

static GtkWidget *window;
static GtkWidget *tv;
static GtkWidget *painter;
static GdkGC *painter_gc;
static GdkColor chroma_colour;
static GdkImage *capture;		/* Capture Image */
static GdkImageType capture_type;
static gint capture_h,capture_w;
static gint window_h, window_w;
static gint window_x, window_y;

static int widescreen=0;
static int widescreen_h;

/*
 *	Bottom panel
 */

GtkWidget *channel_label;
GtkWidget *volume_bar;
GtkWidget *size_label;

static GdkColor colourmap[256];		/* Colour map */
static int colourmap_size;
static int colourmap_base;
static GdkColormap *private_colourmap;



static void make_palette_grey(int num)
{
	char buf[16];
	int i,x=0;
	int step=256/num;
	
	for(i=0;i<256; i+=step)
	{
		sprintf(buf,"#%02X%02X%02X",
			i,i,i);
		gdk_color_parse(buf, &colourmap[x]);
		gdk_color_alloc(gdk_colormap_get_system(), &colourmap[x]);
		x++;
	}
	colourmap_size=num;
	colourmap_base=0;
}

static void make_colour_cube(void)
{
	char buf[16];
	int i;
	
	private_colourmap = gdk_colormap_new(gdk_visual_get_best(),256);
	for(i=0;i<16;i++)
	{
		gdk_color_black(private_colourmap, &colourmap[i]);
	}
	for(i=0;i<225;i++)
	{
		int r, rr;
		r=i%25;
		rr=r%5;
		sprintf(buf,"#%02X%02X%02X",
			(65535*(r/5)/4),
			(65535*(i/25)/8),
			(65535*rr/4));
		gdk_color_parse(buf, &colourmap[i+16]);
		gdk_color_alloc(private_colourmap, &colourmap[i+16]);
	}
	colourmap_size=225;
	colourmap_base=16;
	gdk_window_set_colormap(tv->window, private_colourmap);
	gdk_window_set_colormap(window->window, private_colourmap);
}



static void init_gtk_tvcard(void)
{
	if(chroma_key==0)
		gdk_color_black(gdk_colormap_get_system(), &chroma_colour);
	else
	{
		/* We want the most vile unused colour we can get
		   for our chroma key - this is pretty foul */
		gdk_color_parse("#FA77A1", &chroma_colour);
		gdk_color_alloc(gdk_colormap_get_system(), &chroma_colour);
	}
	if(need_greymap!=0)
		make_palette_grey(need_greymap);
}

/*
 *	Capture an image from capture cards - idea and X11 side taken
 *	from the excellent example in sane-0.67/xcam
 */

#define READ_VIDEO_PIXEL(buf, format, depth, r, g, b)			\
{									\
	switch (format)							\
	{								\
		case VIDEO_PALETTE_GREY:				\
			switch (depth)					\
			{						\
				case 4:					\
				case 6:					\
				case 8:					\
					(r) = (g) = (b) = (*buf++ << 8);\
					break;				\
									\
				case 16:				\
					(r) = (g) = (b) = 		\
						*((guint16 *) buf);	\
					buf += 2;			\
					break;				\
			}						\
			break;						\
									\
									\
		case VIDEO_PALETTE_RGB565:				\
		{							\
			unsigned short tmp = *(unsigned short *)buf;	\
			(r) = tmp&0xF800;				\
			(g) = (tmp<<5)&0xFC00;				\
			(b) = (tmp<<11)&0xF800;				\
			buf += 2;					\
		}							\
		break;							\
									\
		case VIDEO_PALETTE_RGB555:				\
			(r) = (buf[0]&0xF8)<<8;				\
			(g) = ((buf[0] << 5 | buf[1] >> 3)&0xF8)<<8;	\
			(b) = ((buf[1] << 2 ) & 0xF8)<<8;		\
			buf += 2;					\
			break;						\
									\
		case VIDEO_PALETTE_RGB24:				\
			(r) = buf[0] << 8; (g) = buf[1] << 8; 		\
			(b) = buf[2] << 8;				\
			buf += 3;					\
			break;						\
									\
		default:						\
			fprintf(stderr, 				\
				"Format %d not yet supported\n",	\
				format);				\
	}								\
}						


#define PUT_X11_PIXEL(buf, endian, depth, bpp, r, g, b, gl_map)		\
{									\
	switch (depth)							\
	{								\
		case  1: /* duh?  A Sun3 or what?? */			\
			lum = 3*(r) + 5*(g) + 2*(b);			\
			if (lum >= 5*0x8000)				\
				*buf |= dst_mask;			\
			dst_mask <<= 1;					\
			if (dst_mask > 0xff)				\
			{						\
				buf += (bpp);				\
				dst_mask = 0x01;			\
			}						\
			break;						\
									\
		case  8:	/* FIXME: Mono only right now */	\
			lum = ((3*(r) + 5*(g) + 2*(b)) / (10 * 256/colourmap_size)) >> 8;	\
			if (lum >= colourmap_size)			\
				lum = colourmap_size;			\
			buf[0] = colourmap[lum+colourmap_base].pixel;	\
			buf += (bpp);					\
			break;						\
		case 15:						\
			rgb = (  (((r) >> 11) << 10)	/* 5 bits of red */		\
			| (((g) >> 11) <<  5)	/* 5 bits of green */	\
			| (((b) >> 11) <<  0));	/* 5 bits of blue */	\
			((guint16 *)buf)[0] = rgb;			\
			buf += (bpp);					\
			break;						\
									\
		case 16:						\
			rgb = (  (((r) >> 11) << 11)	/* 5 bits of red */	\
			| (((g) >> 10) <<  5)	/* 6 bits of green */	\
			| (((b) >> 11) <<  0));	/* 5 bits of blue */	\
			((guint16 *)buf)[0] = rgb;			\
			buf += (bpp);					\
			break;						\
									\
		case 24:						\
			/* Is this correctly handling all byte order cases? */		\
			if ((endian) == GDK_LSB_FIRST)			\
			{						\
				buf[0] = (b) >> 8; buf[1] = (g) >> 8; buf[2] = (r) >> 8;\
			}						\
			else						\
			{						\
				buf[0] = (r) >> 8; 			\
				buf[1] = (g) >> 8; 			\
				buf[2] = (b) >> 8;			\
			}						\
			buf += (bpp);					\
			break;						\
									\
		case 32:						\
      /* Is this correctly handling all byte order cases?  It assumes	\
	 the byte order of the host is the same as that of the		\
	 pixmap. */							\
			rgb = (((r) >> 8) << 16) | (((g) >> 8) << 8) | ((b) >> 8);\
			((guint32 *)buf)[0] = rgb;			\
			buf += (bpp);					\
			break;						\
		default:						\
			fprintf(stderr, "Unsupported X11 depth.\n");	\
	}								\
}
 
static void grab_image(unsigned char *dst, int xpos, int ypos)
{
	int x;
	int lines=0;
	int pixels_per_line, bytes_per_line, byte_order, bytes_per_pixel;
	int dst_depth, src_depth;
	guint32 r = 0, g = 0, b = 0, lum, rgb, src_mask, dst_mask;
	unsigned char *mem, *src;	

	src_depth = vpic.depth;
	dst_depth = capture->depth;
	
	pixels_per_line = capture_w;
	bytes_per_line = capture->bpl;
	bytes_per_pixel = capture->bpp;
	byte_order = capture->byte_order;

	mem=malloc(bytes_per_line * capture_h);
	if(mem==NULL)
		return;

	src=mem;
	
	read(tv_fd, mem, bytes_per_line * capture_h);
			
	x = 0;

	src_mask = 0x80;
	dst_mask = 0x01;
	
	while (lines<capture_h)
	{
		READ_VIDEO_PIXEL(src, grabber_format, src_depth, r, g, b);
		PUT_X11_PIXEL(dst, byte_order, dst_depth, bytes_per_pixel, r, g, b,
			win.canvas.graylevel);
		if (++x >= pixels_per_line)
		{
			x = 0;
			dst += bytes_per_line - pixels_per_line * bytes_per_pixel;
			lines++;
		}
	}
	free(mem);
}

static void write_ppm_image(FILE *f)
{
	int x;
	int lines=0;
	int pixels_per_line, bytes_per_line, byte_order, bytes_per_pixel;
	int dst_depth, src_depth;
	guint32 r = 0, g = 0, b = 0, src_mask, dst_mask;
	unsigned char *mem, *src;	

	src_depth = vpic.depth;
	dst_depth = capture->depth;
	
	pixels_per_line = capture_w;
	bytes_per_line = capture->bpl;
	bytes_per_pixel = capture->bpp;
	byte_order = capture->byte_order;

	mem=malloc(bytes_per_line * capture_h);
	if(mem==NULL)
		return;

	src=mem;
	
	read(tv_fd, mem, bytes_per_line * capture_h);
			
	x = 0;

	src_mask = 0x80;
	dst_mask = 0x01;
	
	fprintf(f,"P6 %d %d 255\n",
		pixels_per_line, capture_h);
	
	while (lines<capture_h)
	{
		READ_VIDEO_PIXEL(src, grabber_format, src_depth, r, g, b);
		fputc(r>>8, f);
		fputc(g>>8, f);
		fputc(b>>8, f);
		if (++x >= pixels_per_line)
		{
			x=0;
			lines++;
		}
	}
	free(mem);
	fclose(f);
}


/*
 *	Paint the drawing area. We use this to keep it clean when not
 *	capturing, for chromakey areas and for mitshm.
 */
 
static void do_painting(void)
{
	gint x,y,w,h,d;
	char buf[40];
	
	/*
	 *	Called during setup - ignore	 
	 */
	 
	if(painter_gc==NULL)
		return;
		
		
        gdk_window_get_geometry(painter->window, &x, &y, &w, &h, &d);
        
        snprintf(buf,40,"%dx%d",w,h);
        
        gtk_label_set(size_label,buf);

	if(use_shm && capture && capture_running && !capture_hidden)
	{
		if(widescreen)
		{
			gdk_draw_image(painter->window, painter_gc, capture,
				0, capture_h/10, 
				0, 0,
				capture_w,
				widescreen_h);
		}
		else
		{
			gdk_draw_image(painter->window, painter_gc, capture,
				0,0,0,0, 
				capture_w, capture_h);
		}
	}
	else
		gdk_draw_rectangle(painter->window, painter_gc, 1, 0, 0, w, h);
	gdk_flush();
}
		

static gint tv_grabber(gpointer data)
{
	grab_image(capture->mem, 0, 0);
	do_painting();
	return TRUE;
}

static void tv_capture(int handle, int onoff)
{
	if(use_shm==0)
	{
		/*
		 *	Dont capture if unmapped
		 */

		capture_running = onoff;

		if(capture_hidden)
			onoff=0;
			
		if(ioctl(handle, VIDIOCCAPTURE, &onoff)==-1)
			perror("Capture");
/*		printf("Capture is now %d\n",  onoff);*/
	}
	else
	{
		static gint capture_tag;
		if(capture_running && capture && capture->mem)
			grab_image(capture->mem, 0,0);
		if(onoff)
			capture_tag = gtk_idle_add(tv_grabber, 0);
		else
			gtk_idle_remove(capture_tag);
		capture_running = onoff;
	}
}


static void tv_set_window(int handle)
{
	gint x,y,w,h,d;
	GdkWindow *t=painter->window;
	
	/*
	 *	GDK holds the window positioning
	 */
	
        gdk_window_get_geometry( t, &x, &y, &w, &h, &d);
        
        
        	
        /*
         *	We are working on screen positions with the overlay
         *	devices, so see where we are relative to the screen.
         */
        
        gdk_window_get_origin(t, &x, &y);
        if(x==window_x && y==window_y && h==window_h && w==window_w)
        	return;

/*        printf("SET Window :: Window is %d by %d at %d,%d\n", w,h,x,y);*/
	
	if(w < xmin || h < ymin)
	{
		/* Too small, turn it off */
		tv_capture(handle,0);
	}
	
	/* Constrain the window */
	
	h=min(h,ymax);
	w=min(w,xmax);
	
	window_h=h;
	window_w=w;
	window_x=x;
	window_y=y;
	
	widescreen_h=h;
	
	vwin.x = x;
	vwin.y = y;
	vwin.width = w;
	vwin.height = h;
	vwin.clips = NULL; 
	vwin.flags = 0;
	vwin.clipcount = 0;
	
	if(h<32 || w<32)
		return;

	if(widescreen)
	{
		/*
		 *	In wide screen mode we want to overcapture vertically
		 *	by about 10% above and below.
		 */
		vwin.y-=h/10;
		vwin.height+=h/5;
	}
	if(ioctl(tv_fd, VIDIOCSWIN, &vwin)==-1)
		perror("set video window");
	if(ioctl(tv_fd, VIDIOCGWIN, &vwin)==-1)
		perror("get video window");
	/* x,y,h,w are now the values that were chosen by the
	   device to be as close as possible - eg the quickcam only
	   has 3 sizes */
	xsize = vwin.width;
	ysize = vwin.height;

	/*
	 *	Now check the image in question
	 */
	 
	if(use_shm)
	{
		if(capture==NULL || capture_w!=xsize || capture_h!=ysize)
		{
			capture_type = GDK_IMAGE_FASTEST;
			
			if(capture!=NULL)
			{
				gdk_image_destroy(capture);
				capture=NULL;
			}
#ifdef __alpha__
			/*
			 *	Taken from the sane xcam
			 */
			 
			/* Some X servers seem to have a problem with shared images that
			    have a width that is not a multiple of 8.  Duh... ;-( */
			if (xsize % 8)
				capture_type = GDK_IMAGE_NORMAL;
#endif
			
			capture=gdk_image_new(capture_type, 
					gdk_window_get_visual(t),
					xsize,ysize);
					
			capture_h=ysize;
			capture_w=xsize;
		}
		/*
		 *	Capture and draw
		 */
			 
		d=capture_running;
		capture_running=0;
		do_painting();
		capture_running=d;
			
		/*
		 *	Start grabbing again
		 */
		
		grab_image(capture->mem, x,y);
	}
}

static int has_shrunk_or_moved(GtkWidget *wp)
{
	int w,h;
	int x,y;
	
	gdk_window_get_size(wp->window, &w,&h);
        gdk_window_get_origin(wp->window, &x, &y);
        
/*        printf("was at %d,%d,%d,%d now at %d,%d,%d,%d\n",
        	window_x,window_y,window_w,window_h,
        	x,y,w,h);*/
	
	if(w<window_w || h<window_h || x!=window_x || y!=window_y)
		return 1;
	return 0;
}

static void show_pos(void)
{
	tv_set_window(tv_fd);
}


static void capture_on(void)
{
	tv_set_window(tv_fd);
	tv_capture(tv_fd, 1);
	do_painting();
}

static void capture_off(void)
{
	tv_capture(tv_fd,0);
	do_painting();
}

static void set_audio(GtkObject *foo, void *bar)
{
	static int audio_on=0;
	int n=(int)bar;
	struct video_audio va;
	va.audio = 0;	/* For now FIXME FIXME */
	if(ioctl(tv_fd, VIDIOCGAUDIO, &va)==-1)
	{
		perror("get audio");
		return;
	}
	
	if(n==-1)
		n = 1-audio_on;
	if(n)
		va.flags&=~VIDEO_AUDIO_MUTE;
	else
		va.flags|=VIDEO_AUDIO_MUTE;
		
	audio_on = n;
		
	if(ioctl(tv_fd, VIDIOCSAUDIO, &va)==-1)
		perror("set audio");
}
		
static int crap_eraser_tag;
static int crap_eraser_running;
static int capturer_tag;
static int capturer_running;
static int crap_eraser_lock;
static int crap_eraser_delay=0;

static gint crap_eraser(gpointer trash)
{
	if(!crap_eraser_running)
	{
		static int warn=0;
		if(warn++==0)
			printf("Your gtk seems to have an idle bug.\n");
		return FALSE;
	}
	else
	{
		system("/usr/X11R6/bin/xrefresh");
/*		printf("RAN CRAPPER\n");*/
		crap_eraser_lock = 0;
		crap_eraser_delay = 1;
	}
	crap_eraser_running=0;
	return FALSE;
}

static gint capturer(gpointer foo)
{
	if(crap_eraser_running)
	{
		crap_eraser(NULL);
		gtk_idle_remove(crap_eraser_tag);
	}
	if(capturer_running)
	{
		tv_capture(tv_fd, capture_running);
/*		printf("RAN CAPTURER\n");*/
	}
	capturer_running=0;
	return FALSE;
}


static void queue_crap_eraser(int n)
{
	if(crap_eraser_running==0)
	{
/*		printf("CRAPPER QUEUED\n");*/
		crap_eraser_running=1;
		crap_eraser_tag=gtk_idle_add(crap_eraser,0);
	}
	else
/*		printf("CRAPPER ACTIVE\n")*/;
	crap_eraser_lock|=n;
}

static void queue_capturer(void)
{
	if(capturer_running==0)
	{
/*		printf("CAPTURER QUEUED\n");*/
		capturer_tag=gtk_idle_add(capturer,0);
		capturer_running=1;
	}
		
}


/*
 *	Check for a repaint job
 */
  
static gint painting_event(GtkWidget *self, GdkEvent *event)
{
#if 0
	GList *winlist;
	GList *node;
	GdkWindow *win;
	static struct video_clip clips[256];
	int next_clip = 0;
#endif	
	/* Remember if we are drawing */
	int capture_was_running = capture_running;
	int crapped_on = 0;

	if(self != painter && event->type!=GDK_UNMAP)
		return FALSE;
	
/*	printf("event %d\n", event->type);*/
	
	switch(event->type)
	{
		case GDK_VISIBILITY_NOTIFY:
/*			printf("VISIBILITY::  ");*/
		
			/*
			 *	It might look like shared memory cards
			 *	dont need handling at all here, but we want
			 *	 to turn CPU draining captures off whenever
			 *	we can.
			 */
			 				
			switch(event->visibility.state)
			{
				case GDK_VISIBILITY_UNOBSCURED:
/*					printf("Window now unobscured\n");*/
					capture_hidden=0;
					if(capture_running)
						queue_capturer();
					break;
				case GDK_VISIBILITY_PARTIAL:
/*					printf("Partially obscured.\n");*/
					if(use_shm)
					{
						capture_hidden=0;
						tv_capture(tv_fd, capture_running);
						break;
					}
					if(vcap.type&VID_TYPE_CLIPPING)
					{
						/*
						 *	FIXME: write the code here for clipping capable cards
						 */
					}
					/* Fall through. If we cant clip then
					   we stop on partial overrides - messy but
					   unavoidable */
					
				case GDK_VISIBILITY_FULLY_OBSCURED:
					/*
					 *	We aren't here. Turn off capturing
					 *	to save resources and to avoid
					 *	cards peeing into video space.
					 *
					 *	This could however have been caused by an Xrefresh
					 *	and the resulting events will be async so we must
					 *	be clever. We skip the first visibility message after
					 *	we refresh.
					 */
/*					printf("Window has obscurances\n");*/
					if(capture_hidden)
						break;
					capture_hidden=1;
					tv_capture(tv_fd, capture_running);
					if(!crap_eraser_delay && capture_running && clean_display)
					{
/*						printf("Redraw because: Window became obscured\n");*/
						crapped_on|=clean_display;
						if(crapped_on)
							queue_crap_eraser(0);
					}
					else
						crap_eraser_delay=0;
				default:;
			}
			break;
			
		case GDK_UNMAP:
/*			printf("UNMAP - turning the capture off.\n");*/
			capture_hidden=1;
			tv_capture(tv_fd,capture_running);
			if(capture_running && clean_display)
			{
				crapped_on |= clean_display;
				if(crapped_on && capture_was_running)
				{
/*					printf("Refreshing (Window unmapped)\n");*/
					queue_crap_eraser(1);
				}
			}
			break;
		case GDK_MAP:
/*			printf("MAP - turning on the tv if needed.\n");*/
			capture_hidden=0;
			tv_set_window(tv_fd);
			tv_capture(tv_fd,capture_running);
			goto draw_it;
		case GDK_CONFIGURE:
			/*
			 *	Ok we may need to draw. We will need to move
			 *	the tv
			 */

			if(clean_display && capture_was_running && has_shrunk_or_moved(painter))
			{
/*				printf("CONFIGURE queues crap_eraser\n");*/
				queue_crap_eraser(1);
			}
			tv_set_window(tv_fd);
			break;
			
		/*
		 *	A no expose is as good as an expose from our viewpoint
		 *	in fact it warrants more work cos we've just farted
		 *	on someone elses windows until we add the video stuff
		 *	to the X server
		 */
		 
		case GDK_NO_EXPOSE:
/*			printf("Got a NO EXPOSE - may be dirty.\n");*/
			
		case GDK_EXPOSE:
draw_it:
/*			printf("Doing EXPOSE drawing\n");*/
			tv_set_window(tv_fd);
			if(!painter_gc)
			{
				painter_gc=gdk_gc_new(painter->window);
				gdk_gc_set_foreground(painter_gc, &chroma_colour);
				gdk_gc_set_background(painter_gc, &chroma_colour);
			}	
			do_painting();
			if(crapped_on && capture_was_running && has_shrunk_or_moved(painter))
			{
/*				printf("Refreshing (move or resize)\n");*/
				queue_crap_eraser(1);
			}
			break;
		default:
			break;
	}
	return FALSE;
}

 
static void set_channel(GtkObject *o, void *p)
{
	int channel=(int)p;
	printf("set_channel to %d\n", channel);
	if(ioctl(tv_fd, VIDIOCSCHAN, &channel)==-1)
		perror("set channel");
	/*
	 *	See if we have a frequency 
	 */
	 
	ioctl(tv_fd, VIDIOCGFREQ, &vbase_freq);
}

static void set_format(GtkObject *o, void *p)
{
	int format = (int)p;
	
	vt.tuner = 0;		/* FIXME : support multiple tuners */
	
	if(ioctl(tv_fd, VIDIOCGTUNER, &vt)==-1)
		perror("get tuner");

	vt.mode = format;
	vt.tuner = 0;
	
	if(ioctl(tv_fd, VIDIOCSTUNER, &vt)==-1)
		perror("set tuner");
}

		
/*
 *	Widget Stuff
 */
 
void format_bar(GtkWidget *menubar)
{
	GtkWidget* menu;
	GtkWidget* menuitem;
	int n;
	static char *name[4]= {
		"PAL",
		"NTSC",
		"SECAM",
		"AUTO"
	};

	menu = gtk_menu_new();
	
	menuitem = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	for(n=0;n<4;n++)
	{
		
		vt.tuner = 0; 	/* FIXME: support multiple tuners */
		if(ioctl(tv_fd, VIDIOCGTUNER, &vt)==0)
		{
			vt.tuner = 0;
			vt.mode = n;
			if(ioctl(tv_fd, VIDIOCSTUNER, &vt)==-1)
				continue;
		}
		menuitem = gtk_menu_item_new_with_label(name[n]);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
                     (GtkSignalFunc) set_format,(void *)n);
		gtk_widget_show(menuitem);
	}

	menuitem = gtk_menu_item_new_with_label("Format");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
	gtk_container_add(GTK_CONTAINER(menubar), menuitem);
	gtk_widget_show(menuitem);
	vt.tuner = 0; 	/* FIXME: support multiple tuners */
	if(ioctl(tv_fd, VIDIOCGTUNER, &vt)==0)
	{
		vt.mode=0;
		ioctl(tv_fd, VIDIOCSTUNER, &vt);
	}
}


static void process_help(GtkObject *o, void *p)
{
	int x,y;
        gdk_window_get_origin(painter->window, &x, &y);
        x-=30;
        y-=20;
        if(x<0)
        	x+=60;
        if(y<0)
        	y+=30;
        	
        /* make_about_box(x,y); */
}
        

void help_bar(GtkWidget *menubar)
{
	GtkWidget* menu;
	GtkWidget* menuitem;
	int n;
	static char *name[2]= {
		"About",
		"Help Topics"
	};

	menu = gtk_menu_new();
	
	menuitem = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	for(n=0;n<2;n++)
	{
		menuitem = gtk_menu_item_new_with_label(name[n]);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
                     (GtkSignalFunc) process_help,(void *)n);
		gtk_widget_show(menuitem);
	}

	menuitem = gtk_menu_item_new_with_label("Help");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menuitem));
	gtk_container_add(GTK_CONTAINER(menubar), menuitem);
	gtk_widget_show(menuitem);
}

void channel_bar(GtkWidget *menubar)
{
	GtkWidget* menu;
	GtkWidget* menuitem;
	struct video_capability vc;
	struct video_channel chan;
	int n;
	char buf[256];

	menu = gtk_menu_new();
	
	menuitem = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	if(ioctl(tv_fd, VIDIOCGCAP, &vc)==-1)
	{
		perror("get capabilities");
		return;
	}
	
	sprintf(buf, "Gnomovision 1.09: %s", vc.name);
	
	gtk_window_set_title(GTK_WINDOW(window), buf);
	
	for(n=0;n<vc.channels;n++)
	{
		chan.channel=n;
		if(ioctl(tv_fd, VIDIOCGCHAN, &chan)==-1)
		{
			perror("get channel");
			continue;
		}
		printf("Got channel %s\n", chan.name);
		menuitem = gtk_menu_item_new_with_label(chan.name);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
                     (GtkSignalFunc) set_channel,(void *)n);
		gtk_widget_show(menuitem);
	}
	
	menuitem = gtk_menu_item_new_with_label("Input");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
	gtk_container_add(GTK_CONTAINER(menubar), menuitem);
	gtk_widget_show(menuitem);
}

GtkWidget *tv_menu_bar(void)
{
	GtkWidget* menubar;
	GtkWidget* menu;
	GtkWidget* menuitem;

	menubar = gtk_menu_bar_new();
	menu = gtk_menu_new();
	gtk_widget_show(menubar);

	menuitem = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	menuitem = gtk_menu_item_new_with_label("Show");
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
                     (GtkSignalFunc) show_pos, NULL);
	gtk_widget_show(menuitem);

	menuitem = gtk_menu_item_new_with_label("Capture On");
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
                     (GtkSignalFunc) capture_on, NULL);
	gtk_widget_show(menuitem);

	menuitem = gtk_menu_item_new_with_label("Capture Off");
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
                     (GtkSignalFunc) capture_off, NULL);
	gtk_widget_show(menuitem);

	if(vcap.audios!=0)
	{
		menuitem = gtk_menu_item_new_with_label("Audio On");
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
                     (GtkSignalFunc) set_audio, (void *)1);
		gtk_widget_show(menuitem);

		menuitem = gtk_menu_item_new_with_label("Audio Off");
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
	                     (GtkSignalFunc) set_audio, (void *)0);
		gtk_widget_show(menuitem);
	}
	
	menuitem = gtk_menu_item_new_with_label("Preferences");
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
				(GtkSignalFunc) preferences_page, NULL);
	gtk_widget_show(menuitem);

	menuitem = gtk_menu_item_new_with_label("Exit");
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
                     (GtkSignalFunc) gtk_main_quit, NULL);
	gtk_widget_show(menuitem);

	menuitem = gtk_menu_item_new_with_label("File");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
	gtk_container_add(GTK_CONTAINER(menubar), menuitem);
	gtk_widget_show(menuitem);

	channel_bar(menubar);
	
	format_bar(menubar);
	
	help_bar(menubar);
	
	return menubar;
}



static void wide_toggle(void)
{
	int capt = capture_running;
	widescreen = 1 - widescreen;
	tv_capture(tv_fd, 0);
	tv_set_window(tv_fd);
	tv_capture(tv_fd, capt);
}

static void audio_toggle(void)
{
	set_audio(NULL, (void *)-1);
}

static void capture_toggle(void)
{
	if(!capture_running)
		tv_set_window(tv_fd);
	tv_capture(tv_fd, 1-capture_running);
	do_painting();
}

static int snapshot_chosen;
GtkWidget *file_selector;

static void snapshot_run(char *name)
{
	FILE *fp=fopen(name, "w");
	if(fp==NULL)
	{
		/* FIXME: write an alert dialog */
		fprintf(stderr,"Cannot open %s (%s)\n", 
			name, strerror(errno));
		return;
	}
	write_ppm_image(fp);
}

static void snapshot_clicked(GtkWidget *w, gpointer data)
{
	snapshot_chosen = (int)data;
	if(snapshot_chosen)
	{
		snapshot_run(gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selector)));
	}
	gtk_widget_destroy(file_selector);
	file_selector=NULL;
}

static void snapshot_image(void)
{
	GtkWidget *filesel;
	
	if(file_selector)
		return;
	
	filesel = gtk_file_selection_new("Save as");
	
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
		"clicked", (GtkSignalFunc) snapshot_clicked, (void *)0);

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
		"clicked", (GtkSignalFunc) snapshot_clicked, (void *)1);
		
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel), "tv.pnm");
	gtk_widget_show(filesel);
	file_selector = filesel;
}
	
static GnomeUIInfo tbar[] =
{
	GNOMEUIINFO_ITEM(NULL, "Capture Images", capture_toggle, capturer_xpm),
	GNOMEUIINFO_ITEM(NULL, "Save Snapshot", snapshot_image, quit_xpm),
	GNOMEUIINFO_ITEM(NULL, "Audio on/off", audio_toggle, audio_xpm),
	/*	GNOMEUIINFO_ITEM(NULL, "Select a channel", frequency_setting , dummy_xpm), */
	GNOMEUIINFO_ITEM(NULL, "Exit Gnomovision", gtk_main_quit, quit_xpm),
	GNOMEUIINFO_END
};


void
dnd_drag_request(GtkWidget *tv, GdkEvent *event)
{
#ifdef FINISHED
	char *data;
	gtk_widget_dnd_data_set(tv, event, NULL, 0);
#endif
}

void make_tv_set(void)
{
	GtkWidget *menubar;
	GtkWidget *hbox, *vbox, *frame;
	const char *possible_drag_type = "image/ppm";

	window = gnome_app_new("gnomovision", "Televison Gnome");
	gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, TRUE);
	
	xsize = gnome_config_get_int("/gnomovision/geometry/xsize=320");
	ysize = gnome_config_get_int("/gnomovision/geometry/xsize=240");
 
	gtk_widget_set_events(window, GDK_EXPOSURE_MASK|GDK_VISIBILITY_NOTIFY_MASK|GDK_ALL_EVENTS_MASK);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", capture_off, NULL);
	gtk_signal_connect(GTK_OBJECT(window), "delete_event", gtk_main_quit, NULL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", capture_off, NULL);
	gtk_signal_connect(GTK_OBJECT(window), "delete_event", gtk_main_quit, NULL);
	gtk_signal_connect(GTK_OBJECT(window), "map_event", (GtkSignalFunc)
		painting_event, 0);
	gtk_signal_connect(GTK_OBJECT(window), "unmap_event", (GtkSignalFunc)
		painting_event, 0);
	gtk_signal_connect(GTK_OBJECT(window), "other_event", (GtkSignalFunc)
		painting_event, 0);

	vbox = gtk_vbox_new(FALSE, 0);
	
	gnome_app_set_contents(GNOME_APP(window), vbox);

	/*
	 *	FIXME: now I can't resize below this user size. How do
	 *	I fix that ?
	 */
	 	
	hbox = tv = gtk_event_box_new();
	gtk_widget_set_usize(tv, xsize, ysize);
#if 0
	gtk_signal_connect(GTK_OBJECT(tv), "drag_request_event",
			   GTK_SIGNAL_FUNC(dnd_drag_request),
			   NULL);
	gtk_widget_dnd_drag_set(GTK_WIDGET(tv), TRUE, (char **)&possible_drag_type,
				1);
#endif
	
	painter=gtk_drawing_area_new();
	gtk_drawing_area_size(GTK_DRAWING_AREA(painter), xsize, ysize);
	gtk_widget_set_events(painter, GDK_EXPOSURE_MASK|GDK_VISIBILITY_NOTIFY_MASK|GDK_ALL_EVENTS_MASK);
	gtk_signal_connect(GTK_OBJECT(painter), "event", (GtkSignalFunc)
		painting_event, 0);
	gtk_signal_connect(GTK_OBJECT(painter), "map_event", (GtkSignalFunc)
		painting_event, 0);
	gtk_signal_connect(GTK_OBJECT(painter), "unmap_event", (GtkSignalFunc)
		painting_event, 0);
	gtk_signal_connect(GTK_OBJECT(painter), "other_event", (GtkSignalFunc)
		painting_event, 0);
	gtk_container_add(GTK_CONTAINER(tv), painter);
	

	menubar = tv_menu_bar();
	gnome_app_set_menus(GNOME_APP(window), GTK_MENU_BAR(menubar));
	gnome_app_create_toolbar(GNOME_APP(window), tbar);
//	gnome_app_toolbar_set_position(GNOME_APP(window), GNOME_APP_POS_BOTTOM);
	
	gtk_container_border_width(GTK_CONTAINER(GNOME_APP(window)->menubar), 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
/*	gtk_container_add(GTK_CONTAINER(window), vbox); */
	
	gtk_signal_connect(GTK_OBJECT(tv), "expose_event", 
		(GtkSignalFunc)painting_event, NULL);
		
	gtk_widget_set_events(tv, GDK_STRUCTURE_MASK|GDK_EXPOSURE_MASK|GDK_ALL_EVENTS_MASK);

	if(need_colour_cube) 
		make_colour_cube();

	gtk_widget_show(hbox);
	
	hbox = gtk_hbox_new(FALSE, 2);
	
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_IN);
	channel_label = gtk_label_new("");
	gtk_widget_set_usize(channel_label, 96,16);
	gtk_container_add(GTK_CONTAINER(frame), channel_label);
	gtk_widget_show(channel_label);
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_IN);
	size_label = gtk_label_new("");
	gtk_widget_set_usize(size_label, 96,16);
	gtk_container_add(GTK_CONTAINER(frame), size_label);
	gtk_widget_show(size_label);
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE/*TRUE*/, FALSE, 0);
	
	gtk_widget_show(painter);
	gtk_widget_show(hbox);
	gtk_widget_show(vbox);
	gtk_widget_show(window);
	tv_set_window(tv_fd);
}

static GtkWidget *choose;
static int chosen_unit = 0;

static void video_device_ok(void)
{
	/*
	 *	Open the card
	 */
	 
	if((tv_fd=open_tv_card(chosen_unit))==-1)
		return;
	/*
	 *	Do the magic to set our colours up
	 */
	
	init_gtk_tvcard();	
	
	/*
	 *	Blow the selector away
	 */
	 
	gtk_widget_destroy(choose);
	
	/*
	 *	And sprout a tvset
	 */
	 
	make_tv_set();
}

static void video_device_pick(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	chosen_unit = (int)data;
	
	if(event->type == GDK_2BUTTON_PRESS && event->button == 1)
		video_device_ok();
}

static void choose_video_device(void)
{
	GtkWidget *main_vbox, *vbox, *hbox, *button, *frame;
	GSList *owner;
	gint i;
	int count=0;
	
	choose = gtk_dialog_new();
	
	gtk_window_set_title(GTK_WINDOW(choose),"Select Video Device");
	main_vbox = GTK_DIALOG(choose)->vbox;
	
	frame=gtk_frame_new("Select Video Device");
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_IN);
	gtk_container_border_width(GTK_CONTAINER(frame),5);
	
	gtk_container_add(GTK_CONTAINER(main_vbox), frame);
		
	vbox = gtk_vbox_new(FALSE,5);
	gtk_container_border_width(GTK_CONTAINER(vbox),3);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);
	gtk_widget_show(frame);
	owner=NULL;
	
	for(i=0;i<240;i++)
	{
		char *p=probe_tv_set(i);
		if(p==NULL)
			continue;
		if(count==0)
		{
			chosen_unit=i;
		}
		count++;
		button=gtk_radio_button_new_with_label(owner, p);
		gtk_signal_connect(GTK_OBJECT(button), "button_press_event",
			(GtkSignalFunc)video_device_pick,
			(void *)i);
		gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0);
		gtk_widget_show(button);
		owner = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
	}
	hbox = GTK_DIALOG(choose)->action_area;
	
	button = gtk_button_new_with_label("OK");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)video_device_ok, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);
	gtk_widget_grab_default(button);
	
	button = gtk_button_new_with_label("Cancel");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)gtk_main_quit, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

#ifdef FIXME	
	if(count==0)
		video_no_devices();
	if(count==1)
		video_device_ok();
	else
#endif	
		gtk_widget_show(choose);
}


/************** DANGER ZERO CLUE GNOME FEATURE SUPPORT AHEAD *****************/

/*
 *	This heap of shite is caused by 'argp'. I don't know which idiot
 *	added it to Gnome but they ought to be forced to program in COBOL
 *	for 3 months as punishment. Note all options are gone, whichever
 *	fuckwit added Argp can damn well fix it not me.
 */
 

#if 0
static error_t parse_an_arg(int keycrap, char *argcrap, struct argp_state *fucked)
{
	if(keycrap!=ARGP_KEY_ARG)
		return ARGP_ERR_UNKNOWN;
	return 0;
}

static struct argp shite1=
{
	NULL,
	parse_an_arg,
	N_("FILE ..."),
	NULL,
	NULL,
	NULL,
	NULL
};
#endif
/*
 *	Run the TV set
 */

int main(int argc, char *argv[])
{
	int n=0;
	gnome_init("tvset", "0.1", argc, argv);
	if(argv[1] && strcmp(argv[1], "-w")==0)
	{
		argv++;
		widescreen=1;
	}
	if(argv[1])
	{
		sscanf(argv[1],"%d",&n);
		if((tv_fd=open_tv_card(n))==-1)
			return 1;
		init_gtk_tvcard();
		make_tv_set();
	}
	else
		choose_video_device();
	
	gtk_main();
	
/*	printf("Exiting\n");*/
	if(tv_fd != -1)
		close_tv_card(tv_fd);
	return 0;
}
