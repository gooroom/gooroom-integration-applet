iconsymbolicstatusdir = $(datadir)/icons/hicolor/scalable/status
iconsymbolicstatus_DATA =  				\
	audio-volume-error-symbolic.svg		\
	audio-volume-high-symbolic.svg		\
	audio-volume-low-symbolic.svg		\
	audio-volume-medium-symbolic.svg	\
	audio-volume-muted-symbolic.svg		\
	audio-volume-zero-symbolic.svg		\
	battery-error.svg				\
	battery-empty.svg				\
	battery-empty-charging.svg		\
	battery-10.svg					\
	battery-10-charging.svg			\
	battery-25.svg					\
	battery-25-charging.svg			\
	battery-40.svg					\
	battery-40-charging.svg			\
	battery-50.svg					\
	battery-50-charging.svg			\
	battery-60.svg					\
	battery-60-charging.svg			\
	battery-75.svg					\
	battery-75-charging.svg			\
	battery-90.svg					\
	battery-90-charging.svg			\
	battery-full.svg				\
	battery-full-charging.svg		\
	battery-full-charged.svg		\
	security-status-safety.svg		\
	security-status-unknown.svg		\
	security-status-vulnerable.svg

iconsymbolicactionsdir = $(datadir)/icons/hicolor/scalable/actions
iconsymbolicactions_DATA =  go-next-page-symbolic.svg


gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then					\
		echo "Updating Gtk icon cache.";			\
		$(gtk_update_icon_cache);				\
	else								\
		echo "*** Icon cache not updated. Remember to run:";	\
		echo "***";						\
		echo "***   $(gtk_update_icon_cache)";			\
		echo "***";						\
	fi

EXTRA_DIST =            \
	$(icon_DATA)
