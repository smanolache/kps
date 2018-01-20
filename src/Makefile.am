#SUBDIRS = pics

# set the include path for X, qt and KDE
#INCLUDES = \
	-DPACKAGE_KDE_SOUND_DIR=\""$(prefix)/share/sounds/"\" \
	$(all_includes) \
	@PACKAGE_CFLAGS@


# let automoc handle all of the meta source files (moc)
#METASOURCES = AUTO

#KDE_ICON = kpowersave

lib_LTLIBRARIES = libkdeinit_kpowersave.la
bin_PROGRAMS = kpowersave

kpowersave_SOURCES = \
	main.cpp

kpowersave_LDADD = \
	libkdeinit_kpowersave.la \
	-lkio \
	-lkdecore \
	-ldbus-qt-1 \
	-lqt-mt \
	-lXss \
	-lXtst \
	-lhal \
	-ldbus-1

kpowersave_CXXFLAGS = \
	-DPACKAGE_KDE_SOUND_DIR=\""$(prefix)/share/sounds/"\" \
	-I/usr/share/qt3/include \
	-I/usr/include/kde \
	-I/usr/share/qt3/include \
	-I/usr/include/dbus-1.0 \
	-I/usr/lib/x86_64-linux-gnu/dbus-1.0/include \
	-I/usr/include/hal \
	-DDBUS_API_SUBJECT_TO_CHANGE \
	-DQT_THREAD_SUPPORT \
	-D_REENTRANT

libkdeinit_kpowersave_la_CXXFLAGS = \
	-DPACKAGE_KDE_SOUND_DIR=\""$(prefix)/share/sounds/"\" \
	-I/usr/share/qt3/include \
	-I/usr/include/kde \
	-I/usr/share/qt3/include \
	-I/usr/include/dbus-1.0 \
	-I/usr/lib/x86_64-linux-gnu/dbus-1.0/include \
	-I/usr/include/hal \
	-DDBUS_API_SUBJECT_TO_CHANGE \
	-DQT_THREAD_SUPPORT \
	-D_REENTRANT

# the application source, library search path, and link libraries
libkdeinit_kpowersave_la_SOURCES =                      \
	$(BUILT_SOURCES)                \
	dummy.cpp                       \
	autodimm.cpp			\
	autosuspend.cpp                 \
	blacklisteditdialog.cpp         \
	configuredialog.cpp             \
	countdowndialog.cpp		\
	dbusHAL.cpp                     \
	detaileddialog.cpp              \
	hardware.cpp                    \
	hardware_battery.cpp            \
	hardware_batteryCollection.cpp  \
	hardware_cpu.cpp                \
	inactivity.cpp                  \
	infodialog.cpp                  \
	kpowersave.cpp                  \
	logviewer.cpp			\
	screen.cpp                      \
	settings.cpp                    \
	suspenddialog.cpp               \
	autodimm.h			\
	autosuspend.h                   \
	blacklisteditdialog.h           \
	configuredialog.h               \
	countdowndialog.h		\
	dbusHAL.h                       \
	detaileddialog.h                \
	hardware.h                      \
	hardware_battery.h              \
	hardware_batteryCollection.h    \
	hardware_cpu.h                  \
	inactivity.h                    \
	infodialog.h                    \
	kpowersave.h                    \
	logviewer.h			\
	privileges.h			\
	screen.h                        \
	settings.h                      \
	suspenddialog.h

BUILT_SOURCES = \
	autodimm.moc \
	autosuspend.moc \
	blacklisteditdialog.moc \
	configuredialog.moc \
	countdowndialog.moc \
	dbusHAL.moc \
	detaileddialog.moc \
	hardware.moc \
	hardware_battery.moc \
	hardware_batteryCollection.moc \
	hardware_cpu.moc \
	inactivity.moc \
	infodialog.moc \
	kpowersave.moc \
	logviewer.moc \
	screen.moc \
	suspenddialog.moc \
	\
	kpowersave_skel.cpp \
	blacklistedit_Dialog.cpp blacklistedit_Dialog.h \
	configure_Dialog.cpp configure_Dialog.h \
	countdown_Dialog.cpp countdown_Dialog.h \
	detailed_Dialog.cpp detailed_Dialog.h \
	info_Dialog.cpp info_Dialog.h \
	log_viewer.cpp log_viewer.h \
	suspend_Dialog.cpp suspend_Dialog.h

kpowersave_skel.cpp: kpowersave.kidl
	dcopidl2cpp --c++-suffix cpp --no-signals --no-stub $<

kpowersave.kidl: kpowersave.h
	dcopidl $< > $@

############################# .cpp from .h and .moc ########################

blacklistedit_Dialog.cpp: blacklistedit_Dialog.h blacklistedit_Dialog.ui blacklistedit_Dialog.moc
	echo '#include <kdialog.h>' > $@
	echo '#include <klocale.h>' >> $@
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload -tr tr2i18n -i $(@:.cpp=.h) $(top_srcdir)/src/$(@:.cpp=.ui) | perl -pe "s,tr2i18n( \"\" ),QString::null,g" | perl -pe "s,tr2i18n( \"\"\, \"\" ),QString::null,g" | perl -pe "s,image([0-9][0-9]*)_data,img\$1_blacklistedit_Dialog,g" | perl -pe "s,: QWizard\(,: KWizard(,g" >> $@
	echo '#include "'$(@:.cpp=.moc)'"' >> $@

configure_Dialog.cpp: configure_Dialog.h configure_Dialog.ui configure_Dialog.moc
	echo '#include <kdialog.h>' > $@
	echo '#include <klocale.h>' >> $@
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload -tr tr2i18n -i $(@:.cpp=.h) $(top_srcdir)/src/$(@:.cpp=.ui) | perl -pe "s,tr2i18n( \"\" ),QString::null,g" | perl -pe "s,tr2i18n( \"\"\, \"\" ),QString::null,g" | perl -pe "s,image([0-9][0-9]*)_data,img\$1_blacklistedit_Dialog,g" | perl -pe "s,: QWizard\(,: KWizard(,g" >> $@
	echo '#include "'$(@:.cpp=.moc)'"' >> $@

countdown_Dialog.cpp: countdown_Dialog.h countdown_Dialog.ui countdown_Dialog.moc
	echo '#include <kdialog.h>' > $@
	echo '#include <klocale.h>' >> $@
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload -tr tr2i18n -i $(@:.cpp=.h) $(top_srcdir)/src/$(@:.cpp=.ui) | perl -pe "s,tr2i18n( \"\" ),QString::null,g" | perl -pe "s,tr2i18n( \"\"\, \"\" ),QString::null,g" | perl -pe "s,image([0-9][0-9]*)_data,img\$1_blacklistedit_Dialog,g" | perl -pe "s,: QWizard\(,: KWizard(,g" >> $@
	echo '#include "'$(@:.cpp=.moc)'"' >> $@

detailed_Dialog.cpp: detailed_Dialog.h detailed_Dialog.ui detailed_Dialog.moc
	echo '#include <kdialog.h>' > $@
	echo '#include <klocale.h>' >> $@
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload -tr tr2i18n -i $(@:.cpp=.h) $(top_srcdir)/src/$(@:.cpp=.ui) | perl -pe "s,tr2i18n( \"\" ),QString::null,g" | perl -pe "s,tr2i18n( \"\"\, \"\" ),QString::null,g" | perl -pe "s,image([0-9][0-9]*)_data,img\$1_blacklistedit_Dialog,g" | perl -pe "s,: QWizard\(,: KWizard(,g" >> $@
	echo '#include "'$(@:.cpp=.moc)'"' >> $@

info_Dialog.cpp: info_Dialog.h info_Dialog.ui info_Dialog.moc
	echo '#include <kdialog.h>' > $@
	echo '#include <klocale.h>' >> $@
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload -tr tr2i18n -i $(@:.cpp=.h) $(top_srcdir)/src/$(@:.cpp=.ui) | perl -pe "s,tr2i18n( \"\" ),QString::null,g" | perl -pe "s,tr2i18n( \"\"\, \"\" ),QString::null,g" | perl -pe "s,image([0-9][0-9]*)_data,img\$1_blacklistedit_Dialog,g" | perl -pe "s,: QWizard\(,: KWizard(,g" >> $@
	echo '#include "'$(@:.cpp=.moc)'"' >> $@

log_viewer.cpp: log_viewer.h log_viewer.ui log_viewer.moc
	echo '#include <kdialog.h>' > $@
	echo '#include <klocale.h>' >> $@
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload -tr tr2i18n -i $(@:.cpp=.h) $(top_srcdir)/src/$(@:.cpp=.ui) | perl -pe "s,tr2i18n( \"\" ),QString::null,g" | perl -pe "s,tr2i18n( \"\"\, \"\" ),QString::null,g" | perl -pe "s,image([0-9][0-9]*)_data,img\$1_blacklistedit_Dialog,g" | perl -pe "s,: QWizard\(,: KWizard(,g" >> $@
	echo '#include "'$(@:.cpp=.moc)'"' >> $@

suspend_Dialog.cpp: suspend_Dialog.h suspend_Dialog.ui suspend_Dialog.moc
	echo '#include <kdialog.h>' > $@
	echo '#include <klocale.h>' >> $@
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload -tr tr2i18n -i $(@:.cpp=.h) $(top_srcdir)/src/$(@:.cpp=.ui) | perl -pe "s,tr2i18n( \"\" ),QString::null,g" | perl -pe "s,tr2i18n( \"\"\, \"\" ),QString::null,g" | perl -pe "s,image([0-9][0-9]*)_data,img\$1_blacklistedit_Dialog,g" | perl -pe "s,: QWizard\(,: KWizard(,g" >> $@
	echo '#include "'$(@:.cpp=.moc)'"' >> $@

############################ .moc from .h #################################

blacklistedit_Dialog.moc: blacklistedit_Dialog.h
	/usr/share/qt3/bin/moc $< -o $@

configure_Dialog.moc: configure_Dialog.h
	/usr/share/qt3/bin/moc $< -o $@

countdown_Dialog.moc: countdown_Dialog.h
	/usr/share/qt3/bin/moc $< -o $@

detailed_Dialog.moc: detailed_Dialog.h
	/usr/share/qt3/bin/moc $< -o $@

info_Dialog.moc: info_Dialog.h
	/usr/share/qt3/bin/moc $< -o $@

log_viewer.moc: log_viewer.h
	/usr/share/qt3/bin/moc $< -o $@

suspend_Dialog.moc: suspend_Dialog.h
	/usr/share/qt3/bin/moc $< -o $@



autodimm.moc: autodimm.h
	/usr/share/qt3/bin/moc $< -o $@

autosuspend.moc: autosuspend.h
	/usr/share/qt3/bin/moc $< -o $@

blacklisteditdialog.moc: blacklisteditdialog.h
	/usr/share/qt3/bin/moc $< -o $@

configuredialog.moc: configuredialog.h
	/usr/share/qt3/bin/moc $< -o $@

countdowndialog.moc: countdowndialog.h
	/usr/share/qt3/bin/moc $< -o $@

dbusHAL.moc: dbusHAL.h
	/usr/share/qt3/bin/moc $< -o $@

detaileddialog.moc: detaileddialog.h
	/usr/share/qt3/bin/moc $< -o $@

hardware.moc: hardware.h
	/usr/share/qt3/bin/moc $< -o $@

hardware_battery.moc: hardware_battery.h
	/usr/share/qt3/bin/moc $< -o $@

hardware_batteryCollection.moc: hardware_batteryCollection.h
	/usr/share/qt3/bin/moc $< -o $@

hardware_cpu.moc: hardware_cpu.h
	/usr/share/qt3/bin/moc $< -o $@

inactivity.moc: inactivity.h
	/usr/share/qt3/bin/moc $< -o $@

infodialog.moc: infodialog.h
	/usr/share/qt3/bin/moc $< -o $@

kpowersave.moc: kpowersave.h
	/usr/share/qt3/bin/moc $< -o $@

logviewer.moc: logviewer.h
	/usr/share/qt3/bin/moc $< -o $@

screen.moc: screen.h
	/usr/share/qt3/bin/moc $< -o $@

suspenddialog.moc: suspenddialog.h
	/usr/share/qt3/bin/moc $< -o $@

######################### .h from .ui ###########################

blacklistedit_Dialog.h: blacklistedit_Dialog.ui
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload $< | perl -pe "s,public QWizard,public KWizard,g; s,#include <qwizard.h>,#include <kwizard.h>,g" >> $@

configure_Dialog.h: configure_Dialog.ui
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload $< | perl -pe "s,public QWizard,public KWizard,g; s,#include <qwizard.h>,#include <kwizard.h>,g" >> $@

countdown_Dialog.h: countdown_Dialog.ui
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload $< | perl -pe "s,public QWizard,public KWizard,g; s,#include <qwizard.h>,#include <kwizard.h>,g" >> $@

detailed_Dialog.h: detailed_Dialog.ui
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload $< | perl -pe "s,public QWizard,public KWizard,g; s,#include <qwizard.h>,#include <kwizard.h>,g" >> $@

info_Dialog.h: info_Dialog.ui
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload $< | perl -pe "s,public QWizard,public KWizard,g; s,#include <qwizard.h>,#include <kwizard.h>,g" >> $@

log_viewer.h: log_viewer.ui
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload $< | perl -pe "s,public QWizard,public KWizard,g; s,#include <qwizard.h>,#include <kwizard.h>,g" >> $@

suspend_Dialog.h: suspend_Dialog.ui
	/usr/share/qt3/bin/uic -L /usr/lib/kde3/plugins/designer -nounload $< | perl -pe "s,public QWizard,public KWizard,g; s,#include <qwizard.h>,#include <kwizard.h>,g" >> $@


#kpowersave_la_LDFLAGS = $(KDE_RPATH) $(all_libraries) -module -avoid-version
#kpowersave_la_LIBADD = $(LIB_KIO) @PACKAGE_LIBS@ -ldbus-qt-1 -lXss -lXext -lXtst

# this is where the desktop file will go 
#xdg_apps_DATA = kpowersave.desktop

# and for autostart
#autostartdir = $(prefix)/share/autostart
#autostart_DATA = kpowersave-autostart.desktop

# this is where the shell's XML-GUI resource file goes
#shellrcdir   = $(kde_datadir)/kpowersave
#shellrc_DATA = eventsrc 
# shellrc_DATA = kpowersaveui.rc

# pics subdir
#SUBDIRS = pics
#messages:
#	LIST=`find . -name \*.h -o -name \*.H -o -name \*.cpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		@XGETTEXT@ $$LIST -kI18N_NOOP -ki18n -ktr2i18n -o ../po/kpowersave.pot; \
	fi

#install-data-local:
#	$(mkinstalldirs) $(DESTDIR)$(kde_confdir)
#	$(INSTALL_DATA) $(top_srcdir)/src/config/kpowersaverc_default $(DESTDIR)$(kde_confdir)/kpowersaverc


