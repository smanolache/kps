# set the include path for X, qt and KDE
INCLUDES = \
	-DPACKAGE_KDE_SOUND_DIR=\""$(prefix)/share/sounds/"\" \
	$(all_includes) \
	@PACKAGE_CFLAGS@

# these are the headers for your project
noinst_HEADERS =                                \
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


# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

KDE_ICON = kpowersave

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables

bin_PROGRAMS =
lib_LTLIBRARIES =
kdeinit_LTLIBRARIES = kpowersave.la

# the application source, library search path, and link libraries
kpowersave_la_SOURCES =                                 \
			dummy.cpp                       \
			autodimm.cpp			\
			autosuspend.cpp                 \
			blacklistedit_Dialog.ui         \
			blacklisteditdialog.cpp         \
			configure_Dialog.ui             \
			configuredialog.cpp             \
			countdown_Dialog.ui		\
			countdowndialog.cpp		\
			dbusHAL.cpp                     \
			detailed_Dialog.ui              \
			detaileddialog.cpp              \
			hardware.cpp                    \
			hardware_battery.cpp            \
			hardware_batteryCollection.cpp  \
			hardware_cpu.cpp                \
			inactivity.cpp                  \
			info_Dialog.ui                  \
			infodialog.cpp                  \
			kpowersave.cpp                  \
			kpowersave.skel                 \
			log_viewer.ui			\
			logviewer.cpp			\
			main.cpp                        \
			screen.cpp                      \
			settings.cpp                    \
			suspenddialog.cpp               \
			suspend_Dialog.ui


kpowersave_la_LDFLAGS = $(KDE_RPATH) $(all_libraries) -module -avoid-version
kpowersave_la_LIBADD = $(LIB_KIO) @PACKAGE_LIBS@ -ldbus-qt-1 -lXss -lXext -lXtst

# this is where the desktop file will go 
xdg_apps_DATA = kpowersave.desktop

# and for autostart
autostartdir = $(prefix)/share/autostart
autostart_DATA = kpowersave-autostart.desktop

# this is where the shell's XML-GUI resource file goes
shellrcdir   = $(kde_datadir)/kpowersave
shellrc_DATA = eventsrc 
# shellrc_DATA = kpowersaveui.rc

# pics subdir
SUBDIRS = pics
messages:
	LIST=`find . -name \*.h -o -name \*.H -o -name \*.cpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		@XGETTEXT@ $$LIST -kI18N_NOOP -ki18n -ktr2i18n -o ../po/kpowersave.pot; \
	fi

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(kde_confdir)
	$(INSTALL_DATA) $(top_srcdir)/src/config/kpowersaverc_default $(DESTDIR)$(kde_confdir)/kpowersaverc


