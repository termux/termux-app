include cpprules.in

CPP_FILES_FLAGS = \
	-DAPPLE_APPLICATION_NAME="$(APPLE_APPLICATION_NAME)" \
	-DBUNDLE_ID_PREFIX="$(BUNDLE_ID_PREFIX)" \
	-DBUNDLE_VERSION="$(BUNDLE_VERSION)" \
	-DBUNDLE_VERSION_STRING="$(BUNDLE_VERSION_STRING)"

if XQUARTZ_SPARKLE
CPP_FILES_FLAGS += -DXQUARTZ_SPARKLE -DXQUARTZ_SPARKLE_FEED_URL="$(XQUARTZ_SPARKLE_FEED_URL)"
endif

install-data-hook:
	$(srcdir)/mk_bundke.sh $(srcdir) $(builddir) $(DESTDIR)$(APPLE_APPLICATIONS_DIR)/$(APPLE_APPLICATION_NAME).app install

uninstall-hook:
	$(RM) -rf $(DESTDIR)$(APPLE_APPLICATIONS_DIR)/$(APPLE_APPLICATION_NAME).app/Contents/Resources
	$(RM) -rf $(DESTDIR)$(APPLE_APPLICATIONS_DIR)/$(APPLE_APPLICATION_NAME).app/Contents/Info.plist
	$(RM) -rf $(DESTDIR)$(APPLE_APPLICATIONS_DIR)/$(APPLE_APPLICATION_NAME).app/Contents/PkgInfo

noinst_PRE = Info.plist.cpp
noinst_DATA = $(noinst_PRE:plist.cpp=plist)

CLEANFILES = $(noinst_DATA)

EXTRA_DIST = \
	chown-bundle.sh \
	mk_bundke.sh \
	X11.sh \
	Info.plist.cpp \
	PkgInfo \
	$(resource_DATA) \
	Resources/ar.lproj/Localizable.strings \
	Resources/ar.lproj/locversion.plist \
	Resources/ar.lproj/main.nib/designable.nib \
	Resources/ar.lproj/main.nib/keyedobjects.nib \
	Resources/ar.lproj/main.nib/keyedobjects-110000.nib \
	Resources/ca.lproj/Localizable.strings \
	Resources/ca.lproj/locversion.plist \
	Resources/ca.lproj/main.nib/designable.nib \
	Resources/ca.lproj/main.nib/keyedobjects.nib \
	Resources/ca.lproj/main.nib/keyedobjects-110000.nib \
	Resources/cs.lproj/Localizable.strings \
	Resources/cs.lproj/locversion.plist \
	Resources/cs.lproj/main.nib/designable.nib \
	Resources/cs.lproj/main.nib/keyedobjects.nib \
	Resources/cs.lproj/main.nib/keyedobjects-110000.nib \
	Resources/da.lproj/Localizable.strings \
	Resources/da.lproj/locversion.plist \
	Resources/da.lproj/main.nib/designable.nib \
	Resources/da.lproj/main.nib/keyedobjects.nib \
	Resources/da.lproj/main.nib/keyedobjects-110000.nib \
	Resources/Dutch.lproj/Localizable.strings \
	Resources/Dutch.lproj/locversion.plist \
	Resources/Dutch.lproj/main.nib/designable.nib \
	Resources/Dutch.lproj/main.nib/keyedobjects.nib \
	Resources/Dutch.lproj/main.nib/keyedobjects-110000.nib \
	Resources/el.lproj/Localizable.strings \
	Resources/el.lproj/locversion.plist \
	Resources/el.lproj/main.nib/designable.nib \
	Resources/el.lproj/main.nib/keyedobjects.nib \
	Resources/el.lproj/main.nib/keyedobjects-110000.nib \
	Resources/English.lproj/Localizable.strings \
	Resources/English.lproj/locversion.plist \
	Resources/English.lproj/main.nib/designable.nib \
	Resources/English.lproj/main.nib/keyedobjects.nib \
	Resources/English.lproj/main.nib/keyedobjects-110000.nib \
	Resources/fi.lproj/Localizable.strings \
	Resources/fi.lproj/locversion.plist \
	Resources/fi.lproj/main.nib/designable.nib \
	Resources/fi.lproj/main.nib/keyedobjects.nib \
	Resources/fi.lproj/main.nib/keyedobjects-110000.nib \
	Resources/French.lproj/Localizable.strings \
	Resources/French.lproj/locversion.plist \
	Resources/French.lproj/main.nib/designable.nib \
	Resources/French.lproj/main.nib/keyedobjects.nib \
	Resources/French.lproj/main.nib/keyedobjects-110000.nib \
	Resources/German.lproj/Localizable.strings \
	Resources/German.lproj/locversion.plist \
	Resources/German.lproj/main.nib/designable.nib \
	Resources/German.lproj/main.nib/keyedobjects.nib \
	Resources/German.lproj/main.nib/keyedobjects-110000.nib \
	Resources/he.lproj/Localizable.strings \
	Resources/he.lproj/locversion.plist \
	Resources/he.lproj/main.nib/designable.nib \
	Resources/he.lproj/main.nib/keyedobjects.nib \
	Resources/he.lproj/main.nib/keyedobjects-110000.nib \
	Resources/hr.lproj/Localizable.strings \
	Resources/hr.lproj/locversion.plist \
	Resources/hr.lproj/main.nib/designable.nib \
	Resources/hr.lproj/main.nib/keyedobjects.nib \
	Resources/hr.lproj/main.nib/keyedobjects-110000.nib \
	Resources/hu.lproj/Localizable.strings \
	Resources/hu.lproj/locversion.plist \
	Resources/hu.lproj/main.nib/designable.nib \
	Resources/hu.lproj/main.nib/keyedobjects.nib \
	Resources/hu.lproj/main.nib/keyedobjects-110000.nib \
	Resources/Italian.lproj/Localizable.strings \
	Resources/Italian.lproj/locversion.plist \
	Resources/Italian.lproj/main.nib/designable.nib \
	Resources/Italian.lproj/main.nib/keyedobjects.nib \
	Resources/Italian.lproj/main.nib/keyedobjects-110000.nib \
	Resources/Japanese.lproj/Localizable.strings \
	Resources/Japanese.lproj/locversion.plist \
	Resources/Japanese.lproj/main.nib/designable.nib \
	Resources/Japanese.lproj/main.nib/keyedobjects.nib \
	Resources/Japanese.lproj/main.nib/keyedobjects-110000.nib \
	Resources/ko.lproj/Localizable.strings \
	Resources/ko.lproj/locversion.plist \
	Resources/ko.lproj/main.nib/designable.nib \
	Resources/ko.lproj/main.nib/keyedobjects.nib \
	Resources/ko.lproj/main.nib/keyedobjects-110000.nib \
	Resources/no.lproj/Localizable.strings \
	Resources/no.lproj/locversion.plist \
	Resources/no.lproj/main.nib/designable.nib \
	Resources/no.lproj/main.nib/keyedobjects.nib \
	Resources/no.lproj/main.nib/keyedobjects-110000.nib \
	Resources/pl.lproj/Localizable.strings \
	Resources/pl.lproj/locversion.plist \
	Resources/pl.lproj/main.nib/designable.nib \
	Resources/pl.lproj/main.nib/keyedobjects.nib \
	Resources/pl.lproj/main.nib/keyedobjects-110000.nib \
	Resources/pt.lproj/Localizable.strings \
	Resources/pt.lproj/locversion.plist \
	Resources/pt.lproj/main.nib/designable.nib \
	Resources/pt.lproj/main.nib/keyedobjects.nib \
	Resources/pt.lproj/main.nib/keyedobjects-110000.nib \
	Resources/pt_PT.lproj/Localizable.strings \
	Resources/pt_PT.lproj/locversion.plist \
	Resources/pt_PT.lproj/main.nib/designable.nib \
	Resources/pt_PT.lproj/main.nib/keyedobjects.nib \
	Resources/pt_PT.lproj/main.nib/keyedobjects-110000.nib \
	Resources/ro.lproj/Localizable.strings \
	Resources/ro.lproj/locversion.plist \
	Resources/ro.lproj/main.nib/designable.nib \
	Resources/ro.lproj/main.nib/keyedobjects.nib \
	Resources/ro.lproj/main.nib/keyedobjects-110000.nib \
	Resources/ru.lproj/Localizable.strings \
	Resources/ru.lproj/locversion.plist \
	Resources/ru.lproj/main.nib/designable.nib \
	Resources/ru.lproj/main.nib/keyedobjects.nib \
	Resources/ru.lproj/main.nib/keyedobjects-110000.nib \
	Resources/sk.lproj/Localizable.strings \
	Resources/sk.lproj/locversion.plist \
	Resources/sk.lproj/main.nib/designable.nib \
	Resources/sk.lproj/main.nib/keyedobjects.nib \
	Resources/sk.lproj/main.nib/keyedobjects-110000.nib \
	Resources/Spanish.lproj/Localizable.strings \
	Resources/Spanish.lproj/locversion.plist \
	Resources/Spanish.lproj/main.nib/designable.nib \
	Resources/Spanish.lproj/main.nib/keyedobjects.nib \
	Resources/Spanish.lproj/main.nib/keyedobjects-110000.nib \
	Resources/sv.lproj/Localizable.strings \
	Resources/sv.lproj/locversion.plist \
	Resources/sv.lproj/main.nib/designable.nib \
	Resources/sv.lproj/main.nib/keyedobjects.nib \
	Resources/sv.lproj/main.nib/keyedobjects-110000.nib \
	Resources/th.lproj/Localizable.strings \
	Resources/th.lproj/locversion.plist \
	Resources/th.lproj/main.nib/designable.nib \
	Resources/th.lproj/main.nib/keyedobjects.nib \
	Resources/th.lproj/main.nib/keyedobjects-110000.nib \
	Resources/tr.lproj/Localizable.strings \
	Resources/tr.lproj/locversion.plist \
	Resources/tr.lproj/main.nib/designable.nib \
	Resources/tr.lproj/main.nib/keyedobjects.nib \
	Resources/tr.lproj/main.nib/keyedobjects-110000.nib \
	Resources/uk.lproj/Localizable.strings \
	Resources/uk.lproj/locversion.plist \
	Resources/uk.lproj/main.nib/designable.nib \
	Resources/uk.lproj/main.nib/keyedobjects.nib \
	Resources/uk.lproj/main.nib/keyedobjects-110000.nib \
	Resources/X11.icns \
	Resources/zh_CN.lproj/Localizable.strings \
	Resources/zh_CN.lproj/locversion.plist \
	Resources/zh_CN.lproj/main.nib/designable.nib \
	Resources/zh_CN.lproj/main.nib/keyedobjects.nib \
	Resources/zh_CN.lproj/main.nib/keyedobjects-110000.nib \
	Resources/zh_TW.lproj/Localizable.strings \
	Resources/zh_TW.lproj/locversion.plist \
	Resources/zh_TW.lproj/main.nib/designable.nib \
	Resources/zh_TW.lproj/main.nib/keyedobjects.nib \
	Resources/zh_TW.lproj/main.nib/keyedobjects-110000.nib
