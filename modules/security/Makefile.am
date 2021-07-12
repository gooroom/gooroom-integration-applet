SUBDIRS = utils

gsettings_SCHEMAS = apps.gooroom-security-status.gschema.xml
@GSETTINGS_RULES@

noinst_LTLIBRARIES = libsecurity-module.la

BUILT_SOURCES = \
	security-control-resources.c \
	security-control-resources.h

libsecurity_module_la_SOURCES = \
	$(BUILT_SOURCES) \
	security-module.c \
	security-module.h

libsecurity_module_la_CPPFLAGS = \
	-DGOOROOM_SECURITY_STATUS_VULNERABLE_HELPER=\"/usr/lib/gooroom-security-utils/gooroom-security-status-vulnerable-helper\" \
	-I$(srcdir)/utils \
	-I$(top_srcdir)/common	\
	$(AM_CPPFLAGS)

libsecurity_module_la_CFLAGS =	\
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)	\
	$(JSON_C_CFLAGS) \
	$(LIBNOTIFY_CFLAGS)

libsecurity_module_la_LDFLAGS = \
	$(AM_LDFLAGS)

libsecurity_module_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS)	\
	$(LIBNOTIFY_LIBS) \
	$(JSON_C_LIBS) \
	./utils/libutils.la


resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gresource.xml)
security-control-resources.c: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name security_control $<
security-control-resources.h: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name security_control $<

CLEANFILES = $(BUILT_SOURCES)
