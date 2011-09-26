@INTLTOOL_DESKTOP_RULE@

desktopdir       = $(sysconfdir)/xdg/autostart
desktop_in_files = tihos.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

helperdir = $(libdir)/tihos
helper_SCRIPTS = 3g btnetfw forceusbpower gprsoff gprson hostmode_switch literki_switch rotate toggle_tray wlan screenkeepon screenonswitch

EXTRA_DIST= $(helper_SCRIPTS) \
            $(desktop_in_files)

CLEANFILES = $(desktop_DATA)
