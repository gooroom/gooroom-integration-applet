noinst_LTLIBRARIES = libendsession-module.la

libendsession_module_la_CPPFLAGS = \
	-I$(top_srcdir)/common	\
	$(AM_CPPCFLAGS)

BUILT_SOURCES = \
	endsession-control-resources.c \
	endsession-control-resources.h

libendsession_module_la_SOURCES = \
	$(BUILT_SOURCES)	\
	endsession-module.c \
	endsession-module.h

libendsession_module_la_CFLAGS =	\
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(AM_CFLAGS)

libendsession_module_la_LDFLAGS = \
	$(AM_LDFLAGS)

libendsession_module_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS)

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gresource.xml)
endsession-control-resources.c: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name endsession_control $<
endsession-control-resources.h: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name endsession_control $<


CLEANFILES = $(BUILT_SOURCES)
