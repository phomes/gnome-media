#ifndef __GTK_TV_H__
#define __GTK_TV_H__

#include <gdk/gdk.h>
#include <gdk_imlib.h>
#include <gtk/gtkwidget.h>
#include <linux/types.h>
#include <linux/videodev.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_TV(obj)          GTK_CHECK_CAST (obj, gtk_tv_get_type (), GtkTV)
#define GTK_TV_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_tv_get_type (), GtkTVClass)
#define GTK_IS_TV(obj)       GTK_CHECK_TYPE (obj, gtk_tv_get_type ())


typedef struct _GtkTV       GtkTV;
typedef struct _GtkTVClass  GtkTVClass;

typedef struct _GtkTVPrivate GtkTVPrivate;

struct _GtkTV
{
  GtkWidget parent_object;

  /*** These are set up at object creation time ***/
  int fd;
  struct video_capability vcap; /* The capabilities of this capture device */
  struct video_buffer vbuf; /* The current information we have on where
			       the video buffer goes */
  GdkColor chroma_colour;
  /* Misc flags */
  gboolean use_shm, kill_on_overlap, clean_display, need_colour_cube,
    blocking_events;
  gint visible;

  /*** These are initialized at object creation, of course, but are
       set on the fly as people change settings ***/
  struct video_audio vaudio;
  struct video_picture vpic;
  struct video_window vwindow;
  struct video_tuner vtuner;

  guint curinput;

  struct video_clip clips[256];
  GtkWidget *rootwin;

  GtkWidget *oldtoplevel;
  gint toplevel_config_id; /* The signal id for
				 telling us when our toplevel has moved
				 around or whatever */
  gint toplevel_visibility_id;
  struct video_buffer cbuf;
  struct video_window cwin;
  struct video_picture cpic;
};

struct _GtkTVClass
{
  GtkWidgetClass parent_class;
};

guint          gtk_tv_get_type        (void);
GtkWidget*     gtk_tv_new             (int video_num);

/* There's no easy way to do this - this is the easiest for me, writing
   GtkTV, but it requires a little bit of work on the part of the app writer */
void           gtk_tv_set_toplevel    (GtkTV *tv);

/* In V4L inputs are called "channels", don't ask me why :) */
guint          gtk_tv_get_num_inputs  (GtkTV *tv);

void           gtk_tv_set_input       (GtkTV *tv, guint inputnum);

/* Sets the audio for the current input */
void           gtk_tv_set_sound      (GtkTV *tv,
				      /* For all the params, passing in
					 -1 signifies "no change" */
				      gfloat volume, /* between 0 & 1 */
				      gfloat bass,
				      gfloat treble,
				      gint flags /* VIDEO_AUDIO_* */,
				      gint mode /* VIDEO_MODE_* */);
void           gtk_tv_set_picture    (GtkTV *tv,
				      /* For all the params, passing in
					 -1 signifies "no change" */
				      gfloat brightness, /* between 0 & 1 */
				      gfloat hue,
				      gfloat colour,
				      gfloat contrast,
				      gfloat whiteness);

/* Broken, doesn't work, blah blah. */
GdkImlibImage *gtk_tv_grab_image(GtkTV *tv, gint width, gint height);

typedef enum {
  GTK_TV_FORMAT_PAL,
  GTK_TV_FORMAT_NTSC,
  GTK_TV_FORMAT_SECAM,
  GTK_TV_FORMAT_AUTO
} GtkTVFormat; 

void gtk_tv_set_format(GtkTV *tv, GtkTVFormat fmt);

/* To do the DGA stuff, you need root privs. This is here to remind you to
   be security-concious and release them ASAP */
#define gtk_tv_release_privileges() seteuid(getuid())

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_TV_H__ */


