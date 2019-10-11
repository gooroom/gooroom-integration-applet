noinst_LTLIBRARIES = libsound-module.la

libsound_module_la_CPPFLAGS = \
	-I$(top_srcdir)/common	\
	$(AM_CPPCFLAGS)

libsound_module_la_SOURCES = \
	pulseaudio-volume.c \
	pulseaudio-volume.h \
	pulseaudio-debug.c \
	pulseaudio-debug.h \
	sound-module.c \
	sound-module.h

libsound_module_la_CFLAGS = \
	-DDEFAULT_MIXER_COMMAND=\"$(DEFAULT_MIXER_COMMAND)\" \
	$(CANBERRA_GTK3_CFLAGS) \
	$(PULSE_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

#	-I$(srcdir)	\
#	$(KEYBINDER_CFLAGS) \
#	$(LIBNOTIFY_CFLAGS) \
#	$(PLATFORM_CFLAGS)

libsound_module_la_LDFLAGS = \
	$(AM_LDFLAGS)

libsound_module_la_LIBADD = \
	$(CANBERRA_GTK3_LIBS) \
	$(PULSE_LIBS) \
	$(GLIB_LIBS) \
	$(GTK_LIBS)

#	$(KEYBINDER_LIBS) \
#	$(LIBNOTIFY_LIBS) \
#	$(LIBM)
