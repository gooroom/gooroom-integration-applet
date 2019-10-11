noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES =	\
	panel-glib.h	\
	panel-glib.c	\
	common.h \
	common.c

libcommon_la_CPPFLAGS = \
	-DSN_API_NOT_YET_FROZEN \
	$(AM_CPPFLAGS)

libcommon_la_CFLAGS =	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(GIO_CFLAGS)		\
	$(POLKIT_CFLAGS)	\
	$(STARTUP_NOTIFICATION_CFLAGS)

libcommon_la_LDFLAGS = \
	$(AM_LDFLAGS)

libcommon_la_LIBADD =	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS)			\
	$(GIO_LIBS)			\
	$(POLKIT_LIBS)		\
	$(STARTUP_NOTIFICATION_LIBS)

