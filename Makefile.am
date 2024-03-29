
SUBDIRS = po Utils Tokenize SQL Collect IndexSearch IndexSearch/Xapian Monitor Core UI/GTK3/src

dist-hook:
	@if test -d "$(srcdir)/.git"; \
	then \
		echo Creating ChangeLog && \
		( cd "$(top_srcdir)" && \
		  git log --decorate ) > ChangeLog; \
	else \
		echo A git clone is required to generate a ChangeLog >&2; \
	fi

EXTRA_DIST = AUTHORS ChangeLog ChangeLog-dijon ChangeLog-svn FAQ NEWS README TODO \
	Tokenize/filters/external-filters.xml globalconfig.xml \
	textcat*conf.txt pinot*.desktop pinot.spec \
	IndexSearch/Plugins/*src IndexSearch/Plugins/*.xml \
	Core/pinot-*.1 IndexSearch/pinot-*.1 \
	UI/GTK3/src/pinot.1 Core/pinot-dbus-daemon.xml \
	Core/com.github.fabricecolin.Pinot.search-provider.ini \
	Core/com.github.fabricecolin.Pinot.service \
	UI/icons/48x48/pinot.png UI/icons/32x32/pinot.png \
	UI/icons/24x24/pinot.png UI/icons/22x22/pinot.png \
	UI/icons/16x16/pinot.png \
	UI/GTK3/metase-gtk3.gtkbuilder \
	scripts/bash/*.sh

if HAVE_DBUS
man_MANS = Core/pinot-index.1 \
	IndexSearch/pinot-label.1 \
	Core/pinot-search.1 \
	Core/pinot-dbus-daemon.1 \
	UI/GTK3/src/pinot.1
else
man_MANS = Core/pinot-index.1 \
	Core/pinot-search.1 \
	Core/pinot-daemon.1 \
	UI/GTK3/src/pinot.1
endif

dbus-code:
	@echo "Did you remove the SearchProvider2 interface?"
	@gdbus-codegen-glibmm3 --generate-cpp-code=PinotDBus ./Core/pinot-dbus-daemon.xml
	@mv PinotDBus_common.* Utils/
	@mv PinotDBus_stub.* Core/
	@mv PinotDBus_proxy.* IndexSearch/
	@gdbus-codegen-glibmm3 --generate-cpp-code=SearchProvider /usr/share/dbus-1/interfaces/org.gnome.ShellSearchProvider2.xml
	@mv SearchProvider_common.* SearchProvider_stub.* Core/
	@rm -f SearchProvider_proxy.*

builder-translations:
	@scripts/bash/extract-gtk-builder-translations.sh >UI/GTK3/src/BuilderTranslations.h

manuals:
	@help2man --no-info --no-discard-stderr --name "Index documents from the command-line" Core/pinot-index >Core/pinot-index.1
if HAVE_DBUS
	@help2man --no-info --no-discard-stderr --name "Label files from the command-line" IndexSearch/pinot-label     >IndexSearch/pinot-label.1
endif
	@help2man --no-info --no-discard-stderr --name "Query search engines from the command-line" Core/pinot-search  >Core/pinot-search.1
if HAVE_DBUS
	@help2man --no-info --no-discard-stderr --name "D-Bus search and index daemon" Core/pinot-dbus-daemon          >Core/pinot-dbus-daemon.1
else
	@help2man --no-info --no-discard-stderr --name "Search and index daemon" Core/pinot-daemon                     >Core/pinot-daemon.1
endif
	@help2man --no-info --no-discard-stderr --name "A metasearch tool for the Free Desktop" UI/GTK3/src/pinot      >UI/GTK3/src/pinot.1

install-data-local:
	@ln -fs $(bindir)/pinot $(DESTDIR)$(bindir)/pinot-prefs
	@mkdir -p $(DESTDIR)$(sysconfdir)/pinot
	$(INSTALL_DATA) $(srcdir)/Tokenize/filters/external-filters.xml $(DESTDIR)$(sysconfdir)/pinot/external-filters.xml
	$(INSTALL_DATA) $(srcdir)/globalconfig.xml $(DESTDIR)$(sysconfdir)/pinot/globalconfig.xml
	$(INSTALL_DATA) $(srcdir)/textcat*conf.txt $(DESTDIR)$(sysconfdir)/pinot/
	@mkdir -p $(DESTDIR)$(datadir)/pinot
if HAVE_DBUS
	$(INSTALL_DATA) $(srcdir)/Core/pinot-dbus-daemon.xml $(DESTDIR)$(datadir)/pinot/pinot-dbus-daemon.xml
	@mkdir -p $(DESTDIR)$(datadir)/dbus-1/services
	$(INSTALL_DATA) $(builddir)/Core/com.github.fabricecolin.Pinot.service $(DESTDIR)$(datadir)/dbus-1/services/com.github.fabricecolin.Pinot.service
endif
	$(INSTALL_DATA) $(srcdir)/UI/GTK3/metase-gtk3.gtkbuilder $(DESTDIR)$(datadir)/pinot/metase-gtk3.gtkbuilder
	@mkdir -p $(DESTDIR)$(datadir)/pinot/engines
if HAVE_BOOST_SPIRIT
	$(INSTALL_DATA) $(srcdir)/IndexSearch/Plugins/*.src $(DESTDIR)$(datadir)/pinot/engines/
endif
	$(INSTALL_DATA) $(srcdir)/IndexSearch/Plugins/*.xml $(DESTDIR)$(datadir)/pinot/engines/
	@mkdir -p $(DESTDIR)$(libdir)/pinot/filters
	@rm -f $(DESTDIR)$(libdir)/lib*filter.a $(DESTDIR)$(libdir)/lib*filter.la
	@mv $(DESTDIR)$(libdir)/lib*filter* $(DESTDIR)$(libdir)/pinot/filters/
	@mkdir -p $(DESTDIR)$(libdir)/pinot/backends
	@rm -f $(DESTDIR)$(libdir)/lib*backend.a $(DESTDIR)$(libdir)/lib*backend.la
	@mv $(DESTDIR)$(libdir)/lib*backend* $(DESTDIR)$(libdir)/pinot/backends/
	@mkdir -p $(DESTDIR)$(datadir)/pinot/stopwords
	@mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/48x48/apps/
	$(INSTALL_DATA) $(srcdir)/UI/icons/48x48/pinot.png $(DESTDIR)$(datadir)/icons/hicolor/48x48/apps/pinot.png
	@mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/UI/icons/32x32/pinot.png $(DESTDIR)$(datadir)/icons/hicolor/32x32/apps/pinot.png
	@mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/24x24/apps/
	$(INSTALL_DATA) $(srcdir)/UI/icons/24x24/pinot.png $(DESTDIR)$(datadir)/icons/hicolor/24x24/apps/pinot.png
	@mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/22x22/apps/
	$(INSTALL_DATA) $(srcdir)/UI/icons/22x22/pinot.png $(DESTDIR)$(datadir)/icons/hicolor/22x22/apps/pinot.png
	@mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/UI/icons/16x16/pinot.png $(DESTDIR)$(datadir)/icons/hicolor/16x16/apps/pinot.png
	@mkdir -p $(DESTDIR)$(datadir)/applications
	@desktop-file-install --vendor="" --dir=$(DESTDIR)$(datadir)/applications $(srcdir)/pinot.desktop
	@desktop-file-install --vendor="" --dir=$(DESTDIR)$(datadir)/applications $(srcdir)/pinot-prefs.desktop
	@mkdir -p $(DESTDIR)${sysconfdir}/xdg/autostart
if HAVE_DBUS
	@desktop-file-install --vendor="" --dir=$(DESTDIR)${sysconfdir}/xdg/autostart $(srcdir)/pinot-dbus-daemon.desktop
	@mkdir -p $(DESTDIR)$(datadir)/gnome-shell/search-providers
	$(INSTALL_DATA) $(builddir)/Core/com.github.fabricecolin.Pinot.search-provider.ini $(DESTDIR)${datadir}/gnome-shell/search-providers/com.github.fabricecolin.Pinot.search-provider.ini
endif
	$(INSTALL_DATA) $(srcdir)/scripts/bash/*.sh $(DESTDIR)$(datadir)/pinot/

uninstall-local:
	@rm -rf $(DESTDIR)$(sysconfdir)/pinot
	@rm -rf $(DESTDIR)$(datadir)/pinot
	@rm -rf $(DESTDIR)$(datadir)/dbus-1/services/com.github.fabricecolin.Pinot.service
	@rm -rf $(DESTDIR)${datadir}/gnome-shell/search-providers/com.github.fabricecolin.Pinot.search-provider.ini
	@rm -rf $(DESTDIR)$(libdir)/pinot
	@rm -rf $(DESTDIR)$(datadir)/icons/hicolor/48x48/apps/pinot.png
	@rm -rf $(DESTDIR)$(datadir)/icons/hicolor/32x32/apps/pinot.png
	@rm -rf $(DESTDIR)$(datadir)/icons/hicolor/24x24/apps/pinot.png
	@rm -rf $(DESTDIR)$(datadir)/icons/hicolor/22x22/apps/pinot.png
	@rm -rf $(DESTDIR)$(datadir)/icons/hicolor/16x16/apps/pinot.png
	@rm -rf $(DESTDIR)$(datadir)/applications/pinot.desktop
	@rm -rf $(DESTDIR)${sysconfdir}/xdg/autostart/pinot-dbus-daemon.desktop

