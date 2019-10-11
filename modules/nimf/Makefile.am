noinst_LTLIBRARIES = libnimf-module.la

BUILT_SOURCES = \
	nimf-control-resources.c \
	nimf-control-resources.h

libnimf_module_la_SOURCES = \
	$(BUILT_SOURCES) \
	nimf-module.c	\
	nimf-module.h

libnimf_module_la_CPPFLAGS = \
	-I$(top_srcdir)/common	\
	$(AM_CPPFLAGS)

libnimf_module_la_CFLAGS =	\
	$(GLIB_CFLAGS)	\
	$(GTK_CFLAGS)	\
	$(GIO_CFLAGS)

libnimf_module_la_LDFLAGS = \
	$(AM_LDFLAGS)

libnimf_module_la_LIBADD = \
	$(GLIB_LIBS)	\
	$(GTK_LIBS)		\
	$(GIO_LIBS)


resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gresource.xml)
nimf-control-resources.c: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name nimf_control $<
nimf-control-resources.h: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name nimf_control $<

CLEANFILES = $(BUILT_SOURCES)

moduledir = $(libdir)/nimf/modules/services
module_LTLIBRARIES = libnimf-gooroom.la

libnimf_gooroom_la_SOURCES = \
	nimf-gooroom.c \
	$(NULL)

libnimf_gooroom_la_CFLAGS = \
	$(GLIB_CFLAGS)	\
	$(GTK_CFLAGS)	\
	$(NIMF_CFLAGS)

libnimf_gooroom_la_LDFLAGS = -module -avoid-version
libnimf_gooroom_la_LIBADD  = \
	$(GLIB_LIBS) \
	$(GTK_LIBS)	\
	$(NIMF_LIBS)


install-data-hook:
	chmod -x $(DESTDIR)$(moduledir)/libnimf-gooroom.so
	rm    -f $(DESTDIR)$(moduledir)/libnimf-gooroom.la

uninstall-hook:
	rm    -f $(DESTDIR)$(moduledir)/libnimf-gooroom.so
	-rmdir -p $(DESTDIR)$(moduledir)

DISTCLEANFILES = Makefile.in
