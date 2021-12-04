#SUBDIRS = utils
noinst_LTLIBRARIES = libupdater-module.la

BUILT_SOURCES = \
	updater-control-resources.c \
	updater-control-resources.h

libupdater_module_la_SOURCES = \
	$(BUILT_SOURCES) \
	updater-module.c \
	updater-module.h

libupdater_module_la_CPPFLAGS = \
	-I$(top_srcdir)/common	\
	$(AM_CPPFLAGS)

libupdater_module_la_CFLAGS =	\
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)	\
	$(LIBNOTIFY_CFLAGS)

libupdater_module_la_LDFLAGS = \
	$(AM_LDFLAGS)

libupdater_module_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS)	\
	$(LIBNOTIFY_LIBS)

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gresource.xml)
updater-control-resources.c: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name updater_control $<
updater-control-resources.h: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name updater_control $<

CLEANFILES = $(BUILT_SOURCES)
