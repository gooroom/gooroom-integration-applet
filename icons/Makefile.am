iconsymbolicstatusdir = $(datadir)/icons/hicolor/scalable/status
iconsymbolicstatus_DATA = \
	integrationapplet-audio-volume-error.svg \
	integrationapplet-audio-volume-high-symbolic.svg \
	integrationapplet-audio-volume-low-symbolic.svg	\
	integrationapplet-audio-volume-medium-symbolic.svg \
	integrationapplet-audio-volume-muted-symbolic.svg \
	integrationapplet-audio-volume-zero-symbolic.svg \
	integrationapplet-display-brightness.svg \
	integrationapplet-date-time.svg \
	integrationapplet-users.svg \
	integrationapplet-users-cleanmode.svg \
	integrationapplet-battery-error.svg \
	integrationapplet-battery-empty.svg \
	integrationapplet-battery-empty-charging.svg \
	integrationapplet-battery-10.svg \
	integrationapplet-battery-10-charging.svg \
	integrationapplet-battery-25.svg \
	integrationapplet-battery-25-charging.svg \
	integrationapplet-battery-40.svg \
	integrationapplet-battery-40-charging.svg \
	integrationapplet-battery-50.svg \
	integrationapplet-battery-50-charging.svg \
	integrationapplet-battery-60.svg \
	integrationapplet-battery-60-charging.svg \
	integrationapplet-battery-75.svg \
	integrationapplet-battery-75-charging.svg \
	integrationapplet-battery-90.svg \
	integrationapplet-battery-90-charging.svg \
	integrationapplet-battery-full.svg \
	integrationapplet-battery-full-charging.svg \
	integrationapplet-battery-full-charged.svg \
	integrationapplet-security-status-safety.svg \
	integrationapplet-security-status-unknown.svg \
	integrationapplet-security-status-vulnerable.svg \
	integrationapplet-tablet.svg \
	integrationapplet-nimf-anthy.svg \
	integrationapplet-nimf-invalid.svg \
	integrationapplet-nimf-m17n-vi.svg \
	integrationapplet-nimf-focus-out.svg \
	integrationapplet-nimf-libhangul.svg \
	integrationapplet-nimf-rime-simplified.svg \
	integrationapplet-nimf-rime-traditional.svg \
	integrationapplet-nimf-system-keyboard.svg \
	updater-base-apply.svg \
	updater-base-error2.svg \
	updater-base-exec.svg \
	updater-base-info.svg \
	updater-base-unkown.svg \
	updater-base.svg

iconsymbolicactionsdir = $(datadir)/icons/hicolor/scalable/actions
iconsymbolicactions_DATA =  \
	go-next-page-symbolic.svg \
	go-prev-page-symbolic.svg \
	integrationapplet-system-settings-symbolic.svg \
	integrationapplet-system-shutdown-symbolic.svg \
	integrationapplet-system-lock-screen-symbolic.svg \
	integrationapplet-system-suspend-symbolic.svg \
	integrationapplet-system-hibernate-symbolic.svg \
	integrationapplet-system-reboot-symbolic.svg \
	integrationapplet-system-logout-symbolic.svg

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
