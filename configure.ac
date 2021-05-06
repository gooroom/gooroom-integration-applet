dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_INIT([gooroom-integration-applet], [0.1.0])
AC_PREREQ(2.63)

dnl ***********************
dnl Initialize automake ***
dnl ***********************
AM_INIT_AUTOMAKE([1.8 dist-xz no-dist-gzip foreign])
AC_CONFIG_HEADERS(config.h)
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl *******************************
dnl *** Check for UNIX variants ***
dnl *******************************
AC_AIX()
AC_ISC_POSIX()
AC_MINIX()

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CC()
AC_PATH_PROG([GDBUS_CODEGEN], [gdbus-codegen])
AM_PROG_CC_C_O()
AC_PROG_LD()
AC_PROG_INSTALL()
IT_PROG_INTLTOOL([0.35.2])

dnl **********************
dnl Initialize libtool ***
dnl **********************
LT_PREREQ([2.2.6])
LT_INIT([dlopen disable-static])
LT_LIB_M

dnl **********************************
dnl *** Check for standard headers ***
dnl **********************************
AC_HEADER_STDC()
AC_CHECK_HEADERS([stdlib.h string.h errno.h unistd.h])

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
GETTEXT_PACKAGE=gooroom-integration-applet
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])
AM_GLIB_GNU_GETTEXT

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
GLIB_GSETTINGS

GTK_REQUIRED=3.20.0
GLIB_REQUIRED=2.44.0
LIBPANEL_REQUIRED=3.24.1
GIO_REQUIRED=2.54.1
CANBERRA_REQUIRED=0.30
PULSE_REQUIRED=0.9.19
UPOWER_GLIB_REQUIRED=0.99.7
ACCOUNTSSERVICE_REQUIRED=0.6.37

PKG_CHECK_MODULES(X11, x11 >= 1.6.7)
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= $GTK_REQUIRED)
PKG_CHECK_MODULES(GIO, gio-2.0 >= $GIO_REQUIRED)
PKG_CHECK_MODULES(JSON_C, json-c >= 0.11.99)
PKG_CHECK_MODULES(GNOME_APPLETS, libpanel-applet >= $LIBPANEL_REQUIRED)
PKG_CHECK_MODULES(CANBERRA_GTK3, libcanberra-gtk3 >= $CANBERRA_REQUIRED)
PKG_CHECK_MODULES(POLKIT, polkit-gobject-1 >= 0.103)
PKG_CHECK_MODULES(PULSE, libpulse-mainloop-glib >= $PULSE_REQUIRED)
PKG_CHECK_MODULES(UPOWER_GLIB, upower-glib >= $UPOWER_GLIB_REQUIRED)
PKG_CHECK_MODULES(UDEV, libudev >= 237)
PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= 0.7.0)
PKG_CHECK_MODULES(ACCOUNTSSERVICE, accountsservice >= $ACCOUNTSSERVICE_REQUIRED)
PKG_CHECK_MODULES(STARTUP_NOTIFICATION, libstartup-notification-1.0 >= 0.12)
PKG_CHECK_MODULES(NIMF, nimf)
PKG_CHECK_MODULES(GNOME_DESKTOP, gnome-desktop-3.0)

AC_ARG_WITH([libpanel-applet-dir], [], [LIBPANEL_APPLET_DIR=$withval], [LIBPANEL_APPLET_DIR=""])
if test "$LIBPANEL_APPLET_DIR" == ""; then
    LIBPANEL_APPLET_DIR=`$PKG_CONFIG --variable=libpanel_applet_dir libpanel-applet`
fi

AC_MSG_NOTICE([installing applets in $LIBPANEL_APPLET_DIR])
AC_SUBST(LIBPANEL_APPLET_DIR)

AC_OUTPUT([
  Makefile
  data/Makefile
  icons/Makefile
  common/Makefile
  modules/Makefile
  modules/sound/Makefile
  modules/power/Makefile
  modules/user/Makefile
  modules/datetime/Makefile
  modules/security/Makefile
  modules/security/utils/Makefile
  modules/endsession/Makefile
  modules/nimf/Makefile
  src/Makefile
  po/Makefile.in
])
