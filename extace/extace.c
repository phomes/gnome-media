/*
 * GTK/GDK/GNOME sound (esd) system output display program
 * 
 * Copyright (C) 1998 by Michael Fullbright
 * Re-hacked to look good by The Rasterman
 * 
 * This software comes under thr GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <esd.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk_imlib.h>

GtkWidget     *disp=NULL;
GdkWindow     *win=NULL;
GdkGC         *gc=NULL;
GdkPixmap     *pixmap=NULL;
gint           width = -1;
gint           height = -1;
gint           colortab[32][64];
gint           mode=0;
GdkImlibImage *im=NULL;
GdkPixmap     *grad[256];

/* NSAMP is number of samples per FFT */
/* RATE  is sample rate (samples/sec) */
/* DELAY is delay between updates, is related to sample speed */
#define NSAMP  1024
#define BUFS   8
#define RATE   44100
#define DELAY  (NSAMP*1000)/RATE
short          aubuf[BUFS][NSAMP];
double        *datawindow=NULL;
double         datawindowsum=0.0;
double         audata[NSAMP*2];
double         audata_backup[NSAMP];
double         aufft[NSAMP];
guint          auval[NSAMP];
gint           sound=-1;
gint           freq=1;
gint           curbuf=0;
gint           lag=5;

void 
open_sound(void)
{
   sound=esd_monitor_stream(ESD_BITS16|ESD_MONO|ESD_STREAM|ESD_PLAY,RATE,NULL,"extace");
   if (sound<0)
     {
	g_error("Cannot connect to EsounD\n");
	exit(1);
     }
}

void
setup_datawindow(void)
{
   gint i;
   double *w;
   double sumw;
   
   w = g_malloc(NSAMP*sizeof(double));
   
   /* add in i=N for window function first */
   sumw = 1.0 - fabs((NSAMP - 0.5*(NSAMP-1.0))/(0.5*(NSAMP+1.0)));
   sumw *= sumw;
   
#ifdef WINDOW_DEBUG
   f = fopen("/tmp/window.in", "w");
#endif
   for (i=0; i<NSAMP; i++) {
      w[i] = 1.0 - fabs((i - 0.5*(NSAMP-1.0))/(0.5*(NSAMP+1.0)));
      sumw += w[i]*w[i];
#ifdef WINDOW_DEBUG
      fprintf(f, "%d %f %f\n", i, w[i], sumw);
#endif
   }
#ifdef WINDOW_DEBUG
   fclose(f);
#endif
   
   sumw *= NSAMP;
#ifdef WINDOW_DEBUG
   fprintf(stderr, "window sum is %f\n",sumw);
#endif
   datawindowsum = sumw;
   datawindow    = w;
}

/*#define TEST_SIGNAL 1*/

int
GetFFT(void)
{
   gint buf;
   short *ptr;
   double *w,*aup,*aup2,*fftp,*aupend;
   
   curbuf++;
   if (curbuf>=BUFS) curbuf=0;
   read(sound, aubuf[curbuf], NSAMP*2);
   
   buf=((BUFS*2)+curbuf-lag)%BUFS;
   if ((curbuf%2)>0) return 0;
   
   if (mode!=2)
     {
	ptr=aubuf[buf];
	w=datawindow;
	aup=audata;
	aupend=aup+NSAMP;
	while(aup<aupend)
	  *aup++=((*w++)*(double)(*ptr++))/32768.0;
     }
   ptr=aubuf[buf];
   aup=audata_backup;
   aupend=aup+NSAMP;
   while(aup<aupend)
     *aup++=((double)(*ptr++))/32768.0;

   if (mode==2) return 1;
   /* do the FFT */
   gsl_fft_real_radix2(audata, NSAMP);
   aufft[0] = (audata[0]*audata[0])/(NSAMP*NSAMP);
   aufft[NSAMP/2] = (audata[NSAMP/2]*audata[NSAMP/2])/(NSAMP*NSAMP);
   /* take the log */
   aup=audata;
   aup2=audata+NSAMP-1;
   aupend=aup+NSAMP;
   fftp=aufft;
   while(aup<aupend)
     {
	*fftp=8+log10((((*aup * *aup)+(*aup2 * *aup2)))/(NSAMP*NSAMP));
	if (*fftp<0) *fftp=0;
	fftp++;
	aup++;aup2--;
     }
   return 1;
}

gint 
disp_configure(GtkWidget *widget, GdkEventConfigure *event)
{
   if (win)
     {
	if ((width!=event->width)||(height!=event->height))
	  {
	     width=event->width;
	     height=event->height;
	     if (pixmap)
	       gdk_pixmap_unref(pixmap);
	     pixmap=gdk_pixmap_new(win,width,height,
				   gtk_widget_get_visual(disp)->depth);
	     gdk_draw_rectangle( pixmap,
				disp->style->black_gc,
				TRUE, 0,0,
				width,height);
	     gdk_window_set_back_pixmap(win,pixmap,0);
	  }
     }
   return TRUE;
}

#define BACKING 1
#define LEVELS 32
#define LWIDTH NSAMP/(2*LEVELS)

void
draw(void)
{
   gint i,j,k;
   static double levels[LEVELS]=
     {
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0};
   static double plevels[LEVELS]=
     {
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0};
   static gint bin[LEVELS];
   double val;
   gchar slide=0;
   GdkPoint pt[4];
   gint scroll=8;
   gint size=8;
   GdkColor cl;
   gint br,lvl;
   
   if (!GetFFT()) return;

   if (!win) return;
   for(i=0;i<32;i++)
     {
	bin[i]=LWIDTH-(15-(double)i)*0.95;
/*	bin[i]=LWIDTH;*/
     }
   j=0;
   for (i=1; i<NSAMP/2; i++)
/*     auval[i-1] =  (aufft[i]*20000)*(double)height;*/
     auval[i-1] =  (aufft[i]/20)*(double)height;
   auval[(NSAMP/2)-1]=0;
   k=0;
   for(i=0;i<LEVELS;i++)
     {
	val=0;
	for(j=0;j<(bin[i]);j++)
	  {val+=auval[k++];}
	val/=(double)bin[i];
	if (slide)
	  {
	     if (val<=(levels[i]-4))
	       levels[i]-=4;
	     else 
	       levels[i]=val;
	  }
	else
	  levels[i]=val;
     }
   switch (mode)
     {
      case 0:
	gdk_window_copy_area(win,gc,0,0,win,scroll,0,width-scroll,height);
	gdk_draw_rectangle( pixmap,
			   disp->style->black_gc,
			   TRUE, width-scroll,0,
			   scroll,height);
	for( i=0; i < LEVELS - 1; i++ ) 
	  {   
	     pt[0].x=width-(i*size)-(scroll*2);
	     pt[0].y=height-(((LEVELS-1-i))*size/2)-(gint)plevels[i];
	     pt[1].x=width-(i*size)-(scroll);
	     pt[1].y=height-(((LEVELS-1-i))*size/2)-(gint)levels[i];
	     pt[2].x=width-(i*size)-(scroll*2);
	     pt[2].y=height-(((LEVELS-1-(i+1)))*size/2)-(gint)levels[i+1];
	     pt[3].x=width-(i*size)-(scroll*3);
	     pt[3].y=height-(((LEVELS-1-(i+1)))*size/2)-(gint)plevels[i+1];
	     lvl=(gint)levels[i]/2;
	     if (lvl>63) lvl=63;
	     br=16+(gint)(levels[i]-plevels[i+1]);
	     
	     if (br<0) br=0;
	     else if (br>31) br=31;
	     cl.pixel=colortab[br][lvl];
	     gdk_gc_set_foreground(gc,&cl);
	     gdk_draw_polygon(win,gc,TRUE,pt,4);
	     gdk_draw_line(win,disp->style->black_gc,
			   pt[0].x,pt[0].y,pt[1].x,pt[1].y);
	     gdk_draw_line(win,disp->style->black_gc,
			   pt[0].x,pt[0].y,pt[3].x,pt[3].y);
	     pt[0].x=width-(i*size)-(scroll);
	     pt[0].y=height-(((LEVELS-1-i))*size/2);
	     pt[1].x=width-(i*size)-(scroll);
	     pt[1].y=height-(((LEVELS-1-i))*size/2)-(gint)levels[i];
	     pt[2].x=width-(i*size)-(scroll*2);
	     pt[2].y=height-(((LEVELS-1-(i+1)))*size/2)-(gint)levels[i+1];
	     pt[3].x=width-(i*size)-(scroll*2);
	     pt[3].y=height-(((LEVELS-1-(i+1)))*size/2);
	     gdk_draw_polygon(win,gc,TRUE,pt,4);
	     gdk_draw_line(win,disp->style->black_gc,
			   pt[2].x,pt[2].y,pt[1].x,pt[1].y);
	  }
	break;
      case 1:
	gdk_draw_rectangle( pixmap,
			   disp->style->black_gc,
			   TRUE, 0,0,
			   width,height);
	for( i=0; i < LEVELS; i++ ) 
	  {   
	     lvl=(gint)levels[i]/2;
	     if (lvl>63) lvl=63;
	     br=16+(gint)(levels[i]-plevels[i+1]);
	     
	     if (br<0) br=0;
	     else if (br>31) br=31;
	     cl.pixel=colortab[br][lvl];
	     gdk_gc_set_foreground(gc,&cl);
	     gdk_draw_rectangle( pixmap,gc,
				TRUE, 
				(i*LWIDTH)+1,height-((gint)levels[i]*3)+1,
				LWIDTH-2,(gint)(levels[i]*3)-2);
	  }
	for(i=0;i<height;i+=8)
	  {
	     gdk_draw_rectangle( pixmap,disp->style->black_gc,
				TRUE, 
				0,i,
				width,2);
	  }
	gdk_window_clear(win);
	break;
      case 2:
	gdk_draw_rectangle( pixmap,
			   disp->style->black_gc,
			   TRUE, 0,0,
			   width,height);
	for(i=0;i<NSAMP;i+=2)
	  {
	     j=(gint)(audata_backup[i]*127)+127;
	     if (j<0) j=0;
	     else if (j>255) j=255;
	     if (j<127)
	       gdk_draw_pixmap(pixmap,gc,grad[j],0,0,
			       i/2,j,1,127-j);
	     else if (j>127)
	       gdk_draw_pixmap(pixmap,gc,grad[j],0,0,
			       i/2,127,1,j-127);
	  }
	gdk_window_clear(win);
	break;
      case 3:
	scroll=1;
	size=1;
	gdk_window_copy_area(win,gc,0,0,win,scroll,0,width-scroll,height);
	gdk_draw_rectangle( pixmap,
			   disp->style->black_gc,
			   TRUE, width-scroll,0,
			   scroll,height);
	for( i=0; i < NSAMP/4 ; i++ ) 
	  {   
	     pt[0].x=width-((i*size)-(scroll))/2-4;
	     pt[0].y=height-((((NSAMP/4)-1-i))*size/4);
	     pt[1].x=width-((i*size)-(scroll))/2-4;
	     pt[1].y=height-((((NSAMP/4)-1-i))*size/4)-(gint)auval[i];
	     lvl=(gint)auval[i];
	     if (lvl>63) lvl=63;

	     cl.pixel=colortab[16][lvl];
	     gdk_gc_set_foreground(gc,&cl);
	     gdk_draw_line(win,gc,pt[0].x,pt[0].y,pt[1].x,pt[1].y);
	  }
	break;
      default:
	break;
     }
   gdk_flush();
   for( i=0; i < LEVELS; i++ ) 
     plevels[i]=levels[i];
#if BACKING
#else

#endif   
}

void
init_colortab(void)
{
   gint i,j;
   gint r,g,b;
   gint rr,gg,bb;
   gint cr[64],cg[64],cb[64];
   unsigned char *data;
   
   j=0;
   r=30;g=0;b=160;
   rr=r;gg=g;bb=b;
   r=160;g=40;b=140;
   for(i=0;i<16;i++)
     {
	cr[j]=(((15-i)*rr)+(i*r))/15;
	cg[j]=(((15-i)*gg)+(i*g))/15;
	cb[j]=(((15-i)*bb)+(i*b))/15;
	j++;
     }
   rr=r;gg=g;bb=b;
   r=210;g=130;b=20;
   for(i=0;i<16;i++)
     {
	cr[j]=(((15-i)*rr)+(i*r))/15;
	cg[j]=(((15-i)*gg)+(i*g))/15;
	cb[j]=(((15-i)*bb)+(i*b))/15;
	j++;
     }
   rr=r;gg=g;bb=b;
   r=240;g=200;b=20;
   for(i=0;i<16;i++)
     {
	cr[j]=(((15-i)*rr)+(i*r))/15;
	cg[j]=(((15-i)*gg)+(i*g))/15;
	cb[j]=(((15-i)*bb)+(i*b))/15;
	j++;
     }
   rr=r;gg=g;bb=b;
   r=255;g=240;b=80;
   for(i=0;i<16;i++)
     {
	cr[j]=(((15-i)*rr)+(i*r))/15;
	cg[j]=(((15-i)*gg)+(i*g))/15;
	cb[j]=(((15-i)*bb)+(i*b))/15;
	j++;
     }
   for(i=0;i<32;i++)
     {
	for(j=0;j<64;j++)
	  {
	     r=cr[j]+((i-16)*4);
	     g=cg[j]+((i-16)*4);
	     b=cb[j]+((i-16)*4);
	     if (r<0) r=0;
	     else if (r>255) r=255;
	     if (g<0) g=0;
	     else if (g>255) g=255;
	     if (b<0) b=0;
	     else if (b>255) b=255;
	     colortab[i][j]=gdk_imlib_best_color_match(&r,&g,&b);
	  }
     }
   data=malloc(64*3);
   for(i=0;i<64;i++)
     {
	data[(i*3)]=cr[63-i];
	data[(i*3)+1]=cg[63-i];
	data[(i*3)+2]=cb[63-i];
     }
   im=gdk_imlib_create_image_from_data(data,NULL,1,64);
   for(i=128;i<256;i++)
     {
	gdk_imlib_render(im,1,i-127);
	grad[i]=gdk_imlib_move_image(im);
     }
   gdk_imlib_flip_image_vertical(im);
   for(i=0;i<127;i++)
     {
	gdk_imlib_render(im,1,127-i);
	grad[i]=gdk_imlib_move_image(im);
     }
   gdk_imlib_kill_image(im);
}

gint 
leave(GtkWidget *widget, gpointer *data)
{
   exit(0);
}

gint 
button_3d_fft(GtkWidget *widget, gpointer *data)
{
   gdk_draw_rectangle( pixmap,
		      disp->style->black_gc,
		      TRUE, 0,0,
		      width,height);
   gdk_window_clear(win);
   mode=0;

   return 0;
}

gint 
button_2d_fft(GtkWidget *widget, gpointer *data)
{
   gdk_draw_rectangle( pixmap,
		      disp->style->black_gc,
		      TRUE, 0,0,
		      width,height);
   gdk_window_clear(win);
   mode=1;

   return 0;
}

gint 
button_oscilloscope(GtkWidget *widget, gpointer *data)
{
   gdk_draw_rectangle( pixmap,
		      disp->style->black_gc,
		      TRUE, 0,0,
		      width,height);
   gdk_window_clear(win);
   mode=2;

   return 0;
}

gint 
button_3d_detailed(GtkWidget *widget, gpointer *data)
{
   gdk_draw_rectangle( pixmap,
		      disp->style->black_gc,
		      TRUE, 0,0,
		      width,height);
   gdk_window_clear(win);
   mode=3;

   return 0;
}

int 
main(int argc, char **argv)
{
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *window;
   GtkWidget *button;
   
   gtk_init(&argc, &argv);
   gdk_imlib_init();
   gtk_widget_push_visual(gdk_imlib_get_visual());
   gtk_widget_push_colormap(gdk_imlib_get_colormap());
   
   window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(window),"ExtacE");
			gtk_container_border_width(GTK_CONTAINER(window),0);
   gtk_window_set_policy(GTK_WINDOW(window),1,1,1);
   gtk_signal_connect(GTK_OBJECT(window),"delete_event",
		      GTK_SIGNAL_FUNC(leave),NULL);
   gtk_widget_show(window);

   vbox=gtk_vbox_new(FALSE,0);
   gtk_container_add(GTK_CONTAINER(window), vbox);
   gtk_widget_show(vbox);
   hbox=gtk_hbox_new(TRUE,0);
   gtk_widget_show(hbox);
   
   button=gtk_button_new_with_label("3D FFT");
   gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,0);
   gtk_widget_show(button);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
		      GTK_SIGNAL_FUNC(button_3d_fft),NULL);

   button=gtk_button_new_with_label("Flat FFT");
   gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,0);
   gtk_widget_show(button);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
		      GTK_SIGNAL_FUNC(button_2d_fft),NULL);

   button=gtk_button_new_with_label("Oscilloscope");
   gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,0);
   gtk_widget_show(button);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
		      GTK_SIGNAL_FUNC(button_oscilloscope),NULL);

   button=gtk_button_new_with_label("3D Detailed FFT");
   gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,0);
   gtk_widget_show(button);
   gtk_signal_connect(GTK_OBJECT(button),"clicked",
		      GTK_SIGNAL_FUNC(button_3d_detailed),NULL);
   
   gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
   disp=gtk_drawing_area_new();
   gtk_widget_show(disp);
   gtk_signal_connect( GTK_OBJECT(disp),"configure_event",
		      (GtkSignalFunc)disp_configure, NULL);
   gtk_box_pack_start(GTK_BOX(vbox),disp,TRUE,TRUE,0);
   gtk_widget_realize(disp);
   height = 256;
   width  = 512;
   gtk_widget_set_usize(disp,width,height);
   win=disp->window;
   gc=gdk_gc_new(win);
   gdk_gc_copy(gc,disp->style->white_gc);
   pixmap=gdk_pixmap_new(win,width,height,
			 gtk_widget_get_visual(disp)->depth);
   gdk_draw_rectangle( pixmap,
		      disp->style->black_gc,
		      TRUE, 0,0,
		      width,height);
   gdk_window_set_back_pixmap(win,pixmap,0);
   setup_datawindow();
   init_colortab();
   open_sound();
   for (;;)
     {
	while (gtk_events_pending()) gtk_main_iteration();
	draw();
     }
   return 0;
}
