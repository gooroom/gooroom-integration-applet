noinst_LTLIBRARIES = libpower-module.la

libpower_module_la_SOURCES = \
	power-module.c \
	power-module.h

libpower_module_la_CPPFLAGS = \
	-I$(top_srcdir)/common \
	$(AM_CPPCFLAGS)

libpower_module_la_CFLAGS =  \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(UPOWER_GLIB_CFLAGS) \
	$(AM_CFLAGS)

libpower_module_la_LDFLAGS = \
	$(AM_LDFLAGS)

libpower_module_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS)	\
	$(UPOWER_GLIB_LIBS)
