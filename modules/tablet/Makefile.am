noinst_LTLIBRARIES = libtablet-module.la

BUILT_SOURCES = \
	tablet-control-resources.c \
	tablet-control-resources.h

libtablet_module_la_SOURCES = \
	$(BUILT_SOURCES) \
	tablet-module.c \
	tablet-module.h

libtablet_module_la_CPPFLAGS = \
	-I$(top_srcdir)/common	\
	-DTABLET_MODE_CHANGE_HELPER=\"$(libexecdir)/tablet-mode-change-helper\" \
	$(AM_CPPFLAGS)

libtablet_module_la_CFLAGS =	\
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

libtablet_module_la_LDFLAGS = \
	$(AM_LDFLAGS)

libtablet_module_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS)

libexec_PROGRAMS = tablet-mode-change-helper

tablet_mode_change_helper_SOURCES = tablet-mode-change-helper.c
tablet_mode_change_helper_CFLAGS = \
	-DLOCALEDIR=\"$(localedir)\" \
	$(GIO_CFLAGS) \
	$(GLIB_CFLAGS)
tablet_mode_change_helper_LDADD = \
	$(GIO_LIBS) \
	$(GLIB_LIBS)

kr.gooroom.integration.applet.policy.in: kr.gooroom.integration.applet.policy.in.in Makefile
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|" $< >$@

@INTLTOOL_POLICY_RULE@
polkitdir       = $(datadir)/polkit-1/actions
polkit_in_files = kr.gooroom.integration.applet.policy.in
polkit_DATA     = $(polkit_in_files:.policy.in=.policy)

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gresource.xml)
tablet-control-resources.c: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name tablet_control $<
tablet-control-resources.h: gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name tablet_control $<

CLEANFILES = $(BUILT_SOURCES)
