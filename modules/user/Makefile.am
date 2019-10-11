noinst_LTLIBRARIES = libuser-module.la

libuser_module_la_CPPFLAGS = \
	-I$(top_srcdir)/common	\
	$(AM_CPPCFLAGS)

BUILT_SOURCES = \
	user-control-resources.c \
	user-control-resources.h

libuser_module_la_SOURCES = \
	$(BUILT_SOURCES)	\
	user-module.c \
	user-module.h

libuser_module_la_CFLAGS =	\
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(ACCOUNTSSERVICE_CFLAGS)	\
	$(AM_CFLAGS)

libuser_module_la_LDFLAGS = \
	$(AM_LDFLAGS)

libuser_module_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS)	\
	$(ACCOUNTSSERVICE_LIBS)

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gresource.xml)
user-control-resources.c: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name user_control $<
user-control-resources.h: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name user_control $<


CLEANFILES = $(BUILT_SOURCES)
