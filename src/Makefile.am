gooroomappletdir = $(GNOME_PANEL_MODULES_DIR)
gooroomapplet_LTLIBRARIES = libgooroom-integration-applet.la

libgooroom_integration_applet_la_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir)/common \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DGNOMELOCALEDIR=\""$(localedir)"\"

BUILT_SOURCES = \
	applet-resources.c \
	applet-resources.h

libgooroom_integration_applet_la_SOURCES = \
	$(BUILT_SOURCES)	\
	popup-window.h	\
	popup-window.c	\
	gooroom-integration-applet.c \
	gooroom-integration-module.c \
	gooroom-integration-applet.h

libgooroom_integration_applet_la_CFLAGS = \
	$(LIBGNOMEPANEL_CFLAGS) \
	$(GNOME_DESKTOP_CFLAGS) \
	$(GLIB_CFLAGS)	\
	$(GTK_CFLAGS)	\
	$(AM_CFLAGS)	\
	$(LIBNOTIFY_CFLAGS)

libgooroom_integration_applet_la_LDFLAGS = \
	-avoid-version	\
	$(AM_LDFLAGS)

libgooroom_integration_applet_la_LIBADD = \
	$(LIBGNOMEPANEL_LIBS) \
	$(GNOME_DESKTOP_LIBS) \
	$(GLIB_LIBS)	\
	$(GTK_LIBS)	\
	$(LIBNOTIFY_LIBS) \
	$(top_builddir)/common/libcommon.la	\
	$(top_builddir)/modules/sound/libsound-module.la	\
	$(top_builddir)/modules/power/libpower-module.la	\
	$(top_builddir)/modules/user/libuser-module.la	\
	$(top_builddir)/modules/datetime/libdatetime-module.la	\
	$(top_builddir)/modules/security/libsecurity-module.la	\
	$(top_builddir)/modules/endsession/libendsession-module.la \
	$(top_builddir)/modules/nimf/libnimf-module.la

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gresource.xml)
applet-resources.c: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name applet $<
applet-resources.h: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name applet $<

CLEANFILES = $(BUILT_SOURCES)
