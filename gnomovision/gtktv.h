#ifndef __GTK_TV_H__
#define __GTK_TV_H__

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>


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
  GtkTVPrivate *p;
};

struct _GtkTVClass
{
  GtkWidgetClass parent_class;
};

guint          gtk_tv_get_type        (void);
GtkWidget*     gtk_tv_new             (int video_num);

/* To do the DGA stuff, you need root privs. This is here to remind you to
   be security-concious and release them ASAP */
#define gtk_tv_release_priviledges() seteuid(getuid())

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_EVENT_BOX_H__ */
