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
	 
	vb->depth = gdk_visual_get_best_depth();
	vb->bytesperline = rwidth*vb->depth/8;
	return 0;
}
