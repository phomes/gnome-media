/*
 *	Nasty low level Xisms	
 */

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

int video_ll_mapping(struct video_buffer *vb)
{
	Display *disp;
	Screen *scr;
	Window root;
	XWindowAttributes xwa;
	XPixmapFormatValues *pf;

	unsigned int rwidth;
	int bank, ram, major, minor;

	disp=GDK_DISPLAY();

	if (XF86DGAQueryVersion(disp, &major, &minor)) 
		XF86DGAGetVideoLL(disp, DefaultScreen(disp), (void *)&(vb->base),
		&rwidth, &bank, &ram);
	else 
	{
		fprintf(stderr,"XF86DGA: DGA not available.\n");
		return -1;
	}

	/*
	 *	Hunt the visual
	 */

	/* GDK doesnt return 32 bpp for some systems, returns 24
	 * OK, so in reality it's 24...but the framebuffer might be
	 * different.  Following code fixes this...
  	 */

	scr = DefaultScreenOfDisplay (disp);
	root = DefaultRootWindow (disp);
	XGetWindowAttributes (disp, root, &xwa);
	vb->depth = xwa.depth;
	
	pf = XListPixmapFormats (disp, &ram);	
	for (bank = 0; bank < ram; bank++) {
		if (pf[bank].depth == vb->depth) {
			vb->depth = pf[bank].bits_per_pixel;
			break;
		}
	}

	vb->depth = (vb->depth + 7) & 0xf8;
	vb->bytesperline = rwidth*vb->depth/8;
	g_print("%d\n",vb->bytesperline);
	g_print("%d\n",vb->depth);
	g_print("%d\n",rwidth);
	return 0;
}
