SUBDIRS = icons

desktopdir = $(datadir)/applications
desktop_in_files = \
	empathy.desktop.in \
	empathy-accounts.desktop.in \
	$(NULL)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

gsettings_files = \
	org.gnome.Empathy.gschema.xml.in \
	$(NULL)
gsettings_SCHEMAS = $(gsettings_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

# We need to compile schemas at make time
# to run from source tree
gschemas.compiled: $(gsettings_SCHEMAS:.xml=.valid)
	$(AM_V_GEN) $(GLIB_COMPILE_SCHEMAS) --targetdir=. .

all-local: gschemas.compiled

convertdir = $(datarootdir)/GConf/gsettings
convert_DATA = \
	empathy.convert

servicefiledir = $(datadir)/dbus-1/services
servicefile_in_files = \
	org.freedesktop.Telepathy.Client.Empathy.Chat.service.in \
	org.freedesktop.Telepathy.Client.Empathy.Auth.service.in \
	org.freedesktop.Telepathy.Client.Empathy.Call.service.in \
	org.freedesktop.Telepathy.Client.Empathy.FileTransfer.service.in

if HAVE_EMPATHY_AV
servicefile_in_files += \
	org.freedesktop.Telepathy.Client.Empathy.AudioVideo.service.in
endif

servicefile_DATA = $(servicefile_in_files:.service.in=.service)

org.freedesktop.Telepathy.Client.Empathy.Chat.service: org.freedesktop.Telepathy.Client.Empathy.Chat.service.in
	$(AM_V_GEN)sed -e "s|[@]libexecdir[@]|$(libexecdir)|" $< > $@

org.freedesktop.Telepathy.Client.Empathy.AudioVideo.service: org.freedesktop.Telepathy.Client.Empathy.AudioVideo.service.in
	$(AM_V_GEN)sed -e "s|[@]libexecdir[@]|$(libexecdir)|" $< > $@

org.freedesktop.Telepathy.Client.Empathy.Auth.service: org.freedesktop.Telepathy.Client.Empathy.Auth.service.in
	$(AM_V_GEN)sed -e "s|[@]libexecdir[@]|$(libexecdir)|" $< > $@

org.freedesktop.Telepathy.Client.Empathy.Call.service: org.freedesktop.Telepathy.Client.Empathy.Call.service.in
	$(AM_V_GEN)sed -e "s|[@]libexecdir[@]|$(libexecdir)|" $< > $@

org.freedesktop.Telepathy.Client.Empathy.FileTransfer.service: org.freedesktop.Telepathy.Client.Empathy.FileTransfer.service.in
	$(AM_V_GEN)sed -e "s|[@]bindir[@]|$(bindir)|" $< > $@

clientfiledir = $(datarootdir)/telepathy/clients
clientfile_DATA = \
	Empathy.Chat.client \
	Empathy.Auth.client \
	Empathy.Call.client \
	Empathy.FileTransfer.client

if HAVE_EMPATHY_AV
clientfile_DATA += \
	Empathy.AudioVideo.client
endif

htmldir = $(datadir)/empathy
html_DATA = \
	Template.html \
	empathy-log-window.html \
	$(NULL)

EXTRA_DIST = 				\
	$(convert_DATA)			\
	$(desktop_in_files)		\
	$(desktop_DATA)			\
	$(schemas_DATA)			\
	$(html_DATA)			\
	$(clientfile_DATA)		\
	$(servicefile_in_files)		\
	$(gsettings_files)		\
	$(NULL)

DISTCLEANFILES = 		\
	$(desktop_DATA)		\
	$(schemas_DATA)		\
	$(servicefile_DATA)	\
	$(gsettings_SCHEMAS)	\
	gschemas.compiled	\
	$(NULL)
