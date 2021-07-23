libutils_la_CPPFLAGS = \
	-DGOOROOM_LOGPARSER_SEEKTIME_HELPER=\"/usr/lib/gooroom-security-utils/gooroom-logparser-seektime-helper\" \
	-DGOOROOM_SECURITY_LOGPARSER_WRAPPER=\"/usr/lib/gooroom-security-utils/gooroom-security-logparser-wrapper\" \
	$(AM_CPPFLAGS)

noinst_LTLIBRARIES = libutils.la

libutils_la_SOURCES =	\
	utils.h \
	utils.c

libutils_la_CFLAGS = \
	$(GLIB_CFLAGS)	\
	$(JSON_C_CFLAGS)

libutils_la_LIBADD = \
	$(GLIB_LIBS)	\
	$(JSON_C_LIBS)
