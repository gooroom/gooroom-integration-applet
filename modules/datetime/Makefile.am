noinst_LTLIBRARIES = libdatetime-module.la

BUILT_SOURCES = \
	datetime-control-resources.c \
	datetime-control-resources.h

libdatetime_module_la_SOURCES = \
	$(BUILT_SOURCES) \
	gooroom-calendar.h \
	gooroom-calendar.c \
	datetime-module.c \
	datetime-module.h

libdatetime_module_la_CPPFLAGS = \
	-I$(top_srcdir)/common \
	$(AM_CPPCFLAGS)

libdatetime_module_la_CFLAGS =  \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(AM_CFLAGS)

libdatetime_module_la_LDFLAGS = \
	$(AM_LDFLAGS)

libdatetime_module_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS)

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gresource.xml)
datetime-control-resources.c: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name datetime_control $<
datetime-control-resources.h: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name datetime_control $<

CLEANFILES = $(BUILT_SOURCES)
