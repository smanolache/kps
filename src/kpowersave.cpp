 /**************************************************************************
 *   Copyright (C) 2004 by  Thomas Renninger                               *
 *                            <trenn@suse.de> and                          *
 *            2004-2007 by  Danny Kukawka                                  *
 *                            <dkukawka@suse.de>, <danny.kukawka@web.de>   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License     *
 *   as published by the Free Software Foundation.                         *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

// KDE headers:
#include <kaboutapplication.h>
#include <kapplication.h>
#include <kaudioplayer.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <knotifyclient.h>
#include <knotifydialog.h>
#include <kpassivepopup.h>
#include <kpopupmenu.h>

// other Qt headers:
#include <qcursor.h>
#include <qevent.h>
#include <qfile.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qtooltip.h>

// own headers:
#include "kpowersave.h"
#include "infodialog.h"

/*! 
*  \file 	kpowersave.cpp
*  \brief 	This file contains the main functionality of the kpowersave-applet.*/

/*! 
 * This is the default constructor of the class kpowersave. 
 */
kpowersave::kpowersave( bool force_acpi_check, bool trace_func ) : KSystemTray(0, "kpowersave"),
								   DCOPObject("KPowersaveIface") {
	trace = trace_func;
	kdDebugFuncIn(trace); 

	display = new screen();
	settings = new Settings();
	autoSuspend = new autosuspend();
	autoDimm = new autodimm();
	hwinfo = new HardwareInfo();
	suspend = hwinfo->getSuspendSupport();
	
	yast2 = NULL;

	resume_result = 0;

	config = KGlobal::config();
	config->setGroup("General");
	if(!config->readBoolEntry("AlreadyStarted", false) || force_acpi_check){
		config->writeEntry("AlreadyStarted", true);
		// check whether APM, ACPI, PMU, CPUFreq or Suspend2Disk/ram supported, otherwise end up
		// and don't start kpowersave ever again until force_acpi_check == true.
		if (!hwinfo->hasACPI() && !hwinfo->hasAPM() && !hwinfo->hasPMU() && 
		    !hwinfo->supportCPUFreq() && !suspend.suspend2disk && !suspend.suspend2ram){
			config->writeEntry("Autostart", false);
			config->sync();
			kdError(debug_area) << "This machine does not support"
				"ACPI, APM, PMU, CPUFreq, Suspend2Disk nor "
				"Suspend2RAM. Close KPowersave now." << endl;
			exit(-1);
		}
	}

	// default init
	if (hwinfo->getAcAdapter()) {
		settings->load_scheme_settings( settings->ac_scheme);
	} else {
		settings->load_scheme_settings( settings->battery_scheme);
	}
	// set the battery warning levels
	hwinfo->setPrimaryBatteriesWarningLevel(settings->batteryWarningLevel, 
						settings->batteryLowLevel,
						settings->batteryCriticalLevel);

	// connect to signals for changes
	connect(hwinfo, SIGNAL(generalDataChanged()), this, SLOT(update()));
	connect(hwinfo, SIGNAL(primaryBatteryChanged()), this, SLOT(update()));
	connect(hwinfo, SIGNAL(ACStatus(bool)), this, SLOT(handleACStatusChange (bool)));
	connect(hwinfo, SIGNAL(resumed(int)), this, SLOT(forwardResumeSignal(int)));

	// connect to error mesages
	connect(autoSuspend, SIGNAL(displayErrorMsg(QString)), this, SLOT(showErrorMessage(QString)));	
	connect(hwinfo, SIGNAL(dbusRunning(int)), this, SLOT(showDBusErrorMsg(int)));

	// connect to events
	connect(hwinfo, SIGNAL(lidcloseStatus(bool)), this, SLOT(handleLidEvent(bool)));
	connect(hwinfo, SIGNAL(powerButtonPressed()), this, SLOT (handlePowerButtonEvent()));
	connect(hwinfo, SIGNAL(sleepButtonPressed()), this, SLOT (handleSleepButtonEvent()));
	connect(hwinfo, SIGNAL(s2diskButtonPressed()), this, SLOT (handleS2DiskButtonEvent()));
	connect(hwinfo, SIGNAL(batteryWARNState(int,int)), this, SLOT(notifyBatteryStatusChange (int,int)));
	connect(hwinfo, SIGNAL(desktopSessionIsActive(bool)), this, SLOT (handleSessionState(bool)));
	connect(autoSuspend, SIGNAL(inactivityTimeExpired()), this, SLOT(do_autosuspendWarn()));
	connect(autoDimm, SIGNAL(inactivityTimeExpired()), this, SLOT(do_downDimm()));
	connect(autoDimm, SIGNAL(UserIsActiveAgain()), this, SLOT(do_upDimm()));
	
	config->sync();
	
	config_dialog_shown = false;
	suspend_dialog_shown = false;
	detailedIsShown = false;
	hal_error_shown = false;
	icon_set_colored = false;
	icon_BG_is_colored = false;
	
 	calledSuspend = -1;
	countWhiteIconPixel = 0;

	pixmap_name = "NONE";
        suspendType = "NULL";

	BAT_WARN_ICON_Timer = new QTimer(this);
	connect(BAT_WARN_ICON_Timer, SIGNAL(timeout()), this, SLOT(do_setIconBG()));
	
	DISPLAY_HAL_ERROR_Timer = new QTimer(this);
	connect(DISPLAY_HAL_ERROR_Timer, SIGNAL(timeout()), this, SLOT(showHalErrorMsg()));

	AUTODIMM_Timer = new QTimer(this);

	initMenu();
	update();
	updateCPUFreqMenu();
	setSchemeSettings();

	kdDebugFuncOut(trace);
}
	

/*! This is the default destructor of class kpowersave. */
kpowersave::~kpowersave(){
	kdDebugFuncIn(trace);

	delete hwinfo;
	delete display;
	delete settings;
	delete autoSuspend;
#ifdef ENABLE_YAST_ENTRY
	delete yast2;
#endif
}

/*!
 * use this function to initalise the main kicker menu
 */
void kpowersave::initMenu() {
	kdDebugFuncIn(trace);

	CONFIGURE_ID = this->contextMenu()->insertItem(SmallIcon("configure", QIconSet::Automatic),
								  i18n("Configure KPowersave..."), 
								  this, SLOT(showConfigureDialog()));
	CONFIGURE_EVENTS_ID = this->contextMenu()->insertItem(SmallIcon("knotify", QIconSet::Automatic),
							      i18n("Configure Notifications..."), 
							      this, SLOT(showConfigureNotificationsDialog()));
#ifdef ENABLE_YAST_ENTRY
	YAST_MODULE_MENU_ID = this->contextMenu()->insertItem(SmallIcon("yast", QIconSet::Automatic),
							      i18n("Start YaST2 Power Management Module..."), 
							      this, SLOT(do_config()));
#endif
	
	SLEEP_SEPARATOR_MENU_ID = this->contextMenu()->insertSeparator();
	SUSPEND2DISK_MENU_ID = this->contextMenu()->insertItem( SmallIconSet("suspend_to_disk", 
								QIconSet::Automatic),
								i18n("Suspend to Disk"), this,
								SLOT(do_suspend2disk()));
	SUSPEND2RAM_MENU_ID = this->contextMenu()->insertItem( SmallIconSet("suspend_to_ram", 
							       QIconSet::Automatic),
							       i18n("Suspend to RAM"), this,
							       SLOT(do_suspend2ram()));
	STANDBY_MENU_ID = this->contextMenu()->insertItem( SmallIconSet("stand_by", QIconSet::Automatic),
							   i18n("Standby"), this, SLOT(do_standby()));
	
	speed_menu = new QPopupMenu(this, i18n("Set CPU Frequency Policy"));
	speed_menu->insertItem(i18n("Performance"), PERFORMANCE);
	speed_menu->insertItem(i18n("Dynamic"), DYNAMIC);
	speed_menu->insertItem(i18n("Powersave"), POWERSAVE);
	
	CPUFREQ_SEPARATOR_MENU_ID = contextMenu()->insertSeparator();
	
	CPUFREQ_MENU_ID = contextMenu()->insertItem(i18n("Set CPU Frequency Policy"), speed_menu);
	connect(speed_menu, SIGNAL(activated(int)), this, SLOT(do_setSpeedPolicy(int)));
	connect(hwinfo, SIGNAL(currentCPUFreqPolicyChanged()), this, SLOT(updateCPUFreqMenu()));


	SCHEME_SEPARATOR_MENU_ID = contextMenu()->insertSeparator();
	
	scheme_menu = new QPopupMenu(this, i18n("Set Active Scheme"));
	SCHEME_MENU_ID = contextMenu()->insertItem(i18n("Set Active Scheme"), scheme_menu);
	connect(scheme_menu, SIGNAL(activated(int)), this, SLOT(do_setActiveScheme(int)));
	
	// menu entry for the autosuspend disable checkbox, disabled by default, only 
	// displayed if autosuspend for the current scheme is activated
	AUTOSUSPEND_SEPARATOR_MENU_ID = contextMenu()->insertSeparator();
	AUTOSUSPEND_MENU_ID = this->contextMenu()->insertItem( i18n("Disable Actions on Inactivity"), 
							       this,SLOT(do_setAutosuspend()));
	this->contextMenu()->setItemVisible(AUTOSUSPEND_SEPARATOR_MENU_ID, false);
	this->contextMenu()->setItemVisible(AUTOSUSPEND_MENU_ID, false);

	HELP_SEPARATOR_MENU_ID = contextMenu()->insertSeparator();

	help_menu = new QPopupMenu(this, i18n("&Help"));
	
	help_menu->insertItem( SmallIcon("help", QIconSet::Automatic), i18n("&KPowersave Handbook"), 
			       this, SLOT(slotHelp()));
	help_menu->insertSeparator();
	help_menu->insertItem( i18n("&Report a bug ..."), this, SLOT(slotReportBug()));
	help_menu->insertItem( SmallIcon("kpowersave", QIconSet::Automatic), 
			       i18n("&About KPowersave"), this, SLOT(slotAbout()));

	HELP_MENU = contextMenu()->insertItem(SmallIcon("help", QIconSet::Automatic),
							i18n("&Help"), help_menu);
	
	connect(this, SIGNAL(quitSelected()), this, SLOT(_quit()));
	
	kdDebugFuncOut(trace);
}

/*!
 * This funtion load and manipulate the icons for the kickerapplet-section. 
 * The redraw interval depends on \ref icon_set_colored and \ref BAT_icon_BG_intervall.
 */
void kpowersave::redrawPixmap(){
	kdDebugFuncIn(trace);

	// if colored icon_background: normal redraw intervall is set off.
	// Icon (only) redrawed every BAT_icon_BG_intervall
	if (icon_set_colored) {
		if (icon_state_changed) {
			loadIcon();
			drawIcon();
		}
	}
	else {
		loadIcon();
		drawIcon();
	}

	kdDebugFuncOut(trace);
}

/*!
 * Starts the configure dialog of kpowersave.
 */
void kpowersave::showConfigureDialog() {
	kdDebugFuncIn(trace);

	if(!config_dialog_shown) {
		if (settings->schemes.count() > 0){	
			configDlg = new ConfigureDialog(config, hwinfo, settings);
 			configDlg->show();
			config_dialog_shown = true;
			connect(configDlg, SIGNAL(destroyed()), this, SLOT(observeConfigDlg()));
			connect(configDlg, SIGNAL(openHelp()), this, SLOT(slotHelp()));
			connect(configDlg, SIGNAL(openKNotify()), this, SLOT(showConfigureNotificationsDialog()));
		}
		else {
			KPassivePopup::message(i18n("WARNING"), i18n("Cannot find any schemes."),
					       SmallIcon("messagebox_warning", 20), this, 
					       i18n("Warning"), 15000);
		}
	} else {
		configDlg->setWindowState(configDlg->windowState() & ~WindowMinimized | WindowActive);
		configDlg->setActiveWindow();
	}
	
	kdDebugFuncOut(trace);
}

/*!
 * Starts the notification configure dialog of kpowersave.
 */
void kpowersave::showConfigureNotificationsDialog() {
	kdDebugFuncIn(trace);

	KNotifyDialog::configure(this);

	kdDebugFuncOut(trace);
}

/*!
 * Load the icons (from filesystem) for the kicker applet to \ref pixmap .
 * To reduce the systemload the icons are only reloaded if \ref pixmap_name
 * is changed.
 */
void kpowersave::loadIcon(){
	kdDebugFuncIn(trace);

	QString pixmap_name_tmp = "NONE";

	BatteryCollection *primary = hwinfo->getPrimaryBatteries();

	if (!hwinfo->isOnline() ) {
		pixmap_name_tmp = QString("ERROR");
	}
	else if (hwinfo->getAcAdapter() || primary->getBatteryState() == BAT_NONE) {
		icon_set_colored = false;
		
		if (primary->getBatteryState() == BAT_NONE || (primary->getRemainingPercent() < 0 ||
		    primary->getRemainingPercent() >= 99))
			pixmap_name_tmp = QString("laptoppower");
		else
			pixmap_name_tmp = QString("laptopcharge");
	}
	else {
		switch(primary->getBatteryState()) {
			case BAT_CRIT:
			case BAT_LOW:
				if (icon_BG_is_colored) pixmap_name_tmp = QString("laptopbattery");
				else pixmap_name_tmp = QString("laptopbatteryRED");
				icon_BG_is_colored = !icon_BG_is_colored;
				icon_set_colored = true;
				break;
			case BAT_WARN:
				if (icon_BG_is_colored) pixmap_name_tmp = QString("laptopbattery");
				else pixmap_name_tmp = QString("laptopbatteryORANGE");
				icon_BG_is_colored = !icon_BG_is_colored;
				icon_set_colored = true;
				break;
			default:
				// if battery is discharging and not in warning, low or critical state
				pixmap_name_tmp = QString("laptopbattery");
				icon_set_colored = false;
				icon_BG_is_colored = false;
		}
		
		if (icon_set_colored){
			icon_state_changed = false;
			BAT_WARN_ICON_Timer->start(BAT_icon_BG_intervall, true);
		}
	}

	// reload icon only if new icon selected
	if(pixmap_name_tmp != pixmap_name) {
		pixmap_name = pixmap_name_tmp;
		if (pixmap_name.startsWith("ERROR")) {
			pixmap = SmallIcon("laptoppower", 22, KIcon::DisabledState);
		}
		else 
			pixmap = SmallIcon(pixmap_name, 22);
	}
	
	kdDebugFuncOut(trace);
}


/*!
 * This function draw the battery-capacity (colored field) to the icon.
 * Here also counted the white pixel in the icon-files. Since the icons are
 * the same and white pixel only in the retangel of the icon, the white pixel
 * stored in \ref countWhiteIconPixel only one time.
 */
void kpowersave::drawIcon(){
	kdDebugFuncIn(trace);

	BatteryCollection *primary = hwinfo->getPrimaryBatteries();

	QImage image = pixmap.convertToImage();
	int w = image.width();
	int h = image.height();
	int x, y;
	
	if((pixmap_name.contains("laptopbattery") || pixmap_name.contains("charge")) && 
	    countWhiteIconPixel == 0) {
		for (x = 0; x < w; x++)
			for (y = 0; y < h; y++)
				if(QColor(image.pixel(x, y)) == Qt::white) countWhiteIconPixel++;
	}
	
	int c = (countWhiteIconPixel * primary->getRemainingPercent()) / 100;
	
	if (c > 0) {
		uint ui;
		QRgb Rgb_set;
		
		if (hwinfo->getAcAdapter()) {
			Rgb_set = qRgb(0x00, 0xff, 0x00); //green
		}
		else {
			switch(primary->getBatteryState()) {
				case BAT_CRIT:
				case BAT_LOW:
					Rgb_set = qRgb(0xff, 0x00, 0x00);//red
					break;
				case BAT_WARN:
					Rgb_set = qRgb(0xff, 0x55, 0x00); //orange
					break;
				default:
					Rgb_set = qRgb(0x00, 0xff, 0x00); //green
			}
		}
		if (image.depth() <= 8) {
			ui = image.numColors();
			image.setNumColors(ui + 1);
			image.setColor(ui, Rgb_set);
		}
		ui = 0xff000000 | Rgb_set;
		
		for (y = h - 1; y >= 0; y--) {
			for (x = 0; x < w ; x++) {
				if(QColor(image.pixel(x, y)) == Qt::white) {
					image.setPixel(x, y, ui);
					c--;
					if (c <= 0) goto quit;
				}
			}
		}
	}
quit:
	fullIcon.convertFromImage(image);
	setPixmap(fullIcon);

	kdDebugFuncOut(trace);
}

/*!
* By this function we fill and update the Tooltip for the icon in the kicker applet.
* The content of the tooltip alway updated, if something change.
* \todo Check if the tooltip also updated if mouse \b is over the icon, if not we maybe 
*	should implement this.\n If it is possible we should update the tooltip permanently 
*	while the mouse cursor is over the widget
*/
void kpowersave::updateTooltip() {
	kdDebugFuncIn(trace);

	BatteryCollection *primary = hwinfo->getPrimaryBatteries();
	int percent = primary->getRemainingPercent();
	int minutes = primary->getRemainingMinutes();
	kdDebug(debug_area) << "updateToolTip: remaining minutes: " << minutes
			    << endl;
	int charging_state = primary->getChargingState();

	QString tmp, num3;
	num3.setNum(minutes % 60);
	num3 = num3.rightJustify(2, '0');

	if (hwinfo->getAcAdapter()) {
		if (percent == 100)
			tmp = i18n("Plugged in -- fully charged");
		// assume that no battery is there
		else {
			if ((percent < 0 && minutes < 0) ||
			    BAT_NONE == primary->getBatteryState())
				tmp = i18n("Plugged in");
			else if (minutes > 0) {
				// BAT_NONE != getBatteryState() && minutes > 0
				if (charging_state == CHARGING)
					tmp = i18n("Plugged in -- %1% charged (%2:%3 h until full "
						   "charged)").arg(percent).arg(minutes / 60).arg(num3);
				else
					tmp = i18n("Plugged in -- %1% charged (%2:%3 remaining hours)")
						   .arg(percent).arg(minutes / 60).arg(num3);
			} else if (charging_state == CHARGING && hwinfo->hasAPM())
				tmp = i18n("Plugged in -- %1% charged").arg(percent);
			else {
				if (percent == -1)
					tmp = i18n("Plugged in -- no battery");
				else
					tmp = i18n("Plugged in -- %1% charged").arg(percent);
			}
		}
	} else {
		if (minutes >= 0)
			tmp = i18n("Running on batteries -- %1% charged (%2:%3 hours remaining)")
				   .arg(percent).arg(minutes / 60).arg(num3);
		else
			tmp = i18n("Running on batteries -- %1% charged").arg(percent);
	}
	// add string whether battery is charging, but only if < 100% to avoid
	// stupid tooltip message on machines which always with 100% and on AC 
	// are charging, as e.g. Sony Vaio FS vgn-fs115b
	if (charging_state == CHARGING && percent < 100) 
		tmp += i18n(" -- battery is charging");

	QToolTip::add(this, tmp);

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to starts the Yast2-power-management module. This called by the menuentry 
 * with ID \ref YAST_MODULE_MENU_ID, named "Start YaST2 Power Management Module".
 * It create a new KProcess and execute "/sbin/yast2 power-management" with kdesu.
 */
void kpowersave::do_config(){
	kdDebugFuncIn(trace);
	
#ifdef ENABLE_YAST_ENTRY
	delete yast2;
	
	yast2 = new KProcess;
	*yast2 << "kdesu" << "--nonewdcop" << "/sbin/yast2" << "power-management";

	connect(yast2, SIGNAL(processExited(KProcess *)),
	         SLOT(slotConfigProcessExited(KProcess *)));
	if(!yast2->start(KProcess::NotifyOnExit)) {
		delete yast2;
		yast2 = NULL;
	}
#endif
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to open the KPowersave help
 */
void kpowersave::slotHelp()
{
    kapp->invokeHelp( "", "kpowersave" );
}

/*!
 * \b SLOT to open the KPowersave About dialog
 */
void kpowersave::slotAbout()
{
 	KAboutApplication a( this );
        a.exec();
}

/*!
 * \b SLOT to open the website to report bugs
 */
void kpowersave::slotReportBug()
{
#ifdef DISTRO_IS_SUSE
 #ifdef DISTRO_IS_SLES_SLED
	kapp->invokeBrowser("https://bugzilla.novell.com/");
 #else
	kapp->invokeBrowser("http://en.opensuse.org/Submitting_Bug_Reports");
 #endif
#else 
 #ifdef DISTRO_IS_ALTLINUX
	kapp->invokeBrowser("http://bugzilla.altlinux.org/");
 #else
  #ifdef DISTRO_IS_UBUNTU
	kapp->invokeBrowser("https://launchpad.net/distros/ubuntu/+bugs");
  #else
   #ifdef DISTRO_IS_PARDUS
	kapp->invokeBrowser("http://bugs.pardus.org.tr/");
   #else
	kapp->invokeBrowser("http://sourceforge.net/tracker/?group_id=124576&atid=700009");
   #endif
  #endif
 #endif
#endif
}

/*!
 * \b SLOT to set the icon background on/off if battery is in critical, low or warning-state. Within 
 * this function we set \ref icon_state_changed to true and call \ref redrawPixmap() to redraw the 
 * kickerapplet icon and create a icon with blinking background. \n \n
 * The slot called by the QTimer \ref BAT_WARN_ICON_Timer . The interval of the timer is defined 
 * trough \ref BAT_icon_BG_intervall and starts here: \ref loadIcon() .  
 */
void kpowersave::do_setIconBG(){
	kdDebugFuncIn(trace);
	
	if (icon_set_colored) icon_state_changed = true;
	redrawPixmap();

	kdDebugFuncOut(trace);
}
/*!
 * \b SLOT to enable/disable the autosuspend.
 */
void kpowersave::do_setAutosuspend(){
	kdDebugFuncIn(trace);

	if(!contextMenu()->isItemChecked(AUTOSUSPEND_MENU_ID)) {
		autoSuspend->stop();
		contextMenu()->setItemChecked(AUTOSUSPEND_MENU_ID, true);	
	}
        else {
		if(settings->autoSuspend) {
			contextMenu()->setItemChecked(AUTOSUSPEND_MENU_ID, false);
			setAutoSuspend(false);
		}
		else {
			contextMenu()->setItemVisible(AUTOSUSPEND_MENU_ID, false);
			contextMenu()->setItemChecked(AUTOSUSPEND_MENU_ID, false);
			contextMenu()->setItemVisible(AUTOSUSPEND_SEPARATOR_MENU_ID, false);
		}
        }
	
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT which called if the \ref configDlg is destroyed. We set within this SLOT 
 * \ref config_dialog_shown to false.
 * TODO: check if we maybe should force here the current default scheme depending on the AC/battery state
 */
void kpowersave::observeConfigDlg(){
	kdDebugFuncIn(trace);
	
	// reload general settings
	settings->load_general_settings();
	// set the battery warning levels - all other general settings don't need to 
	// get set, since we check the settings only on events.
	hwinfo->setPrimaryBatteriesWarningLevel(settings->batteryWarningLevel, 
						settings->batteryLowLevel,
						settings->batteryCriticalLevel);

	// reload the maybe changed scheme settings
	settings->load_scheme_settings( settings->currentScheme );
	// set the scheme
	setSchemeSettings();	

	config_dialog_shown=false;

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT which called from \ref do_config() if the 'kdesu yast2' KProcess exited.
 * This function control the return value and display if needed a errormessage on failure.
 */
void kpowersave::slotConfigProcessExited(KProcess *proc){
	kdDebugFuncIn(trace);

#ifdef ENABLE_YAST_ENTRY
	if (proc->normalExit()){
		if (proc->exitStatus() != 0 && proc->exitStatus() != 16){
			KPassivePopup::message( i18n("WARNING"),
						i18n("Could not start YaST Power Management Module. "
						     "Check if it is installed."),
						SmallIcon("messagebox_warning", 20), this, 
						i18n("Warning"), 15000);
		}
	}
	else{
		KPassivePopup::message( i18n("WARNING"),
					i18n("Could not start YaST Power Management Module. "
					     "Check if it is installed."),
					SmallIcon("messagebox_warning", 20), this, i18n("Warning"), 15000);
	}
#endif
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to send the command for "suspend to disk" to HAL daemon.
 * If there is a error while "suspend to disk" the user get e messagebox.
 * This function need a running HAL daemon for "suspend to disk".
 * \return boolean with the result of the operation
 * \retval true if successful 
 * \retval false if command not supported or if powersaved not running
 */
bool kpowersave::do_suspend2disk(){
	kdDebugFuncIn(trace);

	if (!suspend.suspend2disk) {
		kdWarning(debug_area) << "This machine does not provide "
			"suspend2disk via HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	if (0 == suspend.suspend2disk_allowed) {
		KPassivePopup::message( i18n("WARNING"),
					i18n("Suspend to disk disabled by administrator."),
					SmallIcon("messagebox_warning", 20), 
					this, i18n("Warning"), 15000);
		this->contextMenu()->setItemEnabled(SUSPEND2DISK_MENU_ID, false);
		kdDebugFuncOut(trace);
		return false;
	}

	calledSuspend = SUSPEND2DISK;
	if (!handleMounts(true)) {
		kdWarning(debug_area) << "Could not umount ..." << endl;
		calledSuspend = -1;
		kdDebugFuncOut(trace);
		return false;
	}
	
	if(settings->lockOnSuspend) {
		display->lockScreen( settings->lockmethod );
	}
	
	autoSuspend->stop();
	autoDimm->stop();
	notifySuspend(calledSuspend);
	if (!hwinfo->suspend(SUSPEND2DISK)) {
		KPassivePopup::message( i18n("WARNING"),i18n("Suspend to disk failed"),
					SmallIcon("messagebox_warning", 20), this,
					i18n("Warning"), 15000);
		kdDebugFuncOut(trace);
		return false;
	}
	
	kdDebugFuncOut(trace);
	return true;
}

/*!
 * \b SLOT to send the command for "suspend to RAM" to the HAL daemon.
 * If there is a error while "suspend to RAM" the user get e messagebox.
 * This function need a running HAL daemon for "suspend to RAM".
 * \return boolean with the result of the operation
 * \retval true if successful 
 * \retval false if command not supported or if powersaved not running
 */
bool kpowersave::do_suspend2ram(){
	kdDebugFuncIn(trace);

	if (!suspend.suspend2ram) {
		kdWarning(debug_area) << "This machine does not provide "
			"suspend2ram." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (0 == suspend.suspend2ram_allowed) {
		KPassivePopup::message( i18n("WARNING"),
					i18n("Suspend to RAM disabled by administrator."),
					SmallIcon("messagebox_warning", 20), this, 
					i18n("Warning"), 15000);
		this->contextMenu()->setItemEnabled(SUSPEND2RAM_MENU_ID, false);
		kdDebugFuncOut(trace);
		return false;
	}

	calledSuspend = SUSPEND2RAM;
	if (!handleMounts(true)) {
		kdWarning(debug_area) << "Could not umount ..." << endl;
		calledSuspend = -1;
		kdDebugFuncOut(trace);
		return false;
	}
	
	if(settings->lockOnSuspend) {
		display->lockScreen( settings->lockmethod );
	}
	
	autoSuspend->stop();
	autoDimm->stop();
	notifySuspend(calledSuspend);
	if (!hwinfo->suspend(SUSPEND2RAM)) {
		KPassivePopup::message( i18n("WARNING"),i18n("Suspend to RAM failed"),
					SmallIcon("messagebox_warning", 20), this,
					i18n("Warning"), 15000);
		kdDebugFuncOut(trace);
		return false;
	}

	kdDebugFuncOut(trace);
	return true;
}

/*!
 * \b SLOT to send the command for "stand-by" to the HAL daemon.
 * If there is a error while "stand-by" the user get e messagebox.
 * This function need a running HAL daemon for "stand-by".
 * \return boolean with the result of the operation
 * \retval true if successful 
 * \retval false if command not supported or if powersaved not running
 */
bool kpowersave::do_standby(){
	kdDebugFuncIn(trace);

	if (!suspend.standby) {
		kdWarning(debug_area) << "This machine does not provide "
			"suspend2ram via HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (0 == suspend.standby_allowed) {
		KPassivePopup::message( i18n("WARNING"),i18n("Standby disabled by administrator."),
					SmallIcon("messagebox_warning", 20), this, 
					i18n("Warning"), 15000);
		this->contextMenu()->setItemEnabled(STANDBY_MENU_ID, false);
		kdDebugFuncOut(trace);
		return false;
	}

	calledSuspend = STANDBY;
	if (!handleMounts(true)) {
		kdWarning(debug_area) << "Could not umount ..." << endl;
		calledSuspend = -1;
		kdDebugFuncOut(trace);
		return false;
	}

	if(settings->lockOnSuspend) {
		display->lockScreen( settings->lockmethod );
	}

	autoSuspend->stop();
	autoDimm->stop();
	notifySuspend(calledSuspend);
	if (!hwinfo->suspend(STANDBY)) {
		KPassivePopup::message( i18n("WARNING"),i18n("Standby failed"),
					SmallIcon("messagebox_warning", 20), this,
					i18n("Warning"), 15000);
		kdDebugFuncOut(trace);
		return false;
	}
	
	kdDebugFuncOut(trace);
	return true;
}

/*!
 * \b SLOT to send check if we should display the warning dialog and display
 * the dialog if needed or call directly autosuspend after the signal
 * \ref autosuspend::inactivityTimeExpired was recieved. 
 */
void kpowersave::do_autosuspendWarn() {
	kdDebugFuncIn(trace);

	if (settings->autoSuspendCountdown && (settings->autoSuspendCountdownTimeout > 0)) {
		// we have to display the warn dialog
		if(!contextMenu()->isItemChecked(AUTOSUSPEND_MENU_ID)) {
			QString message;

			countdown = new countDownDialog(settings->autoSuspendCountdownTimeout);
			
			if(settings->autoInactiveAction == "Suspend to Disk") {
				countdown->setPixmap("suspend2disk");
			} else if (settings->autoInactiveAction == "Suspend to RAM") {
				countdown->setPixmap("suspend2ram");
			} else if (settings->autoInactiveAction == "Standby") {
				countdown->setPixmap("standby");
			} else {
				countdown->setPixmap("kpowersave");
			}
			
			// TODO: rework this after translation round for openSUSE 10.3 !
			message = i18n("Inactivity detected.") + " " +
				  i18n("To stop the %1 press the 'Cancel' button before the countdown "
				       "expire.").arg(i18n("Autosuspend")) + "\n\n" +
				  i18n("The computer autosuspend in: ");

			countdown->setMessageText(message);

			connect(countdown, SIGNAL(dialogClosed(bool)), this, SLOT(do_autosuspend(bool)));
			countdown->showDialog();
		}	
	} else {
		// call directly autosuspend
		do_autosuspend(false);
	}
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to send the related suspend command for autosuspend
 * \param  chancel boolean with info if the autosuspend should get chanceld
 * \return boolean with the result of the operation
 * \retval true if successful 
 * \retval false if command not supported or on any other error
 * \todo	add check if the requested command is supported befor send and 
 *		add message for this case to tell that maybe changed config!
 */
bool kpowersave::do_autosuspend(bool chancel) {
	kdDebugFuncIn(trace);
	
	// TODO: check if this is really needed, it get called also on the suspend methodes
	autoSuspend->stop();

	if (chancel) {
		kdDebug(debug_area) << "The autosuspend was cancelled (via the"
			" cancel dialog), start again." << endl;
		setAutoSuspend(false);
		return false;
	}

	if(!settings->disableNotifications) {
		KNotifyClient::event( this->winId(), "autosuspend_event", 
				      i18n("System is going into suspend mode now"));
	}
	
	if(!settings->autoSuspend ||
	   contextMenu()->isItemChecked(AUTOSUSPEND_MENU_ID)) {
		return false;
	}

	if(settings->autoInactiveAction == "Suspend to Disk")
		return do_suspend2disk();
	if (settings->autoInactiveAction == "Suspend to RAM")
		return do_suspend2ram();
	if (settings->autoInactiveAction == "Standby")
		return do_standby();
	return false;
}

/*!
 * \b SLOT to dimm the display down to the configured level if the signal
 * \ref autodimm::inactivityTimeExpired was recieved. 
 * \param 
 * \return boolean with the result of the operation
 * \retval true 	if successful 
 * \retval false 	else
 */
void kpowersave::do_downDimm() {
	kdDebugFuncIn(trace);

	if (hwinfo->supportBrightness()) {
		if (!AUTODIMM_Timer->isActive()) {
			int dimmToLevel = (int)((float)hwinfo->getMaxBrightnessLevel()*((float)settings->autoDimmTo/100.0));
			
			// check if we really need to dimm down 
			if (dimmToLevel < hwinfo->getCurrentBrightnessLevel()) {
				int steps = hwinfo->getCurrentBrightnessLevel() - dimmToLevel;
				int timePerStep = (1500 / steps);

				autoDimmDown = true;
	
				AUTODIMM_Timer = new QTimer(this);
				connect(AUTODIMM_Timer, SIGNAL(timeout()), this, SLOT(do_dimm()));
				AUTODIMM_Timer->start(timePerStep, false);
			} else {
				kdWarning(debug_area) << "Don't dimm down, current level is already lower than requested Level" << endl;
			}
		} else {
			// wait until the timer is stopped, try later!
			QTimer::singleShot(1500, this, SLOT(do_downDimm()));
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to dimm the display up to the configured level if the signal
 * \ref autodimm::UserIsActiveAgain was recieved. 
 * \param 
 * \return boolean with the result of the operation
 * \retval true 	if successful 
 * \retval false 	else
 */
void kpowersave::do_upDimm() {
	kdDebugFuncIn(trace);
	
	//NOTE we go back to the value of the scheme and not the last on, to reduce trouble with the scheme

	if (hwinfo->supportBrightness()) {	
		if (!AUTODIMM_Timer->isActive()) {
			int dimmToLevel = (int)((float)hwinfo->getMaxBrightnessLevel()*((float)settings->brightnessValue/100.0));
			
			// check if we really need to dimm up
			if (dimmToLevel > hwinfo->getCurrentBrightnessLevel()) {
				int steps = dimmToLevel - hwinfo->getCurrentBrightnessLevel();
				int timePerStep = (750 / steps);

				autoDimmDown = false;
	
				AUTODIMM_Timer = new QTimer(this);
				connect(AUTODIMM_Timer, SIGNAL(timeout()), this, SLOT(do_dimm()));
				AUTODIMM_Timer->start(timePerStep, false);

				// start autodimm again
				setAutoDimm(false);
			} else {
				kdWarning(debug_area) << "Don't dimm up, current level is already above requested Level" << endl;
			}
		} else {
			// wait until the timer is stopped, try later!
			QTimer::singleShot(750, this, SLOT(do_downDimm()));
		}
	}

	kdDebugFuncOut(trace);
}


/*!
 * \b SLOT to dimm the display down
 * \return boolean with the result of the operation
 * \retval true 	if successful 
 * \retval false 	else
 */
void kpowersave::do_dimm() {
	kdDebugFuncIn(trace);

	int current = hwinfo->getCurrentBrightnessLevel();

	if (autoDimmDown) {
		// dimm the display down
		if (current > 0 &&
		    current > ((int)((float)hwinfo->getMaxBrightnessLevel()*((float)settings->autoDimmTo/100.0))-1)) {
			hwinfo->setBrightness((current -1) , -1);
		} else {
			AUTODIMM_Timer->stop();

			// start checking if the user get active again
			// NOTE: we start this here because the X-Server detect brightness changes as
			// 	 User activity --> FUCKING STUPID PIECE OF SHIT
			autoDimm->startCheckForActivity();
		}
	} else {
		// dimm the display up
		if (current < ((int)((float)hwinfo->getMaxBrightnessLevel()*((float)settings->brightnessValue/100.0))-1)) {
			hwinfo->setBrightness((current +1) , -1);
		} else {
			AUTODIMM_Timer->stop();
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * Function handle umount/remount external storage media before/after 
 * suspend.
 * \param suspend 	boolean with info if the machine go into suspend or not
 * \return 		result of the operation
 * \retval true		if all was successful
 * \retval false	if not
 */
bool kpowersave::handleMounts( bool suspend ) {
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "called suspend: " << suspend << endl;

	bool _ret = false;
	QString _errormsg;

	if (settings->unmountExternalOnSuspend) {
		QString _method;
		DCOPRef dcop_ref = DCOPRef( "kded", "mediamanager" );
		
		if (suspend) {
			_method = "unmountAllSuspend()";
		} else {
			_method = "remountAllResume()";
		}
		DCOPReply reply = dcop_ref.call(_method.latin1());
		if ( reply.isValid() ) {
			reply.get(_errormsg);
			if (_errormsg.isEmpty()) {
				kdDebugFuncOut(trace);
				return true;
			} else {
				kdError(debug_area) << "ERROR while umount/remount partitions: " << _errormsg << endl;
			}
		} else {
			kdWarning(debug_area) << "Could not umount external storage partitions." << endl;
		}

	} else {
		kdDebugFuncOut(trace);
		return true;
	}
	
	// this is only needed for suspend case and an error ... 
	// on resume a simple error msg should be enough
	if (!_ret && suspend) {
		// handle error case
		QString _msg;
		QString _e_msg;
		QString _suspend;

		if (!_errormsg.isEmpty()) {
			_e_msg = _errormsg;
		} else {
			_e_msg = i18n("Could not call DCOP interface to umount external media.");
		}

		// ugly: need qt-tags because mediamanager can return html formated strings !!!
		_msg = 	"<qt>" + 
			i18n("Could not umount external media before suspend/standby. \n "
			    "(Reason: %1)\n \n Would you like to continue suspend/standby "
			    "anyway? \n(Warning: Continue suspend can cause data loss!)").arg(_e_msg) +
			"</qt>";

		_suspend = getSuspendString(calledSuspend);

		int answer = KMessageBox::questionYesNo( 0, _msg, 
							 i18n("Error while prepare %1").arg(_suspend),
							 i18n("Suspend anyway"), i18n("Cancel suspend"), 
							 "ignoreMountOnSuspend");

		if (answer == KMessageBox::Yes) {
			_ret = true;
		}
	}

	kdDebugFuncOut(trace);
	return _ret;
}

/*!
 * Handle the event for the power button and call the related action.
 */
void kpowersave::handlePowerButtonEvent( ) {
	kdDebugFuncIn(trace);

	/* Only go to suspend on button event if we already resumed successful.
	   This should solve problems if we get may a event for the powerbutton
	   if there machine was waked up via power button. */
	if (calledSuspend == -1) {
		handleActionCall(settings->powerButtonAction, settings->powerButtonActionValue);
	}
	
	kdDebugFuncOut(trace);
}

/*!
 * Handle the event for the suspend2ram/sleep button and call the related action.
 */
void kpowersave::handleSleepButtonEvent() {
	kdDebugFuncIn(trace);

	// Only go to suspend on button event if we already resumed successful.
	if (calledSuspend == -1) {
		handleActionCall(settings->sleepButtonAction, -1);
	}

	kdDebugFuncOut(trace);
}

/*!
 * Handle the event for the suspend2disk (hibernater) button and call the related action.
 */
void kpowersave::handleS2DiskButtonEvent(){
	kdDebugFuncIn(trace);

	// Only go to suspend on button event if we already resumed successful.
	if (calledSuspend == -1) {
		handleActionCall(settings->s2diskButtonAction, -1);
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to handle the lidclose event. If the screen get locked
 * depends on the user specific settings.
 * \param closed 	boolean with info if the lid is closed or not
 */
void kpowersave::handleLidEvent( bool closed ){
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "Lid closed? " << closed << endl;
	
	if (closed) {
		// get new general settings! This could maybe removed if we 
		// could be shure, that the settings are actuall
		settings->load_general_settings();
		
		// handle screen lock
		if (settings->lidcloseAction < 0) {
			if(settings->lockOnLidClose) {
				if(!display->lockScreen( settings->lockmethod )) {
					KPassivePopup::message( i18n("WARNING"),
								i18n("Could not lock the screen. There may "
								     "be a problem with the selected \nlock " 
								     "method or something else."),
								SmallIcon("messagebox_warning", 20), this,
								i18n("Warning"), 10000);
				
				}
			}
			if(settings->forceDpmsOffOnLidClose) {
				display->forceDPMSOff();
			}
		} else {
			// handle lock action
			if (hwinfo->currentSessionIsActive()) {
				handleActionCall(settings->lidcloseAction, settings->lidcloseActionValue);
			} else {
				kdWarning(debug_area) << "Session is not active, don't react on lidclose "
					    << "event with a action call (like e.g. Suspend)!" << endl;
			}
		}

		if(!settings->disableNotifications)
			KNotifyClient::event( this->winId(), "lid_closed_event", i18n("The Lid was closed."));
	} else {
		if(settings->forceDpmsOffOnLidClose) {
			// reset the scheme settings to avoid problems related to call xset on lidclose
			setSchemeSettings();
		} 

		if (settings->lockOnLidClose) {
			activateLoginScreen();
		}
	
		if(!settings->disableNotifications)
			KNotifyClient::event( this->winId(), "lid_opened_event", i18n("The Lid was opened."));
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to show the login dialog if the desktop was locked before the suspend.
 */
void kpowersave::activateLoginScreen(){
	kdDebugFuncIn(trace);

        // get new general settings! This could maybe removed if we 
	// could be shure, that the settings are actuall
	settings->load_general_settings();
	
	if(settings->timeToFakeKeyAfterLock >= 0) {
		QTimer::singleShot(settings->timeToFakeKeyAfterLock, display, SLOT(fakeShiftKeyEvent()));
	}
	
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to set the current suspend type for later use.
 */
void kpowersave::setSuspendType( QString suspendtype){
	kdDebugFuncIn(trace);

	suspendType = suspendtype;

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT which called if kpowersave is exited by the user. In this case the user
 * is asked through a yes/no box if "KPowersave start automatically on log in" and the 
 * result is written to the KDE configfile.
 */
void kpowersave::_quit (){
	kdDebugFuncIn(trace);
	
	// set the KDE-Settings back to user default
	if(getenv("KDE_FULL_SESSION")) {
		// first try to call the KDE method via DCOP to reset, if not fall back
		if (!display->resetKDEScreensaver()) {
			settings->load_kde();
			// reset to KDE screensaver settings
			display->blankOnlyScreen(false);
			if(!settings->kde->enabled) display->setScreenSaver(false);
			else display->setScreenSaver(true);
			
			if(!settings->kde->displayEnergySaving) display->setDPMS(false);
			else display->setDPMS(true);
			
			display->has_DPMS = display->setDPMSTimeouts( settings->kde->displayStandby,
								      settings->kde->displaySuspend,
								      settings->kde->displayPowerOff);
		}
	}
	
	// set, if this is a GNOME session, XScreensaver settings back to user default
	QString session = getenv("DESKTOP_SESSION");
	if(session.startsWith("gnome")) {
		display->resetXScreensaver();
	}
	
	if(!settings->autostartNeverAsk) { 
		QString tmp1 = i18n ("Start KPowersave automatically when you log in?");
		int tmp2 = KMessageBox::questionYesNo ( 0, tmp1, i18n("Question"), 
							i18n("Start Automatically"), i18n("Do Not Start"));
		config->setGroup("General");
		config->writeEntry ("Autostart", tmp2 == KMessageBox::Yes);
		config->sync ();
	}
	
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT called if the user select a 'CPU Frequency Policy' from the menu ( \ref CPUFREQ_MENU_ID ). 
 */
void kpowersave::do_setSpeedPolicy(int menu_id){
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "menu_id/set policy to: " << menu_id << endl;

	if(!hwinfo->setCPUFreq((cpufreq_type)menu_id, settings->cpuFreqDynamicPerformance)) {
		KPassivePopup::message(i18n("WARNING"),
				 i18n("CPU Freq Policy %1 could not be set.").arg(speed_menu->text(menu_id)),
				 SmallIcon("messagebox_warning", 20), this, i18n("Warning"), 10000);
	} else {
		hwinfo->checkCurrentCPUFreqPolicy();
		update();
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT called if the user select a scheme from the menu. If there is any errormessage
 * while try to set the selected scheme, the user get a messagebox with info.
 */
void kpowersave::do_setActiveScheme( int i ){
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "set scheme to: " << i << endl;

	if(settings->schemes[i] && (settings->schemes[i] != settings->currentScheme)) {
		for (int x = 0; x < (int) scheme_menu->count(); x++){
			if (x == i)
				scheme_menu->setItemChecked(x, true);
			else
				scheme_menu->setItemChecked(x, false);
		}
		settings->load_scheme_settings( settings->schemes[i]);
		setSchemeSettings();
		notifySchemeSwitch();
 	} else if (!settings->schemes[i]){
		KPassivePopup::message( i18n("WARNING"),
					i18n("Scheme %1 could not be activated.").arg(scheme_menu->text(i)),
					SmallIcon("messagebox_warning", 20), this, i18n("Warning"), 5000);
	} 

	kdDebugFuncOut(trace);
}

/*!
 * This function is invoked if something has to be updated. This including also menu entries.
 * If the battery is in warning state (and powersave set pdaemon->send_battery_state_change_message) 
 * the function pop-up a messagebox.
 */
void kpowersave::update(){
	kdDebugFuncIn(trace);
	
	int redraw_pixmap = 0;
	QString justMins;
	
	/* need to redraw pixmap in toolbar */
	if (hwinfo->update_info_ac_changed){
		redraw_pixmap = 1;
	}
	if (!hwinfo->isOnline()){
		this->contextMenu()->setItemVisible(SUSPEND2DISK_MENU_ID, false);
		this->contextMenu()->setItemVisible(SUSPEND2RAM_MENU_ID, false);
		this->contextMenu()->setItemVisible(STANDBY_MENU_ID, false);
		this->contextMenu()->setItemVisible(SLEEP_SEPARATOR_MENU_ID, false);
		this->contextMenu()->setItemVisible(SCHEME_SEPARATOR_MENU_ID, false);
		this->contextMenu()->setItemVisible(SCHEME_MENU_ID, false);
		this->contextMenu()->setItemVisible(HELP_SEPARATOR_MENU_ID, false);
		this->contextMenu()->setItemVisible(CONFIGURE_ID, false);
		this->contextMenu()->setItemVisible(CONFIGURE_EVENTS_ID, false);
		if (!pixmap_name.startsWith("ERROR")) {
			// dirty !!! but this work for the moment
			hwinfo->update_info_cpufreq_policy_changed = true;
			suspend = hwinfo->getSuspendSupport();
			redraw_pixmap = 1;
		}
	}
	else{
		if (pixmap_name.startsWith("ERROR")) {
			redraw_pixmap = 1;
			hwinfo->update_info_cpufreq_policy_changed = true;
			suspend = hwinfo->getSuspendSupport();
		}
		this->contextMenu()->setItemVisible(SUSPEND2DISK_MENU_ID, true);
		this->contextMenu()->setItemVisible(SUSPEND2RAM_MENU_ID, true);
		this->contextMenu()->setItemVisible(STANDBY_MENU_ID, true);
		this->contextMenu()->setItemVisible(SLEEP_SEPARATOR_MENU_ID, true);
		this->contextMenu()->setItemVisible(SCHEME_SEPARATOR_MENU_ID, true);
		this->contextMenu()->setItemVisible(SCHEME_MENU_ID, true);
		this->contextMenu()->setItemVisible(HELP_SEPARATOR_MENU_ID, true);
		this->contextMenu()->setItemVisible(CONFIGURE_ID, true);
		this->contextMenu()->setItemVisible(CONFIGURE_EVENTS_ID, true);
		
		if (suspend.suspend2disk && (suspend.suspend2disk_allowed || 
		    suspend.suspend2disk_allowed == -1)) {
			this->contextMenu()->setItemEnabled(SUSPEND2DISK_MENU_ID, true);
		} else {
			if (!suspend.suspend2disk)
				this->contextMenu()->setItemVisible(SUSPEND2DISK_MENU_ID, false);
			else
				this->contextMenu()->setItemEnabled(SUSPEND2DISK_MENU_ID, false);
		}

		if (suspend.suspend2ram && (suspend.suspend2ram_allowed || 
		    suspend.suspend2ram_allowed == -1)) {
			this->contextMenu()->setItemEnabled(SUSPEND2RAM_MENU_ID, true);
		} else {
			if (!suspend.suspend2ram)
				this->contextMenu()->setItemVisible(SUSPEND2RAM_MENU_ID, false);
			else
				this->contextMenu()->setItemEnabled(SUSPEND2RAM_MENU_ID, false);
		}

		if (suspend.standby && (suspend.standby_allowed || suspend.standby_allowed == -1)) {
			this->contextMenu()->setItemEnabled(STANDBY_MENU_ID, true);
		} else {
			if (!suspend.standby)	
				this->contextMenu()->setItemVisible(STANDBY_MENU_ID, false);
			else
				this->contextMenu()->setItemEnabled(STANDBY_MENU_ID, false);
		}
	}

	if (hwinfo->update_info_cpufreq_policy_changed == true){
		updateCPUFreqMenu();
	}

	BatteryCollection *primary = hwinfo->getPrimaryBatteries();

	if (hwinfo->update_info_primBattery_changed == true){
		justMins.setNum(primary->getRemainingMinutes() % 60);
		justMins = justMins.rightJustify(2, '0');
		
		redraw_pixmap = 1;
		hwinfo->update_info_primBattery_changed = false;
	}
	
	updateSchemeMenu();

	if (redraw_pixmap){
		redrawPixmap();
	}

	kdDebugFuncOut(trace);
}

/*!
 * This function is involved if the CPUFreqMenu must be updated.
 */
void kpowersave::updateCPUFreqMenu(){
	kdDebugFuncIn(trace);

	if (hwinfo->supportCPUFreq() && hwinfo->isOnline() && hwinfo->isCpuFreqAllowed()) {
		/* set CPU frequency menu entries *********/
		/* speed menu has id 3 in context menu */
		contextMenu()->setItemVisible(CPUFREQ_MENU_ID, true);
		contextMenu()->setItemEnabled(CPUFREQ_MENU_ID, true);
		contextMenu()->setItemVisible(CPUFREQ_SEPARATOR_MENU_ID, true);
	
		switch (hwinfo->getCurrentCPUFreqPolicy()){
			case PERFORMANCE:
				speed_menu->setItemChecked(PERFORMANCE, true);
				speed_menu->setItemChecked(DYNAMIC, false);
				speed_menu->setItemChecked(POWERSAVE, false);
				break;
			case DYNAMIC:
				speed_menu->setItemChecked(PERFORMANCE, false);
				speed_menu->setItemChecked(DYNAMIC, true);
				speed_menu->setItemChecked(POWERSAVE, false);
				break;
			case POWERSAVE:
				speed_menu->setItemChecked(PERFORMANCE, false);
				speed_menu->setItemChecked(DYNAMIC, false);
				speed_menu->setItemChecked(POWERSAVE, true);
				break;
		}
	} else {
		/* there never were policies */
		if (!speed_menu) {
			return ;
		} else if (hwinfo->supportCPUFreq() && (hwinfo->isCpuFreqAllowed() != 1)) {
			contextMenu()->setItemEnabled(CPUFREQ_MENU_ID, false);
			contextMenu()->setItemVisible(CPUFREQ_SEPARATOR_MENU_ID, true);
		} else{
			/* there were CPU freq policies, but they are not accessible any more */
			/* delete speed_menu */
			contextMenu()->setItemVisible(CPUFREQ_MENU_ID, false);
                        contextMenu()->setItemVisible(CPUFREQ_SEPARATOR_MENU_ID, false);
		}
	}
	
	hwinfo->update_info_cpufreq_policy_changed = false;

	kdDebugFuncOut(trace);
}

/*!
 * The function used to update the scheme menu. A update is needed if 
 * if there is maybe new schemes or if the current scheme changed or switched
 * By this way also set the settings for screensaver and other parameter 
 * related to the selected scheme.
 */
void kpowersave::updateSchemeMenu(){
	kdDebugFuncIn(trace);

	if (settings->schemes.count() == 0 || !hwinfo->isOnline()){
		/* there never were schemes */
		if (!scheme_menu)
			return ;
		else{
			/* there were schemes, but they are not accessible any more */
			/* delete scheme_menu */
			scheme_menu->clear();
			contextMenu()->setItemVisible(SCHEME_MENU_ID, false);
			contextMenu()->setItemVisible(SCHEME_SEPARATOR_MENU_ID, false);
			return ;
		}
	}

	/* redraw all scheme entries ... */
	scheme_menu->clear();
	// clear the list of real scheme names
	org_schemenames.clear();

	org_schemenames = settings->schemes;

	int x = 0;
	for ( QStringList::iterator it = org_schemenames.begin(); it != org_schemenames.end(); ++it ) {
		
		QString _t = *it;

		if ( *it == settings->ac_scheme ){
			scheme_menu->insertItem( SmallIcon("scheme_power", QIconSet::Automatic),
						 i18n( (QString)*it ), x, x);
		}
		else{
			if ( *it == settings->battery_scheme ){
				scheme_menu->insertItem(SmallIcon("scheme_powersave", QIconSet::Automatic),
							i18n( *it ), x, x);
			}
			else{
				if ((QString)*it == "Acoustic"){ 
					scheme_menu->insertItem(SmallIcon("scheme_acoustic",
								QIconSet::Automatic), 
								i18n("Acoustic"), x, x);
				}
				else if ((QString)*it == "Presentation"){
					scheme_menu->insertItem(SmallIcon("scheme_presentation",
								QIconSet::Automatic), 
								i18n("Presentation"), x, x);
					
				}
				else if((QString)*it == "AdvancedPowersave") {
					scheme_menu->insertItem(SmallIcon("scheme_advanced_powersave",
								QIconSet::Automatic),
								i18n( "Advanced Powersave" ), x, x);
				}
				else {
					scheme_menu->insertItem(i18n( *it ), x, x);
				}
			}
		}

		if ( *it == settings->currentScheme ) {
			scheme_menu->setItemChecked(x, true);
		}
		++x;
	}
	
	if (x == 0 && scheme_menu){
		// this should not happen, scheme_list should have been NULL before
		// now we'd have an empty menu ...
	}
	else{
		contextMenu()->setItemVisible(SCHEME_MENU_ID, true);
		contextMenu()->setItemVisible(SCHEME_SEPARATOR_MENU_ID, true);
	}

	kdDebugFuncOut(trace);
}


/*!
 * Reimplemented eventhandler for mouse enterEvent. This is called if the mouse cursor 
 * enters the widget. In this case if the user move the mouse cursor over the kpowersave
 * trayicon. \n \n
 * We use this event to update the Tooltip with all needed information. The time beetween
 * the event and the automatically popup of the QToolTip should be long enought to collect
 * the needed values and add a updated Tooltip.
 */
void kpowersave::enterEvent( QEvent */*qee*/ ){

	updateTooltip();

}

/*!
 * Event handler for mouse wheel events.  If the system supports changing display
 * brightness and changing brightness is enabled in the current scheme settings,
 * this will raise the brightness by one level for wheel up events and lower the 
 * brightness by one level for wheel down events.
 */
void kpowersave::wheelEvent (QWheelEvent *qwe)
{
	kdDebugFuncIn(trace);

	if (!hwinfo->supportBrightness() && settings->brightness)
		return;

	if (qwe->orientation () == Vertical) {
		if (qwe->delta() > 0) {
			do_brightnessUp(5);
		} else {
			do_brightnessDown(5);
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * Reimplemented eventhandler for mousePressEvents which is involved if the user click 
 * on the icon on the kickerapplet. This was written to guarantee that a click with the 
 * right and the left mousebutton activate the menu. In the future this can also used 
 * to popup maybe a other menu. 
 */
void kpowersave::mousePressEvent(QMouseEvent *qme){
	kdDebugFuncIn(trace);

	KSystemTray::mousePressEvent(qme);
	if (hwinfo->isOnline()) {
		if (qme->button() == RightButton){
			// TODO check if  maybe some rechecks needed 
			this->contextMenu()->exec(QCursor::pos());
		} else if (qme->button() == LeftButton) {
			showDetailedDialog();
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT called if the detaileddialog is closed. With this we prevent open 
 * the dialog twice, use this function to reset the used variables.
 */
void kpowersave::closedetaileddialog() {
	detailedIsShown = false;
}

/*!
 * \b SLOT used to display error messages related to D-Bus in kpowersave. This function 
 * block all messeges which we have in kpowersave!
 * TODO: do something usefull
 */
void kpowersave::showDBusErrorMsg( int type ){
	kdDebugFuncIn(trace);

	static bool displayed = false;

	QString msg;
	QString dlg_name; 

	switch (type) {
		case DBUS_RUNNING:
			update();
			return;
		case DBUS_NOT_RUNNING:
			msg = i18n("The D-Bus daemon is not running.\nStarting it will "
				   "provide full functionality: /etc/init.d/dbus start");
			dlg_name = "dbusNotRunning";
			break;
		default:
			kdDebugFuncOut(trace);
			return;
	} 

	if (!displayed && !dlg_name.isEmpty()) {
		infoDialog *dlg = new infoDialog( config, i18n("Warning"), msg, 
					 i18n("Don't show this message again."), dlg_name);

		if (!dlg->dialogIsDisabled()) {
			dlg->show();
		}
		// set this always to true ... this should reduce the calls
		displayed = true;
	}

	kdDebugFuncOut(trace);
}

/*! 
 * \b SLOT to display the HAL error message. We use this 
 * function to delay the message, and prevent display them is HAL or
 * powersaved restarted.
 */
void kpowersave::showHalErrorMsg() {
	kdDebugFuncIn(trace);

	if (hwinfo->isOnline()) {
		// HAL is back!
		update();
	}
	if (!hwinfo->dbus_terminated) {
		hal_error_shown = false;
		DISPLAY_HAL_ERROR_Timer->stop();
	}

	kdDebugFuncOut(trace);
	return;
}

/*!
 * \b SLOT used to display messeges in kpowersave. This function
 * block all messeges which we have in kpowersave!
 */
void kpowersave::showErrorMessage( QString msg ){
	kdDebugFuncIn(trace);

	if(settings->psMsgAsPassivePopup) {
		KPassivePopup::message("KPowersave", msg, SmallIcon("messagebox_warning", 20),
				       this, i18n("Warning"), 10000);
	} else {
		kapp->updateUserTimestamp();
		// KMessageBox::error( 0, msg);
		KMessageBox::queuedMessageBox(0, KMessageBox::Error, msg);
        }

	kdDebugFuncOut(trace);
}


/*!
 * Use this function to set the SchemeSettings. This function currently set the 
 * e.g. the screensaver and dpms settings. Don't forget to call this function if
 * a scheme is changed or if the settings changed.
 */
void kpowersave::setSchemeSettings(){
	kdDebugFuncIn(trace);

	// --> check if there is a scheme set, if not, use defaults
	if ( settings->currentScheme.isEmpty()) {
		if (hwinfo->getAcAdapter()) {
			settings->load_scheme_settings( settings->ac_scheme);
		} else {
			settings->load_scheme_settings( settings->battery_scheme);
		}
	}

	// call setPowerSave() depending on AC state
	if (settings->callSetPowerSaveOnAC) {
		if (hwinfo->getAcAdapter())
			hwinfo->setPowerSave(false);
		else
			hwinfo->setPowerSave(true);
	}
	
	// --> set autosuspend settings
	if(settings->autoSuspend) {
		setAutoSuspend(false);
	} else {
		this->contextMenu()->setItemVisible(AUTOSUSPEND_MENU_ID, false);
		this->contextMenu()->setItemChecked(AUTOSUSPEND_MENU_ID, false);
		this->contextMenu()->setItemVisible(AUTOSUSPEND_SEPARATOR_MENU_ID, false);
		autoSuspend->stop();
	}

	// --> set autodimm settings
	if (settings->autoDimm) {
		setAutoDimm(true);
	} else {
		autoDimm->stop();
	}

	// --> set screensaver
	if(settings->specSsSettings){
		if(settings->disableSs) display->setScreenSaver(false);
		else {
			display->setScreenSaver(true);
			if(settings->blankSs) display->blankOnlyScreen(true);
			else {
				display->blankOnlyScreen(false);
			}
		}
		
	} // TODO: check if this really work !!!
	else if(getenv("KDE_FULL_SESSION")) {
		// try to reset the complete screensaver settings. Ff this fail, use own methodes
		if (!display->resetKDEScreensaver()) {
			settings->load_kde();
			// Always disable blankOnly screensaver setting (default). KDE does
			// not provide a GUI to configure it and most likely we are the
			// only ones messing with it
			display->blankOnlyScreen(false);

			// reset to global screensaver settings
			if(!settings->kde->enabled) display->setScreenSaver(false);
			else {
				display->setScreenSaver(true);
				// What should we do with settings->kde->lock ?
				// Maybe nothing ?!
			}	
		}
	} else if ((getenv("DESKTOP_SESSION") != NULL) && !strcmp(getenv("DESKTOP_SESSION"), "gnome")) {
		// use this to set XScreensaver back to default settings this should
		// also cover the DPMS settings for GNOME/XScreensaver
		display->resetXScreensaver();
	}
	
	// --> set DPMS settings
	if(settings->specPMSettings){
		// set the new DPMS settings
		if(settings->disableDPMS) {
			display->setDPMS(false);
		}
		else {
			display->setDPMS(true);
			display->has_DPMS = display->setDPMSTimeouts( settings->standbyAfter,
								      settings->suspendAfter,
								      settings->powerOffAfter);
		}
	}
	else  if(getenv("KDE_FULL_SESSION")){
		// try to reset the KDE screensaver/DPMS settings (if there are also 
		// no special screensaver settings) otherwise fall back and set values from files
		if (!settings->specSsSettings && !display->resetKDEScreensaver()) {
			settings->load_kde();
			// reset to global screensaver settings
			if(!settings->kde->displayEnergySaving) {
				display->setDPMS(false);
			}
			else {
				display->setDPMS(true);
				display->has_DPMS = display->setDPMSTimeouts( settings->kde->displayStandby,
									      settings->kde->displaySuspend,
									      settings->kde->displayPowerOff);
			}
		}
	}
	
	// --> set brightness settings
	if(settings->brightness && hwinfo->supportBrightness()) {
		// set to given values
		hwinfo->setBrightness (-1, settings->brightnessValue);
	}

	// --> set CPU Freq settings
	if(hwinfo->supportCPUFreq()) {
		hwinfo->setCPUFreq( settings->cpuFreqPolicy, settings->cpuFreqDynamicPerformance);
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT which called to set and start the autosuspend monitoring.
 * \param resumed boolean value which represent information if machine 
 *		  currently back from suspend/standby
 */
void kpowersave::setAutoSuspend( bool resumed ){
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "resumed? " << resumed << endl;

	if(settings->autoInactiveActionAfter > 0 && settings->autoSuspend) {
		int autoInactiveActionAfter = 0;

		if( settings->autoInactiveAction.startsWith("_NONE_")) {
			autoSuspend->stop();
			return;
		}
		if (resumed) {
			autoSuspend->stop();
			delete autoSuspend;
			autoSuspend = new autosuspend();
			connect(autoSuspend, SIGNAL(inactivityTimeExpired()), this, 
				SLOT(do_autosuspendWarn()));
		}
		
		if (settings->autoSuspendCountdown && (settings->autoSuspendCountdownTimeout > 0)) {
			autoInactiveActionAfter = ((settings->autoInactiveActionAfter * 60) -
						    settings->autoSuspendCountdownTimeout);
		} else {
			autoInactiveActionAfter = settings->autoInactiveActionAfter * 60;
		}

		if(settings->autoInactiveSBlistEnabled) {
			autoSuspend->start( autoInactiveActionAfter, settings->autoInactiveSBlist );
		}
		else {
			autoSuspend->start( autoInactiveActionAfter, settings->autoInactiveGBlist );
		}
		this->contextMenu()->setItemVisible(AUTOSUSPEND_SEPARATOR_MENU_ID, true);
		this->contextMenu()->setItemVisible(AUTOSUSPEND_MENU_ID, true);
	}
	else {
		// if autosuspend is not NULL: stop autosuspend
		if (autoSuspend) {
			autoSuspend->stop();
		}

		this->contextMenu()->setItemVisible(AUTOSUSPEND_MENU_ID, false);
		this->contextMenu()->setItemVisible(AUTOSUSPEND_SEPARATOR_MENU_ID, false);
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT which called to set and start the autodimm monitoring.
 * \param resumed boolean value which represent information if machine 
 *		  currently back from suspend/standby
 */
void kpowersave::setAutoDimm( bool resumed ){
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "resumed? " << resumed << endl;

	if(settings->autoDimmAfter > 0 && settings->autoDimm) { 
		if(settings->autoDimmTo < 0) {
			autoDimm->stop();
			kdWarning(debug_area) << "Not allowed or set level for dimm" << endl;
		} else {
			if (resumed) {
				// setup again
				autoDimm->stop();
				delete autoDimm;
				autoDimm = new autodimm();
				connect(autoDimm, SIGNAL(inactivityTimeExpired()), this, SLOT(do_downDimm()));
				connect(autoDimm, SIGNAL(UserIsActiveAgain()), this, SLOT(do_upDimm()));
			}

			if (settings->autoDimmSBlistEnabled) {
				autoDimm->start(settings->autoDimmAfter * 60, settings->autoDimmSBlist);
			} else {
				autoDimm->start(settings->autoDimmAfter * 60, settings->autoDimmGBlist);
			}
		}
	} else {
		if (autoDimm)
			autoDimm->stop();
	}	

	kdDebugFuncOut(trace);
}

// -------- start KNotify functions ------------- //

/*!
 * \b SLOT called if a battery warning state reached and related signal recieved. 
 * Here we emit the related KNotify event, if not disabled.
 * \param type  integer with  the type of the battery
 * \param state integer represent the reached battery state
 */
void kpowersave::notifyBatteryStatusChange ( int type, int state ) {
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "type: " << type << "state: " << state << endl;


	if (type == BAT_PRIMARY) {
		BatteryCollection *primary = hwinfo->getPrimaryBatteries();
		int min = primary->getRemainingMinutes();

		if (primary->getChargingState() == CHARGING) {
			kdDebug(debug_area) << "kpowersave::notifyBatteryStatusChange: Battery is charging, ignore event" << endl;
			return;
		}
		if (hwinfo->getAcAdapter()) {
			// the machine is on AC, no need to inform about battery state,
			// this is maybe only a race condition with not directly actuall
			// charge state
			kdDebug(debug_area) << "kpowersave::notifyBatteryStatusChange: Machine is on AC, ignore event" << endl;
			kdDebugFuncOut(trace);
			return;
		}

		switch (state) {
		case BAT_WARN:
			if (!settings->disableNotifications)
				KNotifyClient::event(this->winId(), "battery_warning_event",
						     i18n("Battery state changed to WARNING -- remaining time: "
						     "%1 hours and %2 minutes.").arg(min/60).arg(min%60));
			// set/call related actions
			handleActionCall(settings->batteryWarningLevelAction,
					 settings->batteryWarningLevelActionValue);
			break;
		case BAT_LOW:
			if (!settings->disableNotifications)
				KNotifyClient::event(this->winId(), "battery_low_event",
				i18n("Battery state changed to LOW -- remaining time: "
						     "%1 hours and %2 minutes.").arg(min/60).arg(min%60));
			// set/call related actions
			handleActionCall(settings->batteryLowLevelAction,
					 settings->batteryLowLevelActionValue);
			break;
		case BAT_CRIT:
			// handle carefully:
			if (settings->batteryCriticalLevelAction == GO_SHUTDOWN) {
				if (!settings->disableNotifications)
					KNotifyClient::event(this->winId(), "battery_critical_event",
							     i18n("Battery state changed to CRITICAL -- "
								  "remaining time: %1 hours and %2 minutes.\n"
								  "Shut down your system or plug in the power "
								  "cable immediately. Otherwise the machine\n"
								  "will go shutdown in 30 seconds")
								.arg(min/ 60).arg(min%60));
				
				QTimer::singleShot(30000, this, SLOT(handleCriticalBatteryActionCall()));
			} else {
				if (!settings->disableNotifications)
					KNotifyClient::event(this->winId(), "battery_critical_event",
							     i18n("Battery state changed to CRITICAL -- "
								  "remaining time: %1 hours and %2 minutes.\n"
								  "Shut down your system or plug in the power "
								  "cable immediately.")
								.arg(min/ 60).arg(min%60));

				handleActionCall(settings->batteryCriticalLevelAction,
						 settings->batteryCriticalLevelActionValue);
			}
			break;
		default:
			break;
		} 
	} else {
		// TODO: add some code later for the other batteries
	}

	kdDebugFuncOut(trace);
}


/*!
 * Function to call the action for battery critical event. This is ugly, but
 * because of QTimer::singleShot() can't take param ... 
 * NOTE: Use this only for SHUTDOWN atm
 */
void kpowersave::handleCriticalBatteryActionCall () {
	kdDebugFuncIn(trace);

	handleActionCall(GO_SHUTDOWN, settings->batteryCriticalLevelActionValue, true, true);

	kdDebugFuncOut(trace);
}

/*!
 * Function to set a special action for a battery warning level.
 * \param action 	integer with the type of the action from \ref action
 * \param value 	integer value of the action as e.g. a brightness level
 * \param checkAC	bool if there should be a check for AC state befor call the action
 */
void kpowersave::handleActionCall ( action action, int value , bool checkAC, bool batWarnCall ) {
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "action: " << action << "value: " << value 
						   << "checkAC: " << checkAC << endl;

	if (hwinfo->currentSessionIsActive()) {
		switch (action) {
			case GO_SHUTDOWN:
				// to be shure if we really need the shutdown
				if ((checkAC && !hwinfo->getAcAdapter()) || !checkAC ) {
					DCOPRef shutdown = DCOPRef( "ksmserver", "ksmserver" );
					shutdown.send("logout", 0, 2, 2);
				}
				break;
			case LOGOUT_DIALOG:
				{
					DCOPRef shutdown = DCOPRef( "ksmserver", "ksmserver" );
					shutdown.send("logout", 1, 2, 2);
				}
				break;
			case GO_SUSPEND2RAM:
				QTimer::singleShot(100, this, SLOT(do_suspend2ram()));
				break;
			case GO_SUSPEND2DISK:
				QTimer::singleShot(100, this, SLOT(do_suspend2disk()));
				break;
			case BRIGHTNESS:
				hwinfo->setBrightness( -1, value );
				break;
			case CPUFREQ_POWERSAVE:
				hwinfo->setCPUFreq( POWERSAVE );
				break;
			case CPUFREQ_DYNAMIC:
				hwinfo->setCPUFreq( DYNAMIC, settings->cpuFreqDynamicPerformance );
				break;
			case CPUFREQ_PERFORMANCE:
				hwinfo->setCPUFreq( PERFORMANCE );
				break;
			case SWITCH_SCHEME: // not supported atm
			case UNKNOWN_ACTION:
			case NONE:
			default:
				kdError(debug_area) << "Could not set the requested Action: " << action << endl;
				break;
		}
	} else if (batWarnCall) {
		if (!hwinfo->isPolicyPowerIfaceOwned()) {
			switch (action) {
				case GO_SHUTDOWN:
					// to be shure if we really need the shutdown
					if ((checkAC && !hwinfo->getAcAdapter()) || !checkAC ) {
						DCOPRef shutdown = DCOPRef( "ksmserver", "ksmserver" );
						shutdown.send("logout", 0, 2, 2);
					}
					break;
				default:
					kdError(debug_area) << "Could not call requested action, inactive session: " << action << endl;
					break;
			}
		}
	} else {
		kdError(debug_area) << "Could not set the requested action, session is inactiv: " << action << endl;
	}
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT called if ac stated changed. Here we emit the related KNotify event.
 * and switch to the AC/battery scheme depending on the state of AC
 * \param acstate boolean represent the state of AC (true == AC plugged in ...)
 */
void kpowersave::handleACStatusChange ( bool acstate , bool notifyEvent ) {
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "acstate: " << acstate << "notifyEvent: " << notifyEvent << endl;

	int index;

	if (hwinfo->currentSessionIsActive()) {

		// emit notify event
		if(notifyEvent && !settings->disableNotifications) {
			if (acstate) {
				KNotifyClient::event(this->winId(), "plug_event", i18n("AC adapter plugged in"));
			} else {
				KNotifyClient::event(this->winId(), "unplug_event", i18n("AC adapter unplugged"));
			}
		}
	
		// handle switch to AC/battery default scheme
		if (acstate) {
			index = settings->schemes.findIndex(settings->ac_scheme);
		} else {
			index = settings->schemes.findIndex(settings->battery_scheme);
		}
	
		if (index != -1)
			do_setActiveScheme(index);
	
		// update applet
		update();
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT called if scheme switched. Here we emit the related KNotify events
 * if they are not disabled.
 */
void kpowersave::notifySchemeSwitch() {
	kdDebugFuncIn(trace);

	if(!settings->disableNotifications) {
		QString _scheme = settings->currentScheme;
		QString eventType;

		if( _scheme != "Performance" && _scheme != "Powersave" && _scheme != "Acoustic" &&
		    _scheme != "Presentation" && _scheme != "AdvancedPowersave" )
			eventType = "scheme_Unknown";
		else
			eventType = "scheme_" + _scheme;

		KNotifyClient::event( this->winId(), eventType, 
				      i18n("Switched to scheme: %1").arg(i18n(_scheme)));
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT called if the machine suspend. Here we emit the related KNotify events
 * if they are not disabled.
 */
void kpowersave::notifySuspend( int suspendType ) {
	kdDebugFuncIn(trace);

	if(!settings->disableNotifications) {
		switch (suspendType) {
			case SUSPEND2DISK:
				KNotifyClient::event( this->winId(), "suspend2disk_event", 
						      i18n("System is going into %1 now.").
						      arg(i18n("Suspend to Disk")));
				break;
			case SUSPEND2RAM:
				KNotifyClient::event( this->winId(), "suspend2ram_event", 
						      i18n("System is going into %1 now.").
						      arg(i18n("Suspend to RAM")));
				break;
			case STANDBY:
				KNotifyClient::event( this->winId(), "standby_event", 
						      i18n("System is going into %1 now.").
						      arg(i18n("Standby")));
				break;
			default:
				break;
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT called to independent handleResumeSignal() from event loop and
 * to avoid problems with the QT3 D-Bus bindings
 */
void kpowersave::forwardResumeSignal( int result ) {
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "result: " << result << endl;
	
	resume_result = result;

	QTimer::singleShot(100, this, SLOT(handleResumeSignal()));

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT called if the machine suspend. Here we emit the related KNotify events
 * if they are not disabled.
 */
void kpowersave::handleResumeSignal() {
	kdDebugFuncIn(trace);

	// fake key to show the login dialog if we locked the screen
	if(settings->lockOnSuspend) {
		activateLoginScreen();
	}
	
	// reset autosuspend and autodimm
	setAutoSuspend(true);
	setAutoDimm(true);

	// reset the CPU Freq Policy ... for more see https://bugzilla.novell.com/show_bug.cgi?id=223164
	if(hwinfo->supportCPUFreq()) {
		hwinfo->setCPUFreq( settings->cpuFreqPolicy, settings->cpuFreqDynamicPerformance );
	}

	if(!settings->disableNotifications) {
		switch (calledSuspend) {
			case SUSPEND2DISK:
				KNotifyClient::event( this->winId(), "resume_from_suspend2disk_event", 
						      i18n("System is resumed from %1.").arg(
						      i18n("Suspend to Disk")));
				break;
			case SUSPEND2RAM:
				KNotifyClient::event( this->winId(), "resume_from_suspend2ram_event", 
					      	      i18n("System is resumed from %1.").arg(
						      i18n("Suspend to RAM")));
				break;
			case STANDBY:
				KNotifyClient::event( this->winId(), "resume_from_standby_event", 
						      i18n("System is resumed from %1.").arg(
						      i18n("Standby")));
				break;
			default:
				kdError(debug_area) << "called suspend type unknown" << endl;
				break;
		
		}
	}

	// handle result of the resume/suspend
	if (resume_result == 0 || resume_result == INT_MAX) {
		if ( resume_result == INT_MAX )
			kdWarning(debug_area) << "Unknown if we successful resumed, look like a D-Bus timeout since "
				    << "elapsed time between suspend and resume is higher than 6 hours" << endl;

		// successful resumed ... remount only in this case
		if (!handleMounts(false)) {
			KPassivePopup::message( i18n("WARNING"), 
						i18n("Could not remount (all) external storage"
						" media."), SmallIcon("messagebox_warning", 20), 
						this, i18n("Warning"), 15000);
		}
	} else {
		kdError(debug_area) << "Unknown error while suspend. Errorcode: " << resume_result << endl;
		QString msg;

		msg = i18n("An unknown error occurred while %1. The errorcode is: '%2'").
			   arg(getSuspendString(calledSuspend)).arg(resume_result);

#if defined(DISTRO_IS_SUSE) || defined(DISTRO_IS_SLES_SLED) || defined(DISTRO_IS_PARDUS)
		// okay we know this system use pm-utils and log is under /var/log/pm-suspend.log
		msg += "\n" + i18n("Do you want to have a look at the log file?");
		int answer = KMessageBox::questionYesNo(0, msg, i18n("Error while %1").
							arg(getSuspendString(calledSuspend)));
		if (answer == KMessageBox::Yes) {
 #if defined(DISTRO_IS_SLES_SLED)
			switch (calledSuspend) {
				case SUSPEND2DISK:
					logview = new LogViewer ("/var/log/suspend2disk.log");
					logview->show();
					break;
				case SUSPEND2RAM:
					logview = new LogViewer ("/var/log/suspend2ram.log");
					logview->show();
					break;
				case STANDBY:
					logview = new LogViewer ("/var/log/standby.log");
					logview->show();
					break;
				default:
					break;
			} 
 #else
			logview = new LogViewer ("/var/log/pm-suspend.log");
			logview->show();
 #endif
		}
#else
		KMessageBox::error(0, msg, i18n("Error while %1").arg(getSuspendString(calledSuspend)));
#endif
	}
	// set back ... suspend is handled
	calledSuspend = -1;
	resume_result = 0;

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT called if the state of the current session change
 * \param state boolean represent the state of the session
 * TODO: fix scheme handling 
 * TODO: fix critical battery situations (see the todo file in the source)
 */
void kpowersave::handleSessionState (bool state) {
	kdDebugFuncIn(trace);

	if (state) {
		// session is active again
		if (settings->autoSuspend) disableAutosuspend(false);
		if (settings->autoDimm) setAutoDimm(false);
		/* handle may missed/not set AC status changes while the 
		   session was inactive and set them to the default schemes ?! */
		handleACStatusChange(hwinfo->getAcAdapter(), false);
		
	} else {
		// session is now inactive
		if (settings->autoSuspend) disableAutosuspend(true);
		if (settings->autoDimm) autoDimm->stop();
	}	

	kdDebugFuncOut(trace);
}

// -------- end KNotify functions ------------- //
// ------------ helper functions -------------- //

/*!
 * Helper function to get a i18n name for a suspend type.
 * \param type	Integer value with the suspend type
 * \return 	QString with the translated name or NULL if it fail
 */
QString kpowersave::getSuspendString (int type) {
	kdDebugFuncIn(trace);

	switch (type) {
		case SUSPEND2DISK:
			return i18n("Suspend to Disk");
			break;
		case SUSPEND2RAM:
			return i18n("Suspend to RAM");
			break;
		case STANDBY:
			return i18n("Standby");
			break;
		default:
			return QString();
	}

	kdDebugFuncOut(trace);
}

// --------- end helper functions ------------- //
// ------------ DCOP functions ---------------- //

/*!
 * DCOP Interface funtion to lock the screen with the userdefined methode.
 * \return boolean with the result of locking the screen
 * \retval true if locking the screen was successful
 * \retval false if locking the screen failed or the user don't wan't to lock
 */
bool kpowersave::lockScreen(){
	kdDebugFuncIn(trace);
	
	settings->load_general_settings();
	
	return display->lockScreen( settings->lockmethod );

	kdDebugFuncOut(trace);
}

/*!
 * DCOP Interface funtion to return the name of the current powersave scheme.
 * \return QString with the name of the current scheme
 */
QString kpowersave::currentScheme (){
	kdDebugFuncIn(trace);
	
	if(hwinfo->isOnline()) {
		return settings->currentScheme;
	} else {
		return "ERROR: D-Bus and/or HAL not running";
	}

	kdDebugFuncOut(trace);
}

/*!
 * DCOP Interface funtion to return the name of the current cpuFreqPolicy.
 * \return QString with the name of the current cpuFreqPolicy
 */
QString kpowersave::currentCPUFreqPolicy() {
	kdDebugFuncIn(trace);

	if(hwinfo->isOnline()) {
		QString _cpuFreq = "";
		switch (hwinfo->getCurrentCPUFreqPolicy()){
			case PERFORMANCE:
				_cpuFreq = "PERFORMANCE";
				break;
			case DYNAMIC:
				_cpuFreq = "DYNAMIC";
				break;
			case POWERSAVE:
				_cpuFreq = "POWERSAVE";
				break;
			default:
				_cpuFreq = "UNKNOWN";
				break;
		}
		return _cpuFreq;
	} else {
		return "ERROR: HAL or/and DBus not running";
	}
	
	kdDebugFuncOut(trace);
}

/*!
 * DCOP Interface funtion to send a list with the allowed  
 * CPU Frequency states.
 * \return QStringList with the supported CPUFreq states
 */
QStringList kpowersave::listCPUFreqPolicies() {
	kdDebugFuncIn(trace);

	QStringList ret_list;
	if (hwinfo->isCpuFreqAllowed()) {
		ret_list.append("PERFORMANCE");
		ret_list.append("DYNAMIC");
		ret_list.append("POWERSAVE");
	}
	else {
		ret_list.append("NOT SUPPORTED");
	}

	kdDebugFuncOut(trace);
	return ret_list;
}

/*!
 * DCOP Interface funtion to set the current CPUFreq policy
 * \param policy QString with the policy to set, only values from
 * 		 list_CPUFreqPolicies are allowed (except "NOT SUPPORTED")
 * \return boolean with the result of set the requested CPUFreq policy
 * \retval true if successful set 
 * \retval false if not supported or any other failure
 */
bool kpowersave::do_setCPUFreqPolicy( QString policy ) {
	if (trace) kdDebug(debug_area) << funcinfo << "IN: " << "policy: " << policy << endl;

	
	bool ret = true;
	/*
	if (hwinfo->isCpuFreqAllowed() && hwinfo->isOnline()) {
		if (policy == "PERFORMANCE") {
			hwinfo->setCPUFreq(PERFORMANCE);
		} else if (policy == "DYNAMIC") {
			hwinfo->setCPUFreq(DYNAMIC, settings->cpuFreqDynamicPerformance);
		} else if (policy == "POWERSAVE") {
			hwinfo->setCPUFreq(POWERSAVE);
		} else {
			kdDebugFuncOut(trace);
			ret = false;
		}
	} else {
		ret = false;
	}
	*/
	kdDebugFuncOut(trace);
	return ret;
}

/*!
 * DCOP Interface funtion to send a list with the supported and enabled 
 * sleeping states.
 * \return QStringList with the supported spleeping states
 */
QStringList kpowersave::allowed_sleepingStates(){
	kdDebugFuncIn(trace);
	
	QStringList sleepList;	
	if(hwinfo->isOnline()) {
		if (suspend.suspend2disk && (suspend.suspend2disk_allowed || 
		    suspend.suspend2disk_allowed == -1)){ 
			sleepList.append("suspendToDisk");
		}
		if (suspend.suspend2ram && (suspend.suspend2ram_allowed || 
		    suspend.suspend2ram_allowed == -1)){ 
			sleepList.append("suspendToRAM");
		}
		if (suspend.standby && (suspend.standby_allowed || suspend.standby_allowed == -1)){ 
			sleepList.append("standBy");
		}	
		if(sleepList.isEmpty()){
			sleepList.append("NO_SLEEPING_STATES_SUPPORTED");
		}
	}
	else {
		sleepList.append("ERROR: D-Bus and/or HAL not running");
	}
	
	kdDebugFuncOut(trace);
	return sleepList;
}

/*!
 * DCOP Interface funtion to send a list with the all schemes.
 * \return QStringList with all schemes
 */
QStringList kpowersave::listSchemes(){
	kdDebugFuncIn(trace);

	QStringList _schemeList;	
	if(hwinfo->isOnline()) {
		if (settings->schemes.count() > 0){
			_schemeList = settings->schemes;
		}
	}
	else {
		_schemeList.append("ERROR: D-Bus and/or HAL not running");
	}

	kdDebugFuncOut(trace);
	return _schemeList;
}


/*!
 * DCOP Interface funtion to set the current scheme.
 * \return boolean with the result of set the requested scheme
 * \retval false if failed (e.g. scheme is not in the list)
 * \retval true if scheme found and set
 * \param _scheme QString with the scheme to set, scheme should be 
 *		  named as list from list_schemes()
 */
bool kpowersave::do_setScheme( QString /*_scheme*/ ) {
	kdDebugFuncIn(trace);

/*	int index;
	index = settings->schemes.findIndex(_scheme);

	if (index != -1) {
		do_setActiveScheme(index);
		kdDebugFuncOut(trace);
		return true;
	}
	else {
		kdDebugFuncOut(trace);
		return false;
	}
*/
	kdDebugFuncOut(trace);
	return false;
}

/*!
 * DCOP Interface funtion to send the suspend to disk command to powersave.
 * \return boolean with the result of calling do_suspend2disk()
 * \retval true if successful
 * \retval false if not supported or powersaved not running
 */
bool kpowersave::do_suspendToDisk(){
	kdDebugFuncIn(trace);
	kdDebugFuncOut(trace);
	return do_suspend2disk();
}

/*!
 * DCOP Interface funtion to send the suspend to disk command to powersave.
 * \return boolean with the result of calling do_suspend2ram()
 * \retval true if successful
 * \retval false if not supported or powersaved not running
 */
bool kpowersave::do_suspendToRAM(){
	kdDebugFuncIn(trace);
	kdDebugFuncOut(trace);
	return do_suspend2ram();
}

/*!
 * DCOP Interface funtion to send the suspend to disk command to powersave.
 * \return boolean with the result of calling do_standby()
 * \retval true if successful
 * \retval false if not supported or powersaved not running
 */
bool kpowersave::do_standBy(){
	kdDebugFuncIn(trace);
	kdDebugFuncOut(trace);
	return do_standby();
}

//! dcop function to set the brightness up
bool kpowersave::do_brightnessUp(int percentageStep) {
	kdDebugFuncIn(trace);
	
	bool retval = false;
	
	if(hwinfo->isOnline()) {
		retval = hwinfo->setBrightnessUp(percentageStep);
	} 
	
	kdDebugFuncOut(trace);
	return retval;
}

//! dcop function to set the brightness down
bool kpowersave::do_brightnessDown(int percentageStep) {
	kdDebugFuncIn(trace);
	
	bool retval = false;
	
	if(hwinfo->isOnline()) {
		retval = hwinfo->setBrightnessDown(percentageStep);
	} 
	
	kdDebugFuncOut(trace);
	return retval;
}


/*!
 * DCOP Interface funtion to stop/start the Autosuspend 
 * \param disable boolean which tell if the autosuspend should be stopped (true)
 *		   or started (false).
 */
void kpowersave::disableAutosuspend( bool disable ){
	kdDebugFuncIn(trace);

	if(settings->autoSuspend && settings->autoInactiveActionAfter > 0) {
		if (disable) {
			if ( !contextMenu()->isItemChecked(AUTOSUSPEND_MENU_ID)) {
				autoSuspend->stop();
				contextMenu()->setItemChecked(AUTOSUSPEND_MENU_ID, true);
			}
		}
		else {
			contextMenu()->setItemChecked(AUTOSUSPEND_MENU_ID, false);
			setAutoSuspend(true);
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * DCOP Interface funtion to open/close the detailed dialog.
 */
void kpowersave::showDetailedDialog( ){
	kdDebugFuncIn(trace);

	if (detailedIsShown) {
		detailedDlg->close();
		delete(detailedDlg);
		closedetaileddialog();
		return;
	}

	detailedDlg = new detaileddialog(hwinfo, &fullIcon, settings);

	if (detailedDlg) {
		detailedDlg->show();
		detailedIsShown = true;
	}
	
	connect(detailedDlg, SIGNAL(destroyed()), this, SLOT(closedetaileddialog()));

	kdDebugFuncOut(trace);
}

/*!
 * DCOP Interface funtion to open the configure dialog.
 * \return boolean with the result of open the dialog
 * \retval false if failed (e.g. D-Bus or HAL is not running)
 * \retval true if correct opend 
 */
bool kpowersave::openConfigureDialog (){
	kdDebugFuncIn(trace);
	
	if(hwinfo->isOnline()) {
		showConfigureDialog();
		kdDebugFuncOut(trace);
		return config_dialog_shown;
	} else {
		kdDebugFuncOut(trace);
		return false;
	}
}

/*!
 * DCOP Interface funtion to find out if the current
 * scheme manages DPMS
 * \return boolean
 * \retval false if current scheme does not overwrite DPMS
 * \retval true if current scheme does
 */
bool kpowersave::currentSchemeManagesDPMS () {
	kdDebugFuncIn(trace);
	kdDebugFuncOut(trace);
	return settings->specPMSettings;
}


//! dcop funtion to get the current brightness level
int kpowersave::brightnessGet() {
	kdDebugFuncIn(trace);

	int retval = -1;

	if (hwinfo->supportBrightness()) {
		retval = (int)(((float)hwinfo->getCurrentBrightnessLevel() / (float)hwinfo->getMaxBrightnessLevel()-1) * 100.0);
	}

	kdDebugFuncOut(trace);
	
	return retval;
}

#include "kpowersave.moc"
