/*
 *	Sections of the TVset program dealing with Video4Linux low
 *	level interfaces. Strictly no user interface beyond this point.
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

#include <linux/types.h>
#include <linux/videodev.h>

#include <gtk/gtk.h>

#include "gnomovision.h"

#include "channels.h"

struct video_window vwin;
struct video_picture vpic;
struct video_buffer vbuf;
struct video_capability vcap;
struct video_audio vaudio;
struct video_tuner vt;
unsigned long vfrequency;
unsigned long vbase_freq;
int vchannel;
int grabber_format;
int capture_running;
int capture_hidden;	/* Dont capture we are hidden */
int tv_fd = -1;

int kill_on_overlap = 0;	/* Set for overlay cards that dont clip */
int clean_display = 0;		/* Set on overlay cards that do DMA */
int chroma_key = 0;		/* Set for chromakey cards */
int fixed_size = 0;		/* Size is fixed by the card (we dont scale yet) */
int need_colour_cube = 0;	/* Set for colour cubes 0 - no 1 - 240 colour set for bt848 */
int need_greymap = 0;		/* Greymap needed or depth */
int use_shm = 0;		/* Capture only card */
int xsize = 320, ysize = 200;	/* Initial size */
int xmin, ymin;			/* Limits */
int xmax, ymax;	


/*
 *	TV Channel Handler
 */
 
unsigned long channel_compute(int freq)
{
	int i = PAL_UHF_GHI /*ch_type;*/;
	freq = tvtuner[freq].freq[i];
	if (!freq)	/* no need to set invalid freq */
	    return 0;
	freq *= 16;
	freq /= 1000;
	return freq;
}
 
char *probe_tv_set(int n)
{
	static struct video_capability t;
	char buf[64];
	int fd;
	sprintf(buf,"/dev/video%d", n);
	
	fd=open(buf, O_RDWR);
	if(fd==-1)
		return NULL;
		
	if(ioctl(fd, VIDIOCGCAP, &t)==-1)
	{
		close(fd);
		return NULL;
	}
	close(fd);
	return t.name;
}

int open_tv_card(int n)
{
	int fd;
	char buf[64];
	
	sprintf(buf,"/dev/video%d",n);
	
	fd=open(buf,O_RDWR);
	
	
	if(fd==-1)
	{
		perror("open");
		return -1;
	}
	
	if(ioctl(fd, VIDIOCGAUDIO, &vaudio)==0)
	{
		vaudio.flags|=VIDEO_AUDIO_MUTE;
		ioctl(fd, VIDIOCSAUDIO, &vaudio);
	}

	if(ioctl(fd, VIDIOCGCAP, &vcap))
	{
		perror("get capabilities");
		return -1;
	}
	
	/*
	 *	Use black as the background unless we later find a 
	 *	chromakey device
	 */

	chroma_key = 0;	 
	 
	/* 
	 *	Now see what we support
	 */
	 
	if(!(vcap.type&VID_TYPE_OVERLAY))
	{
		if(vcap.type&VID_TYPE_CAPTURE)
			use_shm = 1;
		else
		{
			fprintf(stderr,"I don't know how to handle this device.\n");
			return -1;
		}
	}
	else
	{
		/* Ok now see what method to use */
		if(vcap.type&VID_TYPE_CLIPPING)
		{
		}
		else if(vcap.type&VID_TYPE_CHROMAKEY)
		{
			chroma_key = 1;
		}
		else
			kill_on_overlap = 1;
		if(vcap.type&VID_TYPE_FRAMERAM)
			clean_display = 1;
		
	}
	
	/*
	 *	Set the window size constraints
	 */
	 
	if(!(vcap.type&VID_TYPE_SCALES))
	{
		xmin = xmax = xsize = vcap.maxwidth;
		ymin = ymax = ysize = vcap.maxheight;
		fixed_size = 1;
	}
	else
	{
		xmin = vcap.minwidth;
		ymin = vcap.minheight;
		xmax = vcap.maxwidth;
		ymax = vcap.maxheight;
		xsize = min(max(xsize, xmin), xmax);
		ysize = min(max(ysize, ymin), ymax);
	}

	/*
	 *	Now get the video parameters (one day X will set these)
	 */

	video_ll_mapping(&vbuf);	 
	
	/*
	 *	Set the overlay buffer up
	 */
	 
	if(ioctl(fd, VIDIOCSFBUF, &vbuf)==-1)
	{
		perror("Set frame buffer");
		/*
		 *	If we can't use MITSHM then we can't overlay if
		 *	the board can't handle our framebuffer...
		 *
		 *	For SHM we can do post processing.
		 */
		 
		if(!use_shm)
			return -1;
	}
	
	if(seteuid(getuid())==-1)
	{
		perror("seteuid");
		exit(1);
	}
	
	/*
	 *	Set the format
	 */
 
	ioctl(fd, VIDIOCGPICT , &vpic);

	if(use_shm)
	{	
	
		/*
		 *	Try 24bit RGB, then 565 then 555. Arguably we ought to
		 *	try one matching our visual first, then try them in 
		 *	quality order
		 *
		 */
		
		
		if(vcap.type&VID_TYPE_MONOCHROME)
		{
			vpic.depth=8;
			vpic.palette=VIDEO_PALETTE_GREY;	/* 8bit grey */
			if(ioctl(fd, VIDIOCSPICT, &vpic)==-1)
			{
				vpic.depth=6;
				if(ioctl(fd, VIDIOCSPICT, &vpic)==-1)
				{
					vpic.depth=4;
					if(ioctl(fd, VIDIOCSPICT, &vpic)==-1)
					{
						fprintf(stderr, "Unable to find a supported capture format.\n");
						return -1;
					}
				}
			}
			/*
			 *	Make a grey scale cube - only go to 6bit
			 */
			 
			if(vpic.depth==4)
				need_greymap = 16;
			else
				need_greymap = 64;
		}
		else
		{
			vpic.depth=24;
			vpic.palette=VIDEO_PALETTE_RGB24;
		
			if(ioctl(fd, VIDIOCSPICT, &vpic)==-1)
			{
				vpic.palette=VIDEO_PALETTE_RGB565;
				vpic.depth=16;
				
				if(ioctl(fd, VIDIOCSPICT, &vpic)==-1)
				{
					vpic.palette=VIDEO_PALETTE_RGB555;
					vpic.depth=15;
				
					if(ioctl(tv_fd, VIDIOCSPICT, &vpic)==-1)
					{
						fprintf(stderr, "Unable to find a supported capture format.\n");
						return -1;
					}
				}
			}
		}
	}
	/*
	 *	Soft overlays by chroma etc we dont have to care about
	 *	the format for.. but DMA to frame buffer we do
	 */
	else if(vcap.type&VID_TYPE_FRAMERAM)
	{
		vpic.depth = vbuf.depth;
		switch(vpic.depth)
		{
			case 8:
				vpic.palette=VIDEO_PALETTE_HI240;	/* colour cube */
				need_colour_cube=1;
				break;
			case 15:
				vpic.palette=VIDEO_PALETTE_RGB555;
				break;
			case 16:
				vpic.palette=VIDEO_PALETTE_RGB565;
				break;
			case 24:
				vpic.palette=VIDEO_PALETTE_RGB24;
				break;
			case 32:		
				vpic.palette=VIDEO_PALETTE_RGB32;
				break;
			default:
				fprintf(stderr,"Unsupported X11 depth.\n");
		}
		if(ioctl(fd, VIDIOCSPICT, &vpic)==-1)
		{
			/* Depth mismatch is fatal on overlay */
			perror("set depth");
			return -1;
		}
	}
		
	if(ioctl(fd, VIDIOCGPICT, &vpic)==-1)
		perror("get picture");
			
	grabber_format = vpic.palette;
	
	/*
	 *	See if we have a frequency 
	 */
	 
	ioctl(fd, VIDIOCGFREQ, &vbase_freq);
	
	return fd;
}	

void close_tv_card(int handle)
{
	close(handle);
}

void set_tv_frequency(long value)
{
	if(ioctl(tv_fd, VIDIOCSFREQ, &vfrequency)==-1)
		perror("set frequency");
}

void set_tv_picture(struct video_picture *vp)
{
	if(ioctl(tv_fd, VIDIOCSPICT, vp)==-1)
		perror("set picture");
}

