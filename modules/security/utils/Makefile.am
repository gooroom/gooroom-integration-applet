libutils_la_CPPFLAGS = \
	-DGOOROOM_LOGPARSER_SEEKTIME_HELPER=\"$(libexecdir)/gooroom-security-status-tools/gooroom-logparser-seektime-helper\" \
	-DGOOROOM_SECURITY_LOGPARSER_WRAPPER=\"$(libexecdir)/gooroom-security-status-tools/gooroom-security-logparser-wrapper\" \
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
