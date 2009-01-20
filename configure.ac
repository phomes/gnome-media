AC_PREREQ(2.60)

AC_INIT([gnome-media],
        [2.25.5],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=gnome-media])

AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-bzip2 -Wall -Wno-portability])

AS_VERSION

AM_MAINTAINER_MODE

if type -p stow > /dev/null && test -d /usr/local/stow ; then
   AC_MSG_NOTICE([*** Found /usr/local/stow: default install prefix set to /usr/local/stow/${PACKAGE}-${VERSION} ***])
   ac_default_prefix="/usr/local/stow/${PACKAGE}-${VERSION}"
fi

AM_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

IT_PROG_INTLTOOL([0.35.0])

AC_PROG_CC
AC_ISC_POSIX

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h strings.h sys/ioctl.h unistd.h])
AC_CHECK_HEADERS([sys/sockio.h sys/param.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MMAP

# CFLAGS
GNOME_COMPILE_WARNINGS

# AS_NANO(GM_RELEASE="yes", GM_RELEASE="no")
AS_COMPILER_FLAG(-Wall, GM_ERROR_CFLAGS="-Wall")

if test "x$GM_RELEASE" = "xno"
then
  AS_COMPILER_FLAG(-Werror, GM_ERROR_CFLAGS="$GM_ERROR_CFLAGS -Werror")
fi
AC_MSG_NOTICE([Using $GM_ERROR_CFLAGS as error checking CFLAGS])

dnl=======================================================================
dnl Start of pkg-config checks - common libs
dnl=======================================================================

DBUS_GLIB_REQUIRED_VERSION=0.74
GLIB_REQUIRED_VERSION=2.18.2
GTK_REQUIRED_VERSION=2.10.0
LIBGLADE_REQUIRED_VERSION=1.99.2
GCONF_REQUIRED_VERSION=2.6.1
PA_REQUIRED_VERSION=0.9.12
CANBERRA_REQUIRED_VERSION=0.4

PKG_CHECK_MODULES(MEDIA, [
        glib-2.0 >= $GLIB_REQUIRED_VERSION
        gmodule-2.0 >= $GLIB_REQUIRED_VERSION
        gtk+-2.0 >= $GTK_REQUIRED_VERSION
])

host=`uname -s`
case "$host" in
*SunOS*)
        MEDIA_LIBS="$MEDIA_LIBS -ldevinfo -lX11"
        ;;
esac

AC_SUBST(MEDIA_CFLAGS)
AC_SUBST(MEDIA_LIBS)

dnl=======================================================================
dnl Check for the new volume control modules
dnl=======================================================================

PKG_CHECK_MODULES(VOLUME_CONTROL,
        dbus-glib-1 >= $DBUS_GLIB_REQUIRED_VERSION
        gobject-2.0 >= $GLIB_REQUIRED_VERSION
        gtk+-2.0 >= $GTK_REQUIRED_VERSION
        gio-2.0
        gconf-2.0 >= $GCONF_REQUIRED_VERSION
        libcanberra-gtk >= $CANBERRA_REQUIRED_VERSION
        libxml-2.0
)
AC_SUBST(VOLUME_CONTROL_CFLAGS)
AC_SUBST(VOLUME_CONTROL_LIBS)

PKG_CHECK_MODULES(PULSEAUDIO,
        libpulse >= $PA_REQUIRED_VERSION libpulse-mainloop-glib >= $PA_REQUIRED_VERSION,
        have_pulseaudio=yes,
        have_pulseaudio=no)

AC_ARG_ENABLE([pulseaudio],
              AS_HELP_STRING([--enable-pulseaudio],
                             [Enable PulseAudio support @<:@default=auto@:>@]),
              enable_pulseaudio=$enableval, enable_pulseaudio=auto)

if test "x$enable_pulseaudio" != "xno"; then
  if test "x$enable_pulseaudio" = "xyes" -a "x$have_pulseaudio" = "xno"; then
    AC_MSG_ERROR([PulseAudio support explicitly requested but dependencies not found])
  fi

  if test "x$have_pulseaudio" = "xyes" ; then
    AC_DEFINE(HAVE_PULSEAUDIO, [], [Define if we have pulseaudio])
  fi
fi
AM_CONDITIONAL(HAVE_PULSEAUDIO, test x$have_pulseaudio = xyes)
AC_SUBST(HAVE_PULSEAUDIO)
AC_SUBST(PULSEAUDIO_CFLAGS)
AC_SUBST(PULSEAUDIO_LIBS)

dnl=======================================================================
dnl Check for the CDDBSlave2 modules
dnl=======================================================================

have_cddbslave=no

AC_ARG_ENABLE([cddbslave],
  AS_HELP_STRING([--enable-cddbslave], [enable gnome cd database]),
  ,enable_cddbslave=no)

if test "x$enable_cddbslave" != "xno"; then
  PKG_CHECK_MODULES(CDDBSLAVE, [
    glib-2.0 >= 1.3.7
    gmodule-2.0 >= 1.3.7
    libbonobo-2.0 >= 2.0.0
    libgnomeui-2.0 >= 1.102.0],
    have_cddbslave=yes)
fi

# Check for network stuff
AC_CHECK_FUNCS(poll basename)
AC_CHECK_FUNC(inet_aton,,[AC_CHECK_LIB(resolv,inet_aton,CDDBSLAVE_LIBS="$CDDBSLAVE_LIBS -lresolv")])
AC_CHECK_FUNC(socket,,[AC_CHECK_LIB(socket,socket,CDDBSLAVE_LIBS="$CDDBSLAVE_LIBS -lsocket")])
AC_CHECK_FUNC(gethostbyname,,[AC_CHECK_LIB(nsl,gethostbyname,CDDBSLAVE_LIBS="$CDDBSLAVE_LIBS -lnsl")])

dnl=======================================================================
dnl                        IPv6 related checks
dnl=======================================================================

have_ipv6=no

AC_ARG_ENABLE(ipv6,
  AS_HELP_STRING([--enable-ipv6],[enable IPv6 extension]),
  ,enable_ipv6=yes)

if test "x$enable_ipv6" = "xyes"; then
  AC_MSG_CHECKING([whether to enable IPv6])
  AC_TRY_COMPILE([
    #include <sys/types.h>
    #include <sys/socket.h>], [
    struct sockaddr_storage ss;
    socket(AF_INET6, SOCK_STREAM, 0)
    ], have_ipv6=yes)
  AC_MSG_RESULT($have_ipv6)

  if test "x$have_ipv6" = "xyes"; then
    have_getaddrinfo=no
    AC_CHECK_FUNC(getaddrinfo, have_getaddrinfo=yes)

    if test "x$have_getaddrinfo" = "xno"; then
      for lib in bsd socket inet; do
        AC_CHECK_LIB($lib,getaddrinfo,[LIBS="$LIBS -l$lib";have_getaddrinfo=yes;break])
      done
    fi

    if test "x$have_getaddrinfo" = "xyes"; then
      AC_DEFINE(ENABLE_IPV6, 1, [Define if we have IPv6 support])
    fi
  fi
fi

dnl=======================================================================
dnl                        IPv6 related checks ends..
dnl=======================================================================

AC_SUBST(CDDBSLAVE_CFLAGS, "$CDDBSLAVE_CFLAGS $GM_ERROR_CFLAGS")
AC_SUBST(CDDBSLAVE_LIBS)
AM_CONDITIONAL(HAVE_CDDBSLAVE, [test x$have_cddbslave = xyes])

dnl=======================================================================
dnl profiles
dnl=======================================================================

have_profiles=no

AC_ARG_ENABLE([profiles],
  AS_HELP_STRING([--enable-profiles], [enable gnome media profiles]),
  ,enable_profiles=yes)

if test "x$enable_profiles" = "xyes"; then
  PKG_CHECK_MODULES(GMP, [
    gstreamer-0.10
    gconf-2.0
    libglade-2.0
    libgnomeui-2.0
    gio-2.0],
    have_profiles=yes)
fi
AC_SUBST(GMP_CFLAGS)
AC_SUBST(GMP_LIBS)
AM_CONDITIONAL(HAVE_GMP, [test x$have_profiles = xyes])

dnl=======================================================================
dnl Check for GStreamer and GSR deps
dnl=======================================================================

GST_REQ=0.10.3
GST_PLUGINS_BASE_REQ=0.10.3
GST_MAJORMINOR=0.10

dnl make available to Makefile.am
AC_SUBST(GST_MAJORMINOR)
AC_DEFINE_UNQUOTED(GST_MAJORMINOR_QUOTED, "$GST_MAJORMINOR",
  [major/minor of GStreamer as a string constant])

have_gst=no

PKG_CHECK_MODULES(GSR, [
  gstreamer-$GST_MAJORMINOR >= $GST_REQ
  gstreamer-plugins-base-$GST_MAJORMINOR >= $GST_REQ
  gtk+-2.0 >= 2.6.0
  libgnomeui-2.0 >= 1.102.0
  gio-2.0],
  have_gst=yes,
  [
    AC_MSG_RESULT(no)
    AC_MSG_WARN($GSR_PKG_ERRORS)
    PROGRAMS_GRECORD_ERROR="$GSR_PKG_ERRORS"
  ])

have_grecord=no
grecord_reason=""

AC_ARG_ENABLE([grecord],
  AS_HELP_STRING([--enable-grecord], [enable gnome sound recorder]),
  ,enable_grecord=yes)

if test "x$have_profiles" = "xno" ; then
  AC_MSG_WARN([Grecord will be disabled because of disabled profiles])
  enable_grecord=no
  have_grecord=no
  grecord_reason=", gnome media profiles is disabled"
fi

if test "x$have_gst" = "xyes" && test "x$enable_grecord" = "xyes"; then
  have_grecord=yes
else
  if test "x$have_gst" = "xno"; then
    AC_MSG_WARN([You need GStreamer installed for the mixer and recorder to work])
    sleep 5 2>/dev/null
  fi
fi

AC_SUBST(GSR_CFLAGS)
AC_SUBST(GSR_LIBS)
AM_CONDITIONAL(HAVE_GSTREAMER, [test x$have_gst = xyes])
AM_CONDITIONAL(HAVE_GRECORD, [test x$have_grecord = xyes])

dnl uninstalled gsr ui dir
AC_DEFINE_UNQUOTED(GSR_UIDIR_UNINSTALLED, "`pwd`/grecord/src",
  [path to uninstalled gnome-sound-recorder UI dir])

AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

BONOBO_IDLDIR="`$PKG_CONFIG --variable=idldir libbonobo-2.0`"
AC_SUBST(BONOBO_IDLDIR)
BONOBO_ACT_IDLDIR="`$PKG_CONFIG --variable=idldir bonobo-activation-2.0`"
AC_SUBST(BONOBO_ACT_IDLDIR)

ORBIT_IDL="`$PKG_CONFIG --variable=orbit_idl ORBit-2.0`"
AC_SUBST(ORBIT_IDL)

dnl=======================================================================
dnl Check for esound library
dnl=======================================================================

have_esd=no

AC_ARG_ENABLE([esound],
  AS_HELP_STRING([--enable-esound], [enable support for esound daemon]),
  ,enable_esound=yes)

if test "x$enable_esound" = "xyes"; then
  AM_PATH_ESD(0.2.7, have_esd=yes)

  if test "x$have_esd" = "xyes"; then
    AC_DEFINE(HAVE_ESD, 1, [Defined if we have Esound])
  fi
fi

dnl=======================================================================
dnl Test whether jw is installed
dnl=======================================================================

AC_PATH_PROG(JW,jw,no)
if test x$JW = xno; then
  HAVE_JW="no"
else
  HAVE_JW="yes"
fi

AM_CONDITIONAL(HAVE_JW, [test x$HAVE_JW = xyes])
AC_SUBST(HAVE_JW)

dnl=======================================================================
dnl i18n
dnl=======================================================================

GETTEXT_PACKAGE=gnome-media-2.0
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Define to the Gettext package name])
AM_GLIB_GNU_GETTEXT

GNOME_DOC_INIT

AM_PROG_LIBTOOL
AM_PROG_CC_C_O

dnl=======================================================================
dnl vu-meter
dnl=======================================================================

have_vumeter=no
vumeter_reason=""

AC_ARG_ENABLE([vumeter],
  AS_HELP_STRING([--enable-vumeter], [enable vumeter]),
  ,enable_vumeter=no)

if test "x$enable_vumeter" = "xyes" ; then
  if test "x$have_esd" = "xyes"; then
    PKG_CHECK_MODULES(VUMETER, [
      glib-2.0 >= 1.3.7
      gtk+-2.0 >= 2.3.1
      esound >= 0.2.23
      libgnomeui-2.0 > 1.102.0],
      have_vumeter=yes)
  else
    AC_MSG_WARN([vu-meter disabled because esound couldn't be found])
    vumeter_reason=", esound not found"
  fi
fi

AC_SUBST(VUMETER_CFLAGS)
AC_SUBST(VUMETER_LIBS)
AM_CONDITIONAL(HAVE_VUMETER, [test x$have_vumeter = xyes])

dnl=======================================================================
dnl check for gnome-cd requirements
dnl=======================================================================

# GStreamer is optional
# libnautilus-burn is required

have_gnomecd=no
gnomecd_reason=""

AC_ARG_ENABLE([gnomecd],
  AS_HELP_STRING([--enable-gnomecd], [enable gnome cd player]),
  ,enable_gnomecd=no)

if test "x$have_cddbslave" = "xno" ; then
  AC_MSG_WARN([gnome-cd will be disabled because of disabled cddb-slave2])
  enable_gnomecd=no
  have_gnomecd=no
  gnomecd_reason=", missing cddb-slave support"
fi

if test "x$enable_gnomecd" = "xyes"; then
  PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= 2.5.0])

  PKG_CHECK_MODULES(GNOMECD, [
    gstreamer-$GST_MAJORMINOR >= $GST_REQ
    gstreamer-plugins-base-$GST_MAJORMINOR >= $GST_PLUGINS_BASE_REQ
    libnautilus-burn >= 2.9.0
    gtk+-2.0 >= 2.10.0],
    have_gnomecd=yes)

  if test "x$have_gnomecd" = "xno"; then
    PKG_CHECK_MODULES(GNOMECD, [
      libnautilus-burn >= 2.9.0
      gtk+-2.0 >= 2.10.0],
      have_gnomecd=yes)
  fi
fi

AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)

AC_SUBST(GNOMECD_CFLAGS)
AC_SUBST(GNOMECD_LIBS)
AM_CONDITIONAL(HAVE_GNOMECD, [test x$have_gnomecd = xyes])

if test "x$have_gnomecd" = "xyes" && test "x$enable_gnomecd" = "xyes" ; then
  cdrom_header=""
  AC_CHECK_HEADERS(linux/cdrom.h,[
    PROGRAMS_GNOME_CD=gnome-cd
    cdrom_header=linux/cdrom.h])
  AC_CHECK_HEADERS(linux/ucdrom.h,[
    cdrom_header=linux/ucdrom.h])
  AC_CHECK_HEADERS(sys/cdio.h,[
    PROGRAMS_GNOME_CD=gnome-cd
    cdrom_header=sys/cdio.h])
  AC_CHECK_HEADERS(sun/dkio.h,[
    PROGRAMS_GNOME_CD=gnome-cd
    cdrom_header=sun/dkio.h])

  AC_MSG_CHECKING([whether to use cd changer support])
  AC_EGREP_CPP(answer_affirmatively,
  [#include <linux/cdrom.h>
   #ifdef CDROM_CHANGER_NSLOTS
        answer_affirmatively
   #endif
  ], cd_changer=yes, cd_changer=no)
  if test "$cd_changer" = "yes"; then
        USE_CD_CHANGER="-DTCD_CHANGER_ENABLED"
  fi
  AC_MSG_RESULT($cd_changer)

  AC_SUBST(USE_CD_CHANGER)
  AC_SUBST(PROGRAMS_GNOME_CD)

  close_tray=no
  AC_MSG_CHECKING([whether OS supports the CDROMCLOSETRAY ioctl])
  AC_EGREP_CPP(answer_affirmatively,
  [#include <$cdrom_header>
   #ifdef CDROMCLOSETRAY
        answer_affirmatively
   #endif
  ], close_tray=yes)
  if test "$close_tray" = "yes"; then
        AC_DEFINE(HAVE_CDROMCLOSETRAY_IOCTL, 1,
            [Defined if have an ioctl to close the CD-rom tray])
  fi
  AC_MSG_RESULT($close_tray)

  host=`uname -s`

  CDROM_HOST_BSD=false
  CDROM_HOST_LINUX=false
  CDROM_HOST_SOLARIS=false
  case "$host" in
  *FreeBSD*)
        CDROM_HOST=bsd
        CDROM_HOST_BSD=true
        default_cd_device="/dev/acd0c"
        ;;
  *NetBSD*)
        CDROM_HOST=bsd
        CDROM_HOST_BSD=true
        default_cd_device="/dev/rcd0d"
        ;;
  *Linux*)
        CDROM_HOST=linux
        CDROM_HOST_LINUX=true
        default_cd_device="/dev/cdrom"
        ;;
  *SunOS*)
        CDROM_HOST=solaris
        CDROM_HOST_SOLARIS=true
        default_cd_device="/vol/dev/aliases/cdrom0"
        ;;
  esac

  AC_SUBST(CDROM_HOST)
  AC_SUBST(CDROM_HOST_BSD)
  AC_SUBST(CDROM_HOST_LINUX)
  AC_SUBST(CDROM_HOST_SOLARIS)

  AC_SUBST(default_cd_device)
  AC_DEFINE_UNQUOTED(default_cd_device, "$default_cd_device",
    [Default CD device to use])

  dnl uninstalled theme dir
  AC_DEFINE_UNQUOTED(THEME_DIR_UNINSTALLED, "`pwd`/gnome-cd/images",
      [path to uninstalled gnome-cd themes dir])
  AC_SUBST(THEME_DIR_UNINSTALLED)

  dnl used for accessibility of gnome-cd
  PKG_CHECK_MODULES(GAILUTIL,
	gail >= 0.0.3
	libbonobo-2.0 >= 2.0.0
	gconf-2.0)
  AC_SUBST(GAILUTIL_CFLAGS)
  AC_SUBST(GAILUTIL_LIBS)
fi

AM_CONDITIONAL(CDROM_HOST_BSD, test "x$CDROM_HOST_BSD" = "xtrue")
AM_CONDITIONAL(CDROM_HOST_LINUX,   test "x$CDROM_HOST_LINUX"   = "xtrue")
AM_CONDITIONAL(CDROM_HOST_SOLARIS, test "x$CDROM_HOST_SOLARIS" = "xtrue")

dnl=======================================================================
dnl gstreamer-properties
dnl=======================================================================

have_gstprops=no

AC_ARG_ENABLE([gstprops],
  AS_HELP_STRING([--enable-gstprops], [enable gstreamer properties]),
  ,enable_gstprops=yes)

if test "x$have_gst" = "xyes" && test "x$enable_gstprops" = "xyes";
then
  PKG_CHECK_MODULES(GSTPROPS, [
    gconf-2.0
    libgnomeui-2.0
    libglade-2.0 >= 1.99.12
    gstreamer-$GST_MAJORMINOR
    gstreamer-plugins-base-$GST_MAJORMINOR >= $GST_PLUGINS_BASE_REQ
    libxml-2.0],
    have_gstprops=yes)

  GSTPROPS_LIBS="$GSTPROPS_LIBS -lgstinterfaces-$GST_MAJORMINOR"

  GNOMECC_DESKTOP_DIR=${datadir}/control-center-2.0/capplets
  AS_AC_EXPAND(GNOMECC_DESKTOP_DIR, $GNOMECC_DESKTOP_DIR)
  AC_DEFINE_UNQUOTED(GNOMECC_DESKTOP_DIR, "$GNOMECC_DESKTOP_DIR",
                     [Install path for Gnome control-center integration])
  AC_SUBST(GNOMECC_DESKTOP_DIR)
  AC_SUBST(PROGRAMS_GSTPROPS)
fi

AC_SUBST(GSTPROPS_CFLAGS)
AC_SUBST(GSTPROPS_LIBS)
AM_CONDITIONAL(HAVE_GSTPROPS, [test x$have_gstprops = xyes])

dnl=======================================================================
dnl gst-mixer
dnl=======================================================================

have_gstmixer=no

AC_ARG_ENABLE([gstmix],
  AS_HELP_STRING([--enable-gstmix], [enable gstreamer mixer]),
  ,enable_gstmix=yes)

if test "x$have_pulseaudio" = "xno" && test "x$have_gst" = "xyes" && test "x$enable_gstmix" = "xyes";
then
  PKG_CHECK_MODULES(GSTMIXER, [
    libgnomeui-2.0
    gtk+-2.0
    gconf-2.0
    gstreamer-$GST_MAJORMINOR
    gstreamer-plugins-base-$GST_MAJORMINOR
  ],
  [
    OLD_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS $GSTMIXER_CFLAGS"

    AC_CHECK_HEADER(gst/audio/mixerutils.h,
      [AC_CHECK_HEADER(gst/interfaces/mixer.h,
        [AC_CHECK_HEADER(gst/interfaces/propertyprobe.h, have_gstmixer=yes)])])

    CPPFLAGS=$OLD_CPPFLAGS
    GSTMIXER_LIBS="$GSTMIXER_LIBS -lgstinterfaces-$GST_MAJORMINOR -lgstaudio-$GST_MAJORMINOR"
  ])
fi

AC_SUBST(GSTMIXER_CFLAGS)
AC_SUBST(GSTMIXER_LIBS)
AM_CONDITIONAL(HAVE_GSTMIXER, [test x$have_gstmixer = xyes])


dnl ---------------------------------------------------------------------------
dnl - Finish
dnl ---------------------------------------------------------------------------

# Turn on the additional warnings last, so warnings don't affect other tests.
AC_ARG_ENABLE(more-warnings,
	[AC_HELP_STRING([--enable-more-warnings],
	[Maximum compiler warnings])],
	set_more_warnings="$enableval",[
        	if test -d $srcdir/.svn; then
        		set_more_warnings=yes
              	else
                  	set_more_warnings=no
              	fi
        ])
AC_MSG_CHECKING(for more warnings)
if test "$GCC" = "yes" -a "$set_more_warnings" != "no"; then
        AC_MSG_RESULT(yes)
        CFLAGS="\
        -Wall \
        -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
        -Wnested-externs -Wpointer-arith \
        -Wcast-align -Wsign-compare \
        $CFLAGS"

        for option in -Wno-unused-parameter -Wno-strict-aliasing -Wno-sign-compare; do
                SAVE_CFLAGS="$CFLAGS"
                CFLAGS="$CFLAGS $option"
                AC_MSG_CHECKING([whether gcc understands $option])
                AC_TRY_COMPILE([], [],
                        has_option=yes,
                        has_option=no,)
                if test $has_option = no; then
                        CFLAGS="$SAVE_CFLAGS"
                fi
                AC_MSG_RESULT($has_option)
                unset has_option
                unset SAVE_CFLAGS
        done
        unset option
else
        AC_MSG_RESULT(no)
fi


AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LIBS)
AC_SUBST(LDFLAGS)

AC_CONFIG_FILES([
Makefile
po/Makefile.in
gnome-volume-control/Makefile
gnome-volume-control/data/Makefile
gnome-volume-control/data/gnome-volume-control.desktop.in
gnome-volume-control/data/icons/Makefile
gnome-volume-control/data/icons/16x16/Makefile
gnome-volume-control/data/icons/16x16/apps/Makefile
gnome-volume-control/data/icons/16x16/status/Makefile
gnome-volume-control/data/icons/22x22/Makefile
gnome-volume-control/data/icons/22x22/apps/Makefile
gnome-volume-control/data/icons/22x22/status/Makefile
gnome-volume-control/data/icons/24x24/Makefile
gnome-volume-control/data/icons/24x24/apps/Makefile
gnome-volume-control/data/icons/24x24/status/Makefile
gnome-volume-control/data/icons/32x32/Makefile
gnome-volume-control/data/icons/32x32/apps/Makefile
gnome-volume-control/data/icons/32x32/status/Makefile
gnome-volume-control/data/icons/48x48/Makefile
gnome-volume-control/data/icons/48x48/apps/Makefile
gnome-volume-control/data/icons/scalable/Makefile
gnome-volume-control/data/icons/scalable/apps/Makefile
gnome-volume-control/data/icons/scalable/status/Makefile
gnome-volume-control/data/sounds/Makefile
gnome-volume-control/src/Makefile
vu-meter/Makefile
cddb-slave2/Makefile
gnome-cd/Makefile
gnome-cd/gnome-cd.schemas.in
gnome-cd/images/Makefile
gnome-cd/images/None/Makefile
gnome-cd/images/lcd/Makefile
gnome-cd/images/red-lcd/Makefile
gnome-cd/images/media/Makefile
gnome-cd/access/Makefile
gnome-cd/doc/Makefile
grecord/Makefile
grecord/gnome-sound-recorder.schemas.in
grecord/src/Makefile
grecord/doc/Makefile
grecord/icons/Makefile
grecord/icons/16x16/Makefile
grecord/icons/22x22/Makefile
grecord/icons/24x24/Makefile
grecord/icons/32x32/Makefile
grecord/icons/48x48/Makefile
grecord/icons/scalable/Makefile
gstreamer-properties/Makefile
gstreamer-properties/help/Makefile
gst-mixer/Makefile
gst-mixer/icons/Makefile
gst-mixer/icons/16x16/Makefile
gst-mixer/icons/16x16/devices/Makefile
gst-mixer/icons/16x16/status/Makefile
gst-mixer/src/Makefile
gst-mixer/pixmaps/Makefile
gst-mixer/doc/Makefile
profiles/Makefile
profiles/gnome-audio-profiles.schemas.in
profiles/gnome-media-profiles.pc
profiles/glade/Makefile
cddb-slave2/cddb-slave.desktop.in
gnome-cd/gnome-cd.desktop.in
grecord/gnome-sound-recorder.desktop.in
gst-mixer/gnome-volume-control.desktop.in
gstreamer-properties/gstreamer-properties.desktop.in
vu-meter/reclevel.desktop.in
vu-meter/vumeter.desktop.in
])

AC_OUTPUT

echo "

                    ${PACKAGE} ${VERSION}
                    ============


	Prefix:                 ${prefix}
        Source code location:   ${srcdir}
        Compiler:               ${CC}
        CFLAGS:                 ${CFLAGS}

        Volume Control          $have_pulseaudio
        GStreamer properties:   $have_gstprops
        GStreamer profiles:     $have_profiles
        Sound recorder:         $have_grecord$grecord_reason

  *Deprecated*:

        GStreamer mixer:        $have_gstmixer
        vu-meter:               $have_vumeter$vumeter_reason
        cddb-slave:             $have_cddbslave
        gnome-cd:               $have_gnomecd$gnomecd_reason"

if test x$enable_gnomecd = xyes; then
echo "  System CDRom header:    ${cdrom_header}
        Default CD device:      ${default_cd_device}
        OS supports close tray: ${close_tray}
        Architecture:           ${CDROM_HOST}"
fi

echo ""

