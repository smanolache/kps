/***************************************************************************
 *   Copyright (C) 2005 by Danny Kukawka                                   *
 *                         <dkukawka@suse.de>, <danny.kukawka@web.de>      *
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
 
/*! \file configuredialog.cpp
 * All here displayed file members of configureDialog.cpp are related to operations with the 
 * configure dialog for kpowersave
 * \brief 	In this file can be found all configure dialog related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2005
 */

 // KDE - Headers
 #include <kaudioplayer.h>
 #include <kconfig.h>
 #include <kiconloader.h>
 #include <klocale.h>
 #include <kmessagebox.h>
 #include <kinputdialog.h>
 #include <kaccelmanager.h>

 // QT - Headers
 #include <qcheckbox.h>
 #include <qcombobox.h>
 #include <qdialog.h>
 #include <qgroupbox.h>
 #include <qlabel.h>
 #include <qlineedit.h>
 #include <qlistbox.h>
 #include <qpushbutton.h>
 #include <qslider.h>
 #include <qspinbox.h>
 #include <qstringlist.h>
 #include <qtabwidget.h>
 #include <qtoolbox.h>
 #include <qtooltip.h>
 #include <qstring.h>

 #include "configuredialog.h"

/*! This is the default constructor of the class ConfigureDialog. */
ConfigureDialog::ConfigureDialog( KConfig *_config, HardwareInfo *_hwinfo, Settings *_settings,
				  QWidget *parent, const char *name)
				 :configure_Dialog(parent, name, false, WDestructiveClose )
{
	kdDebugFuncIn(trace);

	kconfig = _config;
	settings = _settings;
	hwinfo = _hwinfo;
	suspend = hwinfo->getSuspendSupport();

	QString session = getenv("DESKTOP_SESSION");
	if(session.startsWith("gnome")) gnome_session = true;
        else gnome_session = false;

	initalised = false;
	general_changed = false;
	scheme_changed = false;
	displayed_WARN_autosuspend = false;

	// check if brightness is supporte
	if(hwinfo->supportBrightness()) {
		brightnessLevels = hwinfo->getMaxBrightnessLevel() -1;
		brightness_last = hwinfo->getCurrentBrightnessLevel();
	} else { 
		brightnessLevels = -1;
		brightness_last = -1;
	}
	brightness_changed = false;

	currentScheme = -1;

	// get the correct available suspend types
	SuspendStates suspend = hwinfo->getSuspendSupport();
	if( suspend.suspend2ram && (suspend.suspend2ram_allowed || suspend.suspend2ram_allowed == -1))
		actions.append("Suspend to RAM");
	if ( suspend.suspend2disk && (suspend.suspend2disk_allowed || suspend.suspend2disk_allowed == -1))
		actions.append("Suspend to Disk");
	if ( suspend.standby && (suspend.standby_allowed || suspend.standby_allowed == -1))
		actions.append("Standby");

	setIcons();
	setTooltips();
	getSchemeList();
	setSchemeList();	
	setGeneralSettings();
	setInactivityBox();
	selectScheme(settings->currentScheme);
	
	tL_valueBrightness->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	tL_valueBrightness->setBackgroundMode(Qt::PaletteBase);
	tL_valueBrightness->setAlignment(Qt::AlignCenter);
	tL_valueBrightness->setFocusPolicy(NoFocus);

	// hide Lid item if this is not a laptop
	if (!hwinfo->isLaptop())
		cB_lockLid->hide();
	// hide CPU Freq page if not supported
	if (!hwinfo->supportCPUFreq()) {
		tB_scheme->setItemLabel( 4, tB_scheme->itemLabel(4) + i18n(" - not supported"));
		tB_scheme->setItemEnabled( 4, false);
	}

	if (!hwinfo->supportBrightness()) {
		tB_scheme->setItemLabel( 3, tB_scheme->itemLabel(3) + i18n(" - not supported"));
		tB_scheme->setItemEnabled( 3, false);
	}

	initalised = true;
	kdDebugFuncOut(trace);
}

/*! This is the default destructor of the class ConfigureDialog. */
ConfigureDialog::~ConfigureDialog(){
	kdDebugFuncIn(trace);
	 // no need to delete child widgets, Qt does it all for us
}

/* ---- START General setup SECTION ---- */

/*! 
 * This used to fill the QListBox listBox_schemes and the scheme related QComboBoxes
 * with the existing schemes.
 */
void ConfigureDialog::setSchemeList(){
	kdDebugFuncIn(trace);

	listBox_schemes->clear();
	cB_acScheme->clear();
	cB_batteryScheme->clear();

	for ( QStringList::Iterator it = schemes.begin(); it != schemes.end(); ++it ) {
		QString _tmp = *it;
		if(_tmp == "Performance" || _tmp == i18n("Performance")) {
			listBox_schemes->insertItem(SmallIcon("scheme_power", QIconSet::Automatic), i18n(_tmp));
			cB_acScheme->insertItem(i18n(_tmp));
			cB_batteryScheme->insertItem(i18n(_tmp));
		} else if(_tmp == "Powersave" || _tmp == i18n("Powersave")) {
			listBox_schemes->insertItem(SmallIcon("scheme_powersave", QIconSet::Automatic),
						    i18n(_tmp));
			cB_acScheme->insertItem(i18n(_tmp));
			cB_batteryScheme->insertItem(i18n(_tmp));
		} else if(_tmp == "Presentation" || _tmp == i18n("Presentation")){
			listBox_schemes->insertItem(SmallIcon("scheme_presentation", QIconSet::Automatic),
						    i18n(_tmp));
			cB_acScheme->insertItem(i18n(_tmp));
			cB_batteryScheme->insertItem(i18n(_tmp));
		} else if(_tmp == "Acoustic" || _tmp == i18n("Acoustic")) {
			listBox_schemes->insertItem(SmallIcon("scheme_acoustic", QIconSet::Automatic),
						    i18n(_tmp));
			cB_acScheme->insertItem(i18n(_tmp));
			cB_batteryScheme->insertItem(i18n(_tmp));
		} else if(_tmp == "AdvancedPowersave" || _tmp == i18n("Advanced Powersave")) {
			listBox_schemes->insertItem(SmallIcon("scheme_advanced_powersave", 
						    QIconSet::Automatic), i18n("Advanced Powersave"));
			cB_acScheme->insertItem(i18n("Advanced Powersave"));
			cB_batteryScheme->insertItem(i18n("Advanced Powersave"));
		} else {
			listBox_schemes->insertItem(i18n(_tmp));
			cB_acScheme->insertItem(i18n(_tmp));
			cB_batteryScheme->insertItem(i18n(_tmp));
		}
    	}

	KAcceleratorManager::manage(pB_newScheme);
	kdDebugFuncOut(trace);
}

/*! 
 * This used to get the list of schemes in from the config
 */
void ConfigureDialog::getSchemeList(){
	kdDebugFuncIn(trace);

	if (kconfig->hasGroup("General")) {
		kconfig->setGroup("General");
		schemes = kconfig->readListEntry("schemes", ',');
	}

	kdDebugFuncOut(trace);
}

/*! 
 * This used to set the current scheme based on the name of the scheme
 * \param _scheme 	QString with the name of the scheme
 */
void ConfigureDialog::selectScheme (QString _scheme){
	kdDebugFuncIn(trace);

	// select the current scheme in the listbox
	if(!_scheme.isEmpty()) {
		int pos = schemes.findIndex(_scheme);
		if(pos > -1) {
			listBox_schemes->setCurrentItem(pos);
			currentScheme = pos;
		} else {
			listBox_schemes->setCurrentItem(0);
		}
	} else {
		listBox_schemes->setCurrentItem(0);
	}
	
	kdDebugFuncOut(trace);
}

/*! 
 * This used to set all needed Icons for the dialog.
 */
void ConfigureDialog::setIcons(){
	kdDebugFuncIn(trace);

	/* set all Icons */
	this->setIcon(SmallIcon("kpowersave", QIconSet::Automatic));
	buttonApply->setIconSet(SmallIconSet("apply", QIconSet::Automatic));
	buttonCancel->setIconSet(SmallIconSet("cancel", QIconSet::Automatic));
	buttonOk->setIconSet(SmallIconSet("ok", QIconSet::Automatic));
	buttonHelp->setIconSet(SmallIconSet("help", QIconSet::Automatic));

	pB_editBlacklist->setIconSet(SmallIconSet("configure", QIconSet::Automatic));
	pB_editBlacklistDimm->setIconSet(SmallIconSet("configure", QIconSet::Automatic));
	pB_editAutosuspendGBlacklist->setIconSet(SmallIconSet("configure", QIconSet::Automatic));
	pB_editAutodimmGBlacklist->setIconSet(SmallIconSet("configure", QIconSet::Automatic));

	tB_scheme->setItemIconSet( 0 ,SmallIcon("kscreensaver", QIconSet::Automatic));
	tB_scheme->setItemIconSet( 1 ,SmallIcon("display", QIconSet::Automatic));
	
	if(actions[0] == "Suspend to Disk") 
		tB_scheme->setItemIconSet( 2 ,SmallIcon("suspend_to_disk", QIconSet::Automatic));
	else if(actions[0] == "Suspend to RAM") 
		tB_scheme->setItemIconSet( 2 ,SmallIcon("suspend_to_ram", QIconSet::Automatic));
	else if(actions[0] == "Standby") 
		tB_scheme->setItemIconSet( 2 ,SmallIcon("stand_by", QIconSet::Automatic));
	
	tB_scheme->setItemIconSet( 3 ,SmallIcon("autodimm", QIconSet::Automatic));
	tB_scheme->setItemIconSet( 4 ,SmallIcon("processor", QIconSet::Automatic));
	tB_scheme->setItemIconSet( 5 ,SmallIcon("misc", QIconSet::Automatic));

	tB_general->setItemIconSet( 0, SmallIcon( "scheme_powersave", QIconSet::Automatic));
	tB_general->setItemIconSet( 1, SmallIcon( "button", QIconSet::Automatic));
	tB_general->setItemIconSet( 2, SmallIcon( "scheme_power", QIconSet::Automatic));
	tB_general->setItemIconSet( 3, SmallIcon( "lock", QIconSet::Automatic));
	tB_general->setItemIconSet( 4, SmallIcon( "misc", QIconSet::Automatic));
	pB_configNotify->setIconSet(SmallIconSet("knotify", QIconSet::Automatic));

	kdDebugFuncOut(trace);
}

/*! 
 * This used to set all needed Tooltips for the dialog.
 */
void ConfigureDialog::setTooltips(){
	kdDebugFuncIn(trace);

	// QToolTip::setWakeUpDelay ( 1000 );
	QToolTip::add(cB_specificSettings, i18n("This enables specific screen saver settings. \n"
						"Note: If selected, the global screen saver settings are \n"
						"overwritten while kpowersave runs."));
	QToolTip::add(cB_disable_Ss, i18n("This disables the screen saver. \n"
					  "Note: If selected, the global screen saver settings are \n"
					  "overwritten while kpowersave runs."));
	QToolTip::add(cB_blankScreen, i18n("This blanks the screen instead of using a specific screen saver. \n"
					   "Note: This may work only with KScreensaver."));
	QToolTip::add(cB_SpecificPM, i18n("This enables specific DPMS settings. \n"
					  "Note: If selected, the global DPMS settings are \n"
					  "overwritten while kpowersave runs."));
	QToolTip::add(cB_disablePM, i18n("This disables DPMS support."));
	QToolTip::add(cB_lockSuspend, i18n("If selected, the screen is locked on suspend or standby."));
	QToolTip::add(cB_lockLid, i18n("If selected, the screen is locked if the lid close event is triggered."));
	
	QToolTip::add(cB_autoSuspend, i18n("Check this box to enable or disable automatic suspension of "
					   "the computer."));
	QToolTip::add(cB_autoInactivity, i18n("Activate this action if the user was inactive for the defined "
					      "time \n(greater than 0 minutes). If empty, nothing happens."));
	
	tB_scheme->setItemToolTip ( 0, i18n("All scheme-related screen saver and DPMS settings."));
	tB_scheme->setItemToolTip ( 1, i18n("All scheme-related display brightness settings."));
	tB_scheme->setItemToolTip ( 2, i18n("All scheme-related automatic suspend settings."));

	QToolTip::add(brightnessSlider, i18n("Here you can change the brightness of your display. \n"
					     "Use the slider to change the brightness directly for \n"
					     "testing. To reset back to previous level, please press \n"
					     "the 'Reset' button. "));
	QToolTip::add(pB_resetBrightness, i18n("Use this button to set back the slider and the "
					       "display brightness."));

	kdDebugFuncOut(trace);
}

/*!
 * This function is used to get the real name of the predefined schemes.
 * This is a workaround for the case if Yast translated the name of the 
 * scheme In this case is a new config section created and the old settings
 * would be lost.
 * \param s_scheme QString with the 'current' name of the scheme
 * \return QString with the english name of the scheme 
 */
QString ConfigureDialog::getSchemeRealName(QString s_scheme) {
	kdDebugFuncIn(trace);
	QString ret = s_scheme;

	if( s_scheme == "Performance" || s_scheme == i18n("Performance"))
		ret = "Performance";
	else if( s_scheme == "Powersave" || s_scheme == i18n("Powersave"))
		ret = "Powersave";
	else if( s_scheme == "Presentation" || s_scheme == i18n("Presentation"))
		ret = "Presentation";
	else if( s_scheme == "Acoustic" || s_scheme == i18n("Acoustic"))
		ret = "Acoustic";

	kdDebugFuncOut(trace);
	return ret;
}

/*!
 * This function is used to read the settings from configfile and
 * set the values to the related dialog elements.
 * \param schemeID Integer value, represent the position of the 
 *		   scheme in the QListBox and within the pdaemon-list.
 */
void ConfigureDialog::setConfigToDialog( int schemeID ){
	if (trace) kdDebug() << funcinfo << "IN , scheme: " << schemeID << endl;

	initalised = false;
	
	QString s_scheme = getSchemeRealName(schemes[schemeID]);
	if (kconfig->hasGroup(s_scheme)){
		settings->load_general_settings();
		
		// set the delete scheme button enabled if needed
		if (!s_scheme.startsWith("Performance") && !s_scheme.startsWith("Powersave") && 
		    !s_scheme.startsWith("Presentation") && !s_scheme.startsWith("Acoustic")) {
			if (!s_scheme.startsWith(settings->ac_scheme) || 
			    !s_scheme.startsWith(settings->battery_scheme)) {
				// enable delete button
				pB_deleteScheme->setEnabled( true );
				QToolTip::add(pB_deleteScheme, i18n("Press this button to delete the "
								    "selected scheme."));
			} else {
				// disable button
				pB_deleteScheme->setEnabled( false );
				QToolTip::add(pB_deleteScheme, i18n("You can't delete the current AC "
								    "or battery scheme."));
			}
		} else {
			// disable button, can't delete these scheme, they are default
			pB_deleteScheme->setEnabled( false );
			QToolTip::add(pB_deleteScheme, i18n("You can't delete this default scheme."));
		}

		kconfig->setGroup(s_scheme);
	}
	// no configuration found, set to default values ?!
	else { 
		if(kconfig->hasGroup("default-scheme"))
			kconfig->setGroup("default-scheme");
		else { 
			kdDebugFuncOut(trace);
			return;
		}
	}
	
	cB_disable_Ss_toggled(kconfig->readBoolEntry("disableSs",false));
	cB_blankScreen->setChecked(kconfig->readBoolEntry("blankSs",false));
	cB_specificSettings_toggled(kconfig->readBoolEntry("specSsSettings",false));
	
	int i_standby = kconfig->readNumEntry("standbyAfter", -1);
	if(i_standby >= 0) {
		sB_standby->setValue(i_standby);
	}
	else {
		// load value from default-scheme
		if(kconfig->hasGroup("default-scheme")){
			kconfig->setGroup("default-scheme");
			i_standby = kconfig->readNumEntry("standbyAfter", -1);
			if(i_standby >= 0) {
				sB_standby->setValue(i_standby);
			}
			else sB_standby->setValue(0);
		}
		else{
			sB_standby->setValue(0);
		}
		// reset group to selected scheme
		if (kconfig->hasGroup(s_scheme)) kconfig->setGroup(s_scheme);
		else kconfig->setGroup("default-scheme");
	}
	
	int i_suspend = kconfig->readNumEntry("suspendAfter", -1);
	if(i_suspend >= 0) {
		sB_suspend->setValue(i_suspend);
	}
	else {
		// load value from default-scheme
		if(kconfig->hasGroup("default-scheme")){
			kconfig->setGroup("default-scheme");
			i_standby = kconfig->readNumEntry("suspendAfter", -1);
			if(i_standby >= 0) {
				sB_standby->setValue(i_standby);
			}
			else sB_standby->setValue(0);
		}
		else{
			sB_standby->setValue(0);
		}
		// reset group to selected scheme
		if (kconfig->hasGroup(s_scheme)) kconfig->setGroup(s_scheme);
		else kconfig->setGroup("default-scheme");
	}
	
	int i_poweroff = kconfig->readNumEntry("powerOffAfter", -1);
	if(i_poweroff >= 0) {
		sB_powerOff->setValue(i_poweroff);
	}
	else {
		// load value from default-scheme
		if(kconfig->hasGroup("default-scheme")){
			kconfig->setGroup("default-scheme");
			i_standby = kconfig->readNumEntry("powerOffAfter", -1);
			if(i_poweroff >= 0) {
				sB_powerOff->setValue(i_poweroff);
			}
			else sB_powerOff->setValue(0);
		}
		else{
			sB_powerOff->setValue(0);
		}
		// reset group to selected scheme
		if (kconfig->hasGroup(s_scheme)) kconfig->setGroup(s_scheme);
		else kconfig->setGroup("default-scheme");
	}
	
	cB_SpecificPM_toggled(kconfig->readBoolEntry("specPMSettings",false));
	cB_disablePM_toggled(kconfig->readBoolEntry("disableDPMS",false));
	
	// set autosuspend related settings
	QString _action = kconfig->readEntry("autoInactiveAction", "NULL");
	if( _action != "NULL") {
		int _index = actions.findIndex(_action);
		if( _index != -1) { 
			cB_autoInactivity->setCurrentItem( _index );
			cB_autoInactivity_activated( _index );
			cB_Blacklist->setEnabled(true);
		}
		else {
			// set to empty element if not supported by current machine or
			// if the value is "_NONE_"
			cB_autoInactivity->setCurrentItem( 0 );
			cB_autoInactivity_activated( 0 );
			cB_Blacklist->setEnabled(false);
		}
	}
	else {
		// set to disabled (to reduce code), if a entry found set to enabled !
		cB_Blacklist->setEnabled(false);
		
		if(kconfig->hasGroup("default-scheme")){
			kconfig->setGroup("default-scheme");
			 _action = kconfig->readEntry("autoInactiveAction", "NULL");
			if(_action != "NULL") {
				int _index = actions.findIndex(_action);
				if( _index != -1) { 
					cB_autoInactivity->setCurrentItem( _index );
					tL_autoInactivity_After->setEnabled(true);
					cB_Blacklist->setEnabled(true);
				}
				else {
					cB_autoInactivity->setCurrentItem( 0 );
				}
			}
			else {
				cB_autoInactivity->setCurrentItem( 0 );
			}
			// reset group to selected scheme
			if (kconfig->hasGroup(s_scheme)) kconfig->setGroup(s_scheme);
			else kconfig->setGroup("default-scheme");
		}
		else{
			cB_autoInactivity->setCurrentItem( 0 );
		}
	}
	if( cB_autoInactivity->currentItem() == 0 ) {
		sB_autoInactivity->setEnabled(false);
		tL_autoInactivity_After->setEnabled(false);
	}
	
	int i_autoInactivityAfter = kconfig->readNumEntry("autoInactiveActionAfter", -1);
	if(i_autoInactivityAfter >= 0) {
		sB_autoInactivity->setValue(i_autoInactivityAfter);
	}
	else {
		// load value from default-scheme
		if(kconfig->hasGroup("default-scheme")){
			kconfig->setGroup("default-scheme");
			i_autoInactivityAfter = kconfig->readNumEntry("autoInactiveActionAfter", -1);
			if(i_autoInactivityAfter >= 0) {
				sB_autoInactivity->setValue(i_autoInactivityAfter);
			}
			else sB_autoInactivity->setValue(0);
		}
		else{
			sB_autoInactivity->setValue(0);
		}
		// reset group to selected scheme
		if (kconfig->hasGroup(s_scheme)) kconfig->setGroup(s_scheme);
		else kconfig->setGroup("default-scheme");
	}

	if (hwinfo->supportBrightness()) {
		// enable the widgets
		cB_Brightness->setEnabled(true);
		cB_Brightness_toggled(kconfig->readBoolEntry("enableBrightness",false));

		if (brightnessLevels > 33) 
			brightnessSlider->setLineStep(3);
		else 
			brightnessSlider->setLineStep(100/brightnessLevels);

		brightnessSlider->setPageStep(10);

		int i_brightnessPercent = kconfig->readNumEntry("brightnessPercent", -1);
		if(i_brightnessPercent >= 0) {
			brightnessSlider->setValue(i_brightnessPercent);
			tL_valueBrightness->setText(QString::number(i_brightnessPercent) + " %");
		}
		else {
			brightnessSlider->setValue(100);
			tL_valueBrightness->setText(QString::number(100)+ " %");	
		}
		
		tL_brightness->setText(i18n("Your hardware supports to change the brightness. The "
					    "values of the slider are in percent and mapped "
					    "to the available brightness levels of your hardware."));
	}
	else {
		cB_Brightness->setEnabled(false);
		gB_Brightness->setEnabled(false);
		tL_brightness->setText(i18n("Your Hardware currently not support changing the brightness "
					    "of your display."));
	}

	// no need to enable autodimm if not support change brightness 
	if (hwinfo->supportBrightness()) {
		int i_autoDimmAfter = kconfig->readNumEntry("autoDimmAfter", -1);
		if(i_autoDimmAfter >= 0) {
			sB_autoDimmTime->setValue(i_autoDimmAfter);
		}
		else {
			// load value from default-scheme
			if(kconfig->hasGroup("default-scheme")){
				kconfig->setGroup("default-scheme");
				i_autoDimmAfter = kconfig->readNumEntry("autoDimmAfter", -1);
				if(i_autoDimmAfter >= 0) {
					sB_autoDimmTime->setValue(i_autoDimmAfter);
				}
				else sB_autoDimmTime->setValue(0);
			}
			else{
				sB_autoDimmTime->setValue(0);
			}
			// reset group to selected scheme
			if (kconfig->hasGroup(s_scheme)) kconfig->setGroup(s_scheme);
			else kconfig->setGroup("default-scheme");
		}

		int i_autoDimmTo = kconfig->readNumEntry("autoDimmTo", -1);
		if(i_autoDimmTo >= 0) {
			sB_autoDimmTo->setValue(i_autoDimmTo);
		}
		else {
			// load value from default-scheme
			if(kconfig->hasGroup("default-scheme")){
				kconfig->setGroup("default-scheme");
				i_autoDimmTo = kconfig->readNumEntry("autoDimmTo", -1);
				if(i_autoDimmAfter >= 0) {
					sB_autoDimmTo->setValue(i_autoDimmTo);
				}
				else sB_autoDimmTo->setValue(0);
			}
			else{
				sB_autoDimmTo->setValue(0);
			}
			// reset group to selected scheme
			if (kconfig->hasGroup(s_scheme)) kconfig->setGroup(s_scheme);
			else kconfig->setGroup("default-scheme");
		}
	}

	if (hwinfo->supportCPUFreq()) {
		QString cpuFreqPolicy = kconfig->readEntry("cpuFreqPolicy");
		
		if (cpuFreqPolicy.isEmpty()) {
			// load value from default-scheme
			if(kconfig->hasGroup("default-scheme")){
				kconfig->setGroup("default-scheme");
				cpuFreqPolicy = kconfig->readEntry("cpuFreqPolicy");	
			} 
			if (cpuFreqPolicy.isEmpty()) {
				cpuFreqPolicy = "DYNAMIC";
			}
			// reset group to selected scheme
			if (kconfig->hasGroup(s_scheme)) kconfig->setGroup(s_scheme);
			else kconfig->setGroup("default-scheme");
		}

		if (cpuFreqPolicy.startsWith("PERFORMANCE")) {
			comboB_cpuFreq->setCurrentItem(0);
		} else if (cpuFreqPolicy.startsWith("DYNAMIC")) {
			comboB_cpuFreq->setCurrentItem(1);
		} else if (cpuFreqPolicy.startsWith("POWERSAVE")) {
			comboB_cpuFreq->setCurrentItem(2);
		} else {
			kdWarning() << "Could not read/map CPU Freq Policy, set to dynamic" << endl;
			comboB_cpuFreq->setCurrentItem(1);
		}
	}

	cB_disableNotifications->setChecked(kconfig->readBoolEntry("disableNotifications",false));
	
	cB_Blacklist_toggled(kconfig->readBoolEntry("autoInactiveSchemeBlacklistEnabled",false));
	cB_autoSuspend_toggled(kconfig->readBoolEntry("autoSuspend",false));
	cB_BlacklistDimm_toggled(kconfig->readBoolEntry("autoDimmSchemeBlacklistEnabled",false));
	cB_autoDimm_toggled(kconfig->readBoolEntry("autoDimm",false));
		
	initalised = true;
	scheme_changed = false;
	if(!general_changed) buttonApply->setEnabled(false);
	currentScheme = schemeID;

	kdDebugFuncOut(trace);
}

/*!
 * This is used to set the values from the section general in configfile to the 
 * related items in the dialog.
 */
void ConfigureDialog::setGeneralSettings() {
	kdDebugFuncIn(trace);

	kconfig->setGroup("General");
	
	cB_lockSuspend->setChecked(kconfig->readBoolEntry("lockOnSuspend",false));
	cB_lockLid->setChecked(kconfig->readBoolEntry("lockOnLidClose",false));
	cB_autostart->setChecked(kconfig->readBoolEntry("Autostart",false));
	cB_autostart_neverAsk->setChecked(kconfig->readBoolEntry("AutostartNeverAsk",false));
	
	QString lockmethod = kconfig->readEntry("lockMethod", "NULL");
	if(comboB_lock->count() == 0 ){
		comboB_lock->insertItem(i18n("Select Automatically"),0); 
		comboB_lock->insertItem(i18n("KScreensaver"),1);
		comboB_lock->insertItem(i18n("XScreensaver"),2);
		comboB_lock->insertItem(i18n("xlock"),3);
		if (gnome_session) comboB_lock->insertItem(i18n("GNOME Screensaver"),4);
	}
	
	if (lockmethod == "automatic") comboB_lock->setCurrentItem(0);
	else if (lockmethod == "kscreensaver")  comboB_lock->setCurrentItem(1);
	else if (lockmethod == "xscreensaver") comboB_lock->setCurrentItem(2);
	else if (lockmethod == "xlock") comboB_lock->setCurrentItem(3);
	else if (gnome_session && (lockmethod == "gnomescreensaver")) comboB_lock->setCurrentItem(4);
	else comboB_lock->setCurrentItem(0);
	
	if(cB_lockSuspend->isOn() || cB_lockLid->isOn() ) {
		tL_lockWith->setEnabled(true);
		comboB_lock->setEnabled(true);
	}
	else {
		tL_lockWith->setEnabled(false);
		comboB_lock->setEnabled(false);
	}

	// battery tab
	BatteryCollection* _primBats = hwinfo->getPrimaryBatteries();
	if (_primBats->getNumBatteries() > 0) {
		// set widgets in the tab
		sB_batWarning->setValue(kconfig->readNumEntry("batteryWarning",0));
		sB_batLow->setValue(kconfig->readNumEntry("batteryLow",0));
		sB_batCritical->setValue(kconfig->readNumEntry("batteryCritical",0));
		// hide them and enable only if needed
		sB_batWarnAction_value->hide();
		sB_batLowAction_value->hide();
		sB_batCritAction_value->hide();

		QString _select;
		QStringList _actions = kconfig->readListEntry("batteryAllowedActions", QString());
		_select = kconfig->readEntry("batteryWarningAction", QString());
		fillActionComboBox(cB_batWarning, _actions, _select);
		if (_select == "BRIGHTNESS" && hwinfo->supportBrightness()) {
			sB_batWarnAction_value->show();
			sB_batWarnAction_value->setValue(kconfig->readNumEntry( "batteryWarningActionValue",0));
		}
		_select = kconfig->readEntry("batteryLowAction", QString());
		fillActionComboBox(cB_batLow, _actions, _select);
		if (_select == "BRIGHTNESS" && hwinfo->supportBrightness()) {
			sB_batLowAction_value->show();
			sB_batLowAction_value->setValue( kconfig->readNumEntry( "batteryLowActionValue",0));
		}
		_select = kconfig->readEntry("batteryCriticalAction", QString());
		fillActionComboBox(cB_batCritical, _actions, _select);
		if (_select == "BRIGHTNESS" && hwinfo->supportBrightness()) {
			sB_batCritAction_value->show();
			sB_batCritAction_value->setValue(kconfig->readNumEntry("batteryCriticalActionValue",0));
		}
	} else {
		// disable tab 
		tB_general->setItemLabel( 0, tB_scheme->itemLabel(0) + i18n(" - not supported"));
		tB_general->setItemEnabled( 0, false);
	}

	// buttons tab:
	QStringList _actions = kconfig->readListEntry("buttonsAllowedActions", QString());
	fillActionComboBox(cB_PowerButton, _actions, kconfig->readEntry("ActionOnPowerButton", QString()));
	fillActionComboBox(cB_SleepButton, _actions, kconfig->readEntry("ActionOnSleepButton", QString()));
	fillActionComboBox(cB_S2DiskButton, _actions, kconfig->readEntry("ActionOnS2DiskButton", QString()));
	// avoid logout dialog for lidclose - this make no sense
	_actions.remove("LOGOUT_DIALOG");
	fillActionComboBox(cB_LidcloseButton, _actions, kconfig->readEntry("ActionOnLidClose", QString()));

	// default scheme tab:
	QString _ac_scheme = kconfig->readEntry( "ac_scheme", "Performance");
	QString _bat_scheme = kconfig->readEntry( "battery_scheme", "Powersave");
	cB_acScheme->setCurrentItem(schemes.findIndex(_ac_scheme));
	cB_batteryScheme->setCurrentItem(schemes.findIndex(_bat_scheme));

	kdDebugFuncOut(trace);
}

/* ---- END General setup SECTION ---- */

/* ---- START store settings SECTION ---- */

/*! 
 * This used to save changes in settings of the current scheme.
 */
void ConfigureDialog::saveSchemeSettings() {
	kdDebugFuncIn(trace);

	QString s_scheme = getSchemeRealName(schemes[currentScheme]);
	kconfig->setGroup(s_scheme);

	kconfig->writeEntry("specSsSettings",cB_specificSettings->isOn());
	kconfig->writeEntry("disableSs",cB_disable_Ss->isOn());
	kconfig->writeEntry("blankSs",cB_blankScreen->isOn());
	kconfig->writeEntry("specPMSettings",cB_SpecificPM->isOn());
	kconfig->writeEntry("disableDPMS",cB_disablePM->isOn());
	
	kconfig->writeEntry("standbyAfter",sB_standby->value());
	kconfig->writeEntry("suspendAfter",sB_suspend->value());
	kconfig->writeEntry("powerOffAfter",sB_powerOff->value());

	kconfig->writeEntry("disableNotifications",cB_disableNotifications->isOn());
		
	if(cB_autoInactivity->currentText() == " "){
		kconfig->writeEntry("autoInactiveAction", "_NONE_");
	}
	else{
		int _index = cB_autoInactivity->currentItem();
		if(_index > 0) {
			/*if(_index == (cB_autoInactivity->count()-1)) {
				kconfig->writeEntry("autoInactiveAction","shutdown");	
			}
			else {
				kconfig->writeEntry("autoInactiveAction",actions[(_index)]);
			}*/
			kconfig->writeEntry("autoInactiveAction",actions[(_index)]);
		}
		kconfig->writeEntry("autoInactiveActionAfter",sB_autoInactivity->value());
	}
	kconfig->writeEntry("autoSuspend",cB_autoSuspend->isOn());
	kconfig->writeEntry("autoInactiveSchemeBlacklistEnabled",cB_Blacklist->isOn());
	
	kconfig->writeEntry("autoDimm",cB_autoDimm->isOn());
	kconfig->writeEntry("autoDimmAfter", sB_autoDimmTime->value());
	kconfig->writeEntry("autoDimmTo", sB_autoDimmTo->value());
	kconfig->writeEntry("autoDimmSchemeBlacklistEnabled",cB_BlacklistDimm->isOn());

	kconfig->writeEntry("enableBrightness",cB_Brightness->isOn());
	if(brightness_changed)
		kconfig->writeEntry("brightnessPercent",brightnessSlider->value());


	if(hwinfo->supportCPUFreq()) {
		switch( comboB_cpuFreq->currentItem()) {
			case 0:
				kconfig->writeEntry("cpuFreqPolicy", "PERFORMANCE");
				break;
			case 2:
				kconfig->writeEntry("cpuFreqPolicy", "POWERSAVE");
				break;
			case 1:
			default:
				kconfig->writeEntry("cpuFreqPolicy", "DYNAMIC");
				break;
		}
	}

	kconfig->sync();
	scheme_changed = false;
	if(!general_changed) buttonApply->setEnabled(false);

	kdDebugFuncOut(trace);
}

/*! 
 * This used to save changes in settings of the current scheme.
 */
void ConfigureDialog::saveGeneralSettings() {
	kdDebugFuncIn(trace);

	kconfig->setGroup("General");
	
	kconfig->writeEntry("lockOnSuspend",cB_lockSuspend->isOn());
	kconfig->writeEntry("lockOnLidClose",cB_lockLid->isOn());
	kconfig->writeEntry("Autostart",cB_autostart->isOn());
	kconfig->writeEntry("AutostartNeverAsk",cB_autostart_neverAsk->isOn());
	
	QString selected_method = "";
	int _selected = comboB_lock->currentItem();
	if(_selected == 0) selected_method = "automatic";
	else if(_selected == 1) selected_method = "kscreensaver";
	else if(_selected == 2) selected_method = "xscreensaver";
	else if(_selected == 3) selected_method = "xlock";
	else if(gnome_session && (_selected == 4)) selected_method = "gnomescreensaver";
	kconfig->writeEntry( "lockMethod", selected_method );
	
	kconfig->writeEntry("batteryWarning", sB_batWarning->value());
	kconfig->writeEntry("batteryLow", sB_batLow->value());
	kconfig->writeEntry("batteryCritical", sB_batCritical->value());

	// battery level tab
	QString _action = mapDescriptionToAction(cB_batWarning->currentText());
	kconfig->writeEntry("batteryWarningAction", _action);
	if (_action == "BRIGHTNESS") {
		kconfig->writeEntry("batteryWarningActionValue", sB_batWarnAction_value->value());
	}
	_action = mapDescriptionToAction(cB_batLow->currentText());
	kconfig->writeEntry("batteryLowAction", _action);
	if (_action == "BRIGHTNESS") {
		kconfig->writeEntry("batteryLowActionValue", sB_batLowAction_value->value());
	}
	_action = mapDescriptionToAction(cB_batCritical->currentText());
	kconfig->writeEntry("batteryCriticalAction", _action);
	if (_action == "BRIGHTNESS") {
		kconfig->writeEntry("batteryCriticalActionValue", sB_batCritAction_value->value());
	}

	// button tab
	kconfig->writeEntry("ActionOnPowerButton", mapDescriptionToAction(cB_PowerButton->currentText()));
	kconfig->writeEntry("ActionOnLidClose", mapDescriptionToAction(cB_LidcloseButton->currentText()));
	kconfig->writeEntry("ActionOnSleepButton", mapDescriptionToAction(cB_SleepButton->currentText()));
	kconfig->writeEntry("ActionOnS2DiskButton", mapDescriptionToAction(cB_S2DiskButton->currentText()));

	// schemes tab
	kconfig->writeEntry("ac_scheme", getSchemeRealName(schemes[cB_acScheme->currentItem()]));
	kconfig->writeEntry("battery_scheme", getSchemeRealName(schemes[cB_batteryScheme->currentItem()]));

	kconfig->sync();
	general_changed = false;
	if(!scheme_changed) buttonApply->setEnabled(false);

	kdDebugFuncOut(trace);
}

/* ---- END store settings SECTION ---- */

/* ---- START monitor changes SECTION ---- */

/*!
 * SLOT: Called if a value within the Tab 'General Settings' is changed.
 */
void ConfigureDialog::general_valueChanged(){
	kdDebugFuncIn(trace);

	if(initalised) {
		general_changed = true;
		buttonApply->setEnabled(true);
		// enable/disable the comboB_lock and tL_lockWith
		if(cB_lockSuspend->isOn() || cB_lockLid->isOn() ) {
			tL_lockWith->setEnabled(true);
			comboB_lock->setEnabled(true);
		}
		else {
			tL_lockWith->setEnabled(false);
			comboB_lock->setEnabled(false);
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: Called if a value within the Tab 'Scheme Settings' is changed.
 */
void ConfigureDialog::scheme_valueChanged(){
	kdDebugFuncIn(trace);

	if(initalised) {
		scheme_changed = true;
		buttonApply->setEnabled(true);
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if the current scheme in the QListBox listBox_schemes is
 * changed/ a other scheme was selected.
 */
void ConfigureDialog::listBox_schemes_currentChanged(){
	kdDebugFuncIn(trace);

	// store current settings if something changed
	if(initalised && scheme_changed ) {
		int res = KMessageBox::warningYesNo(this,
						    i18n("There are unsaved changes in the active scheme.\n"
							 "Apply the changes before jumping to the next scheme "
							 "or discard the changes?"), 
						    i18n("Unsaved Changes"), KStdGuiItem::apply(),
						    KStdGuiItem::discard());
		
		if (res == KMessageBox::Yes) {
			// Save changes
			saveSchemeSettings();
		}
		if (res == KMessageBox::No) {
			// discard changes and reset trigger
			scheme_changed = false;
		}
	}
	// set to new Item
	setConfigToDialog(listBox_schemes->currentItem());

	kdDebugFuncOut(trace);
}

/* ---- END monitor changes SECTION ---- */

/* ---- START BUTTON SECTION ---- */

/*!
 * SLOT: called if the 'Apply' button is clicked.
 */
void ConfigureDialog::buttonApply_clicked(){
	kdDebugFuncIn(trace);

	if(initalised && scheme_changed ) {
		saveSchemeSettings();
		scheme_changed = false;
	}
	if(initalised && general_changed ) {
		saveGeneralSettings();
		general_changed = false;
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if the 'Chancel' button is clicked.
 */
void ConfigureDialog::buttonCancel_clicked(){
	kdDebugFuncOut(trace);

	if (scheme_changed || general_changed) {
		int res = KMessageBox::warningYesNoCancel(this,
             						  i18n("There are unsaved changes.\nApply the changes "
							       "before cancel or discard the changes?"), 
						          i18n("Unsaved Changes"), KStdGuiItem::apply(),
							  KStdGuiItem::discard());
		if (res == KMessageBox::Yes) {
			buttonApply_clicked();
		}
		else if (res == KMessageBox::Cancel) return;
	}

	kdDebugFuncOut(trace);
	close();
}

/*!
 * SLOT: called if the 'OK' button is clicked.
 */
void ConfigureDialog::buttonOk_clicked(){
	kdDebugFuncIn(trace);

	buttonApply_clicked();
	buttonApply->setEnabled(false);
	
	kdDebugFuncOut(trace);
	close();
}

/*!
 * SLOT: called if the 'Help' button is clicked.
 */
void ConfigureDialog::buttonHelp_clicked(){
	kdDebugFuncIn(trace);

	emit openHelp();

	kdDebugFuncOut(trace);
}

/* ----  END BUTTON SECTION ---- */

/* ---- START SCHEME ADD/DELETE SECTION ---- */

/*!
 * SLOT: called if the 'New' scheme button is clicked.
 */
void ConfigureDialog::pB_newScheme_clicked(){
	kdDebugFuncIn(trace);

	bool _ok = false;
	bool _end = false;
	QString _new;
	QString _text = i18n("Please insert a name for the new scheme:");
	QString _error;

	getSchemeList();

	while (!_end) {
		_new = KInputDialog::getText( i18n("KPowersave Configuration"), 
					      _error + _text, QString::null, &_ok, this);
		if (!_ok ) {
			_end = true;
		} else {
			_error = QString();
			if (!_new.isEmpty()) {
				if ( schemes.contains(_new))
					_error = i18n("Error: A scheme with this name already exist.\n");
				else 
					_end = true;
			}
		}
	}

	if (!_new.isEmpty()) {
		// write append new scheme to list
		schemes.append(_new);
		kconfig->setGroup("General");
		kconfig->writeEntry("schemes", schemes, ",");
		kconfig->sync();

		// update GUI
		setSchemeList();
		selectScheme(_new);
		saveSchemeSettings();
	}
	
	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if the 'Delete' scheme button is clicked.
 */
void ConfigureDialog::pB_deleteScheme_clicked(){
	kdDebugFuncIn(trace);

	if (pB_deleteScheme->isEnabled()) {
		int answer = KMessageBox::questionYesNo( this, i18n("Do you really want to delete the "
							         "%1 scheme?").arg(schemes[currentScheme]),
							 i18n("Confirm delete scheme"),
							 i18n("Delete"), i18n("Cancel"));
		if (answer == KMessageBox::Yes) {
			// delete the scheme, we can be sure this is a userscheme
			QString _s_tmp = getSchemeRealName(schemes[currentScheme]);
			if (kconfig->hasGroup(_s_tmp) && kconfig->deleteGroup(_s_tmp)) {
				schemes.remove(_s_tmp);
				kconfig->setGroup("General");
				kconfig->writeEntry("schemes", schemes, ",");
				kconfig->sync();

				// update GUI
				setSchemeList();
				selectScheme(settings->currentScheme);
			} else {
				// could not delete the scheme ... error case
				KMessageBox::queuedMessageBox(this, KMessageBox::Error, 
							      i18n("Could not delete the selected scheme."));
			}
		}
	}

	kdDebugFuncOut(trace);
}

/* ----  END SCHEME ADD/DELETE SECTION ---- */

/* ---- START SCREENSAVER SECTION ---- */

/*!
 * SLOT: called if QCheckBox cB_specificSettings is toggled.
 * \param state boolean, true if toggled on
 *		   	 false if toggled off
 */
void ConfigureDialog::cB_specificSettings_toggled(bool state){
	kdDebugFuncIn(trace);

	if(!initalised) cB_specificSettings->setChecked(state);
	
	/* set widgets visible/disabled */
	if(state){
		cB_disable_Ss->setEnabled(true);
		if(cB_disable_Ss->isOn()){
			cB_blankScreen->setEnabled(false);
		}
		else {
			cB_blankScreen->setEnabled(true);
		}
	}
	else {
		cB_disable_Ss->setEnabled(false);
		cB_blankScreen->setEnabled(false);
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if QCheckBox cB_disable_Ss is toggled.
 * \param state boolean, true if toggled on
 *		   	 false if toggled off
 */
void ConfigureDialog::cB_disable_Ss_toggled(bool state){
	kdDebugFuncIn(trace);

	/* set widgets visible/disabled */
	if(!initalised) cB_disable_Ss->setChecked(state);
	cB_blankScreen->setEnabled(!state);
	
	kdDebugFuncOut(trace);
}


/* ---- END SCREENSAVER SECTION ---- */

/* ---- START DPMS SECTION ---- */

/*!
 * SLOT: called if QCheckBox cB_SpecificPM is toggled.
 * \param state boolean, true if toggled on
 *		   	 false if toggled offtrue
 */
void ConfigureDialog::cB_SpecificPM_toggled(bool state){
	kdDebugFuncIn(trace);

	if(!initalised) cB_SpecificPM->setChecked(state);
	
	/* set widgets visible/disabled */
	cB_disablePM->setEnabled(state);
	if(cB_disablePM->isOn()) {
		state = false;
	}
	tL_standbyAfter->setEnabled(state);
	sB_standby->setEnabled(state);
	tL_suspendAfter->setEnabled(state);
	sB_suspend->setEnabled(state);
	tL_powerOffAfter->setEnabled(state);
	sB_powerOff->setEnabled(state);

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if QCheckBox cB_disablePM is toggled.
 * \param state boolean, true if toggled on
 *			 false if toggled off
 */
void ConfigureDialog::cB_disablePM_toggled(bool state){
	kdDebugFuncIn(trace);

	if(!initalised) cB_disablePM->setChecked(state);

	/* set widgets visible/disabled */
	tL_standbyAfter->setEnabled(!state);
	sB_standby->setEnabled(!state);
	tL_suspendAfter->setEnabled(!state);
	sB_suspend->setEnabled(!state);
	tL_powerOffAfter->setEnabled(!state);
	sB_powerOff->setEnabled(!state);

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if the standby QSpinBoxes for DPMS timout is changed.
 */
void ConfigureDialog::sB_standby_valueChanged() {
	kdDebugFuncIn(trace);

	if (initalised) {
		if (sB_standby->value() == 0 ) 
			return;
		if ( sB_standby->value() > sB_suspend->value()) {
			sB_suspend->setValue(sB_standby->value());
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if the suspend QSpinBoxes for DPMS timout is changed.
 */
void ConfigureDialog::sB_suspend_valueChanged() {
	kdDebugFuncIn(trace);

	if (initalised) {
		if (sB_suspend->value() == 0 ) 
			return;

		if ( sB_suspend->value() < sB_standby->value()) {
			sB_standby->setValue(sB_suspend->value());
		}
		if ( sB_suspend->value() > sB_powerOff->value()) {
			sB_powerOff->setValue(sB_suspend->value());
		}
	}
	
	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if the powerOff QSpinBoxes for DPMS timout is changed.
 */
void ConfigureDialog::sB_powerOff_valueChanged() {
	kdDebugFuncIn(trace);

	if (initalised) {
		if (sB_powerOff->value() == 0 ) 
			return;
		if ( sB_powerOff->value() < sB_suspend->value()) {
			sB_suspend->setValue(sB_powerOff->value());
		}
	}

	kdDebugFuncOut(trace);
}

/* ----  END DPMS SECTION ---- */

/* ---- START Inactivity SECTION ---- */

/*! 
 * This used to set the inactivity related combobox cB_autoInactivity .
 */
void ConfigureDialog::setInactivityBox(){
	kdDebugFuncIn(trace);

	cB_autoInactivity->clear();
	// add "nothing" at start of the list
	actions.push_front(" ");
	// add "Turn Off Computer" at end of the list
	// QString _to_i18n = i18n("Turn Off Computer");
	// actions.append("Turn Off Computer");
	
	for ( QStringList::Iterator it = actions.begin(); it != actions.end(); ++it ) {
		cB_autoInactivity->insertItem( i18n( *it ) );
	}

	kdDebugFuncOut(trace);
}

/*! 
 * This used to set the autosuspend related widgets.
 */
void ConfigureDialog::cB_autoSuspend_toggled( bool toggled ) {
	kdDebugFuncIn(trace);

	if(cB_autoSuspend->isOn() != toggled)
		cB_autoSuspend->setChecked(toggled);	

	tL_autoInactivity_explain->setEnabled(toggled);
	cB_autoInactivity->setEnabled(toggled);
	if(cB_autoInactivity->currentItem() > 0) {
		cB_autoInactivity->setEnabled(true);
		tL_autoInactivity_After->setEnabled(true);
		sB_autoInactivity->setEnabled(true);
		cB_Blacklist->setEnabled(true);
		cB_Blacklist_toggled(cB_Blacklist->isOn());
	}
	
	if(!toggled) {
		cB_autoSuspend->setChecked(false);
		tL_autoInactivity_After->setEnabled(false);
		cB_autoInactivity->setEnabled(false);
		sB_autoInactivity->setEnabled(false);
		cB_Blacklist->setEnabled(false);
		pB_editBlacklist->setEnabled(false);
	}
	
	scheme_changed = true;
	buttonApply->setEnabled(true);

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if a new item in cB_autoInactivity is selected.
 * \param selectedItem Integer, contains the ID of the new item in the
 *				comboBox.
 */
void ConfigureDialog::cB_autoInactivity_activated( int selectedItem ) {
	kdDebugFuncIn(trace);

	if( actions[selectedItem] != " " ) {
		if(!displayed_WARN_autosuspend && initalised) {
			QString _msg = "<qt>" + i18n("<b>Note:</b> If you select this option, the computer "
						     "will suspend or standby if the current user is "
						     "inactive for the defined time even if somebody is "
						     "logged in remotely to the X server.<br><br> This "
						     "feature can also produce problems with some programs, "
						     "such as video players or cd burner. These programs can "
						     "be blacklisted by checking <b>Enable scheme-specific "
						     "blacklist</b> and click <b>Edit Blacklist...</b>. If "
						     "this does not help, report the problem or deactivate "
						     "autosuspend.<br><br> Really use this option?") +
				       "</qt>";

			int tmp = KMessageBox::warningContinueCancel(this, _msg);
			if (tmp ==  KMessageBox::Cancel) {
				selectedItem = 0;
			}
			displayed_WARN_autosuspend = true;
		}
		sB_autoInactivity->setEnabled(true);
		tL_autoInactivity_After->setEnabled(true);
		cB_Blacklist->setEnabled(true);
		if(cB_Blacklist->isChecked()) pB_editBlacklist->setEnabled(true);
	
	}
	else {
		sB_autoInactivity->setEnabled(false);
		tL_autoInactivity_After->setEnabled(false);
		cB_Blacklist->setEnabled(false);
		pB_editBlacklist->setEnabled(false);
	}
	
	cB_autoInactivity->setCurrentItem( selectedItem );
	
	if(initalised) {
		scheme_changed = true;
		buttonApply->setEnabled(true);
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: Called if the 'Enable scheme specific blacklist' is toggled.
 * \param toggled boolean, true if toggled on
 *			   false if toggled off
 */
void ConfigureDialog::cB_Blacklist_toggled( bool toggled ){
	kdDebugFuncIn(trace);

	pB_editBlacklist->setEnabled(toggled);
	
	if(initalised) {
		buttonApply->setEnabled(true);
		scheme_changed = true;
	} else {
		cB_Blacklist->setChecked(toggled);
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: Called if the 'edit blacklist' button is toggled.
 */
void ConfigureDialog::pB_editBlacklistSuspend_clicked(){
	kdDebugFuncIn(trace);

	QString _top_text = "";
	bool initialiseImport = false;

	if(tabWidget->currentPageIndex() == 0 ) {
		QString s_scheme = getSchemeRealName(schemes[listBox_schemes->currentItem()]);
		_top_text = listBox_schemes->currentText();
		if (kconfig->hasGroup(s_scheme)){
			kconfig->setGroup(s_scheme);
		}
		blacklist = kconfig->readListEntry("autoInactiveSchemeBlacklist", ',');
		if( blacklist.empty()) {
				QString _msg = i18n("The blacklist of the selected scheme is empty. "
					    "Import the general blacklist?");
			int tmp = KMessageBox::questionYesNo(this, _msg, QString(), i18n("Import"), i18n("Do Not Import"));
			if (tmp ==  KMessageBox::Yes) {
				initialiseImport = true;
				if(kconfig->hasGroup("General")){
					kconfig->setGroup("General");
					blacklist = kconfig->readListEntry("autoInactiveBlacklist", ',');
				}
			}
		}
	}
	else {
		if(kconfig->hasGroup("General")){
			_top_text = i18n("General Autosuspend Blacklist");
			kconfig->setGroup("General");
			blacklist = kconfig->readListEntry("autoInactiveBlacklist", ',');
		}
	}
	blacklistEDlgAS = new blacklistEditDialog(blacklist, _top_text, initialiseImport, this);
	
	connect( blacklistEDlgAS, SIGNAL(config_finished(QStringList)), this, 
		 SLOT(saveSchemeSuspendBlacklist(QStringList)));
	blacklistEDlgAS->exec();

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: Called if the signal config_finished(QStringList) recieved
 * and the autosuspend blacklist is edited.
 * \param new_blacklist QStringlist with the edited blacklisted processes
 */

void ConfigureDialog::saveSchemeSuspendBlacklist( QStringList new_blacklist){
	kdDebugFuncIn(trace);

	if(tabWidget->currentPageIndex() == 0 ) {
		QString s_scheme = getSchemeRealName(schemes[currentScheme]);
		kconfig->setGroup(s_scheme);
		kconfig->writeEntry("autoInactiveSchemeBlacklist", new_blacklist, ',');
	}
	else {
		kconfig->setGroup("General");
		kconfig->writeEntry("autoInactiveBlacklist", new_blacklist, ',');
	}

	kconfig->sync();
	kdDebugFuncOut(trace);
}

/* ---- END Inactivity SECTION ---- */

/* ---- START Autodimm SECTION ---- */

/*! 
 * This used to set the autodimm related widgets.
 */
void ConfigureDialog::cB_autoDimm_toggled( bool toggled ) {
	kdDebugFuncIn(trace);

	if(cB_autoDimm->isOn() != toggled)
		cB_autoDimm->setChecked(toggled);	

	if (toggled)
	tL_autoDimmExplain->setEnabled(toggled);
	tL_autoDimmAfter->setEnabled(toggled);
	sB_autoDimmTime->setEnabled(toggled);
	tL_autoDimmTo->setEnabled(toggled);
	sB_autoDimmTo->setEnabled(toggled);

	if (sB_autoDimmTime->value() > 0)
		cB_BlacklistDimm->setEnabled(toggled);

	if (cB_BlacklistDimm->isOn())
		pB_editBlacklistDimm->setEnabled(toggled);
	
	if (toggled) {
		if (sB_autoDimmTime->value() > 0) {
			tL_autoDimmTo->setEnabled(true);
			sB_autoDimmTo->setEnabled(true);
		} else {
			tL_autoDimmTo->setEnabled(false);
			sB_autoDimmTo->setEnabled(false);
		}
	}

	scheme_changed = true;
	buttonApply->setEnabled(true);

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: Called if the 'Enable scheme specific blacklist' is toggled.
 * \param toggled boolean, true if toggled on
 *			   false if toggled off
 */
void ConfigureDialog::cB_BlacklistDimm_toggled( bool toggled ){
	kdDebugFuncIn(trace);

	pB_editBlacklistDimm->setEnabled(toggled);
	
	if(initalised) {
		buttonApply->setEnabled(true);
		scheme_changed = true;
	} else {
		cB_BlacklistDimm->setChecked(toggled);
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: Called if the 'edit blacklist' button for autodimm is toggled.
 */
void ConfigureDialog::pB_editBlacklistDimm_clicked(){
	kdDebugFuncIn(trace);

	QString _top_text = "";
	bool initialiseImport = false;

	if(tabWidget->currentPageIndex() == 0 ) {
		QString s_scheme = getSchemeRealName(schemes[listBox_schemes->currentItem()]);
		_top_text = listBox_schemes->currentText();
		if (kconfig->hasGroup(s_scheme)){
			kconfig->setGroup(s_scheme);
		}
		blacklist = kconfig->readListEntry("autoDimmSchemeBlacklist", ',');
		if( blacklist.empty()) {
				QString _msg = i18n("The blacklist of the selected scheme is empty. "
						    "Import the general blacklist?");
			int tmp = KMessageBox::questionYesNo(this, _msg, QString(), i18n("Import"), i18n("Do Not Import"));
			if (tmp ==  KMessageBox::Yes) {
				initialiseImport = true;
				if(kconfig->hasGroup("General")){
					kconfig->setGroup("General");
					blacklist = kconfig->readListEntry("autoDimmBlacklist", ',');
				}
			}
		}
	}
	else {
		if(kconfig->hasGroup("General")){
			_top_text = i18n("General Autodimm Blacklist");
			kconfig->setGroup("General");
			blacklist = kconfig->readListEntry("autoDimmBlacklist", ',');
		}
	}
	blacklistEDlgAD = new blacklistEditDialog(blacklist, _top_text, initialiseImport, this);
	
	connect( blacklistEDlgAD, SIGNAL(config_finished(QStringList)), this, 
		 SLOT(saveSchemeDimmBlacklist(QStringList)));
	blacklistEDlgAD->exec();

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: Called if the signal config_finished(QStringList) recieved
 * and the autdimm blacklist is edited.
 * \param new_blacklist QStringlist with the edited blacklisted processes
 */
void ConfigureDialog::saveSchemeDimmBlacklist( QStringList new_blacklist){
	kdDebugFuncIn(trace);

	if(tabWidget->currentPageIndex() == 0 ) {
		QString s_scheme = getSchemeRealName(schemes[currentScheme]);
		kconfig->setGroup(s_scheme);
		kconfig->writeEntry("autoDimmSchemeBlacklist", new_blacklist, ',');
	}
	else {
		kconfig->setGroup("General");
		kconfig->writeEntry("autoDimmBlacklist", new_blacklist, ',');
	}

	kconfig->sync();
	kdDebugFuncOut(trace);
}

/*!
 * SLOT: Called if there \ref sB_autoDimmTime get changed
 * \param value Integer with the new value
 */
void ConfigureDialog::sB_autoDimmTime_valueChanged( int value ) {
	kdDebugFuncIn(trace);

	if (value > 0) {
		if (!tL_autoDimmTo->isEnabled()) {
			tL_autoDimmTo->setEnabled(true);
			sB_autoDimmTo->setEnabled(true);
			cB_BlacklistDimm->setEnabled(true);
			if (cB_BlacklistDimm->isOn())
				pB_editBlacklistDimm->setEnabled(true);
		}
	} else {
		if (tL_autoDimmTo->isEnabled()) {
			tL_autoDimmTo->setEnabled(false);
			sB_autoDimmTo->setEnabled(false);
			cB_BlacklistDimm->setEnabled(false);
			pB_editBlacklistDimm->setEnabled(false);
		}
	}
	kdDebugFuncOut(trace);
}


/* ---- END Autodimm SECTION ---- */

/* ---- START Brightness SECTION ---- */
/*! \b SLOT: to enable the brigthness related widgets */
void ConfigureDialog::cB_Brightness_toggled( bool toggled ) {
	kdDebugFuncIn(trace);

	gB_Brightness->setEnabled(toggled);
	cB_Brightness->setChecked(toggled);
	connect(brightnessSlider, SIGNAL(valueChanged (int)), this, SLOT(brightnessSlider_sliderMoved(int)));

	kdDebugFuncOut(trace);
}

/*! \b SLOT: to change the brightness if the slider is changed */
void ConfigureDialog::brightnessSlider_sliderMoved( int new_value ) {
	kdDebugFuncIn(trace);

	if (cB_Brightness->isEnabled() && cB_Brightness->isChecked()) {
		scheme_valueChanged();
		tL_valueBrightness->setText(QString::number(new_value) + " %");
		hwinfo->setBrightness(-1, new_value);
		pB_resetBrightness->setEnabled(true);
		brightness_changed = true;
	}

	kdDebugFuncOut(trace);
}

/*! \b SLOT: to reset the brightness if the reset button clicked */
void ConfigureDialog::pB_resetBrightness_clicked( ) {
	kdDebugFuncIn(trace);

	hwinfo->setBrightness(brightness_last, -1);
	brightnessSlider->setValue(brightness_last);
	pB_resetBrightness->setEnabled(false);
	brightness_changed = false;

	kdDebugFuncOut(trace);
}

/* ---- END Brightness SECTION ---- */

/* ---- START battery level SECTION ---- */
/*!
 * SLOT: called if the warning QSpinBoxes for battery level is changed.
 */
void ConfigureDialog::sB_batWarning_valueChanged() {
	kdDebugFuncIn(trace);
	
	if (initalised) {
		if (sB_batWarning->value() == 0 ) 
			sB_batWarning->setValue(1);
		if ( sB_batWarning->value() <= sB_batLow->value()) {
			sB_batLow->setValue(sB_batWarning->value()-1);
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if the low QSpinBoxes for battery level is changed.
 */
void ConfigureDialog::sB_batLow_valueChanged() {
	kdDebugFuncIn(trace);

	if (initalised) {
		if (sB_batLow->value() == 0 ) 
			sB_batLow->setValue(1);

		if ( sB_batLow->value() >= sB_batWarning->value()) {
			sB_batWarning->setValue(sB_batLow->value()+1);
		}
		if ( sB_batLow->value() <= sB_batCritical->value()) {
			sB_batCritical->setValue(sB_batLow->value()-1);
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if the critical QSpinBoxes for battery level is changed.
 */
void ConfigureDialog::sB_batCritical_valueChanged() {
	kdDebugFuncIn(trace);

	if (initalised) {
		if (sB_batCritical->value() == 0 ) 
			sB_batCritical->setValue(1);
		if ( sB_batCritical->value() >= sB_batLow->value()) {
			sB_batLow->setValue(sB_batCritical->value()+1);
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if the QComboBox for the battery warning level action changed,
 *       used to hide/show the related QSpinboxes if needed.
 */
void ConfigureDialog::cB_batWarning_activated() {
	kdDebugFuncIn(trace);

	if ( mapDescriptionToAction(cB_batWarning->currentText()) == "BRIGHTNESS") {
		sB_batWarnAction_value->show();
	} else {
		sB_batWarnAction_value->hide();
	}

	kdDebugFuncOut(trace);
}

//! called if the QComboBox for the battery low level action changed
/*!
 *       used to hide/show the related QSpinboxes if needed.
 */
void ConfigureDialog::cB_batLow_activated() {
	kdDebugFuncIn(trace);

	if ( mapDescriptionToAction(cB_batLow->currentText()) == "BRIGHTNESS") {
		sB_batLowAction_value->show();
	} else {
		sB_batLowAction_value->hide();
	}

	kdDebugFuncOut(trace);
}

/*!
 * SLOT: called if the QComboBox for the battery critical level action changed,
 *       used to hide/show the related QSpinboxes if needed.
 */
void ConfigureDialog::cB_batCritical_activated() {
	kdDebugFuncIn(trace);

	if ( mapDescriptionToAction(cB_batCritical->currentText()) == "BRIGHTNESS") {
		sB_batCritAction_value->show();
	} else {
		sB_batCritAction_value->hide();
	}

	kdDebugFuncOut(trace);
}

/* ---- END battery level SECTION ---- */

/*! \b SLOT: to open the KNotify config dialog */
void ConfigureDialog::pB_configNotify_released( ) {
	kdDebugFuncIn(trace);

	emit openKNotify();

	kdDebugFuncOut(trace);
}

/* ---- START helper functions SECTION ---- */
/*!
 * Map the key from config for a action to a descriptive name.
 * \param action	QString with the config key value
 * \return 		QString with the description
 */
QString ConfigureDialog::mapActionToDescription( QString action ) {
	kdDebugFuncIn(trace);

	QString ret;

	if (action.startsWith("SHUTDOWN")) {
		ret = i18n("Shutdown");
	} else if (action.startsWith("LOGOUT_DIALOG")) {
		ret = i18n("Logout Dialog");
	} else if (action.startsWith("SUSPEND2DISK")) {
		if (actions.contains("Suspend to Disk"))
			ret = i18n("Suspend to Disk");
	} else if (action.startsWith("SUSPEND2RAM")) {
		if (actions.contains("Suspend to RAM"))
			ret = i18n("Suspend to RAM");
	} else if (action.startsWith("CPUFREQ_POWERSAVE")) {
		if (hwinfo->supportCPUFreq())
			ret = i18n("CPU Powersave policy");
	} else if (action.startsWith("CPUFREQ_DYNAMIC")) {
		if (hwinfo->supportCPUFreq())
			ret = i18n("CPU Dynamic policy");
	} else if (action.startsWith("CPUFREQ_PERFORMANCE")) {
		if (hwinfo->supportCPUFreq())
			ret = i18n("CPU Performance policy");
	} else if (action.startsWith("BRIGHTNESS")) {
		if (hwinfo->supportBrightness())
			ret = i18n("Set Brightness to");
	} 

	kdDebugFuncOut(trace);
	return ret;
}

/*!
 * Map the action description (maybe translated) back to a QString key for the config.
 * \param description	QString with the description
 * \return 		QString with the config key value
 */
QString ConfigureDialog::mapDescriptionToAction( QString description ) {
	kdDebugFuncIn(trace);

	QString ret;

	if (description.startsWith("Shutdown") || description.startsWith(i18n("Shutdown"))) {
		ret = "SHUTDOWN";
	} else if (description.startsWith("Logout Dialog") || 
		   description.startsWith(i18n("Logout Dialog"))) {
		ret = "LOGOUT_DIALOG";
	} else if (description.startsWith("Suspend to Disk") || 
		   description.startsWith(i18n("Suspend to Disk"))) {
		ret = "SUSPEND2DISK";
	} else if (description.startsWith("Suspend to RAM") || 
		   description.startsWith(i18n("Suspend to RAM"))) {
		ret = "SUSPEND2RAM";
	} else if (description.startsWith("CPU Powersave policy") || 
		   description.startsWith(i18n("CPU Powersave policy"))) {
		ret = "CPUFREQ_POWERSAVE";
	} else if (description.startsWith("CPU Dynamic policy") || 
		   description.startsWith(i18n("CPU Dynamic policy"))) {
		ret = "CPUFREQ_DYNAMIC";
	} else if (description.startsWith("CPU Performance policy") || 
		   description.startsWith(i18n("CPU Performance policy"))) {
		ret = "CPUFREQ_PERFORMANCE";
	} else if (description.startsWith("Set Brightness to") || 
		   description.startsWith(i18n("Set Brightness to"))) {
		ret = "BRIGHTNESS";
	} 

	kdDebugFuncOut(trace);
	return ret;
}

/*!
 * Helper to fill a QComboBox with their text and select the correct item
 * \param _cb 		Pointer to the QComboBox
 * \param _actions	QStringList with the allowed actions
 * \param _select	QString with the action to select
 */
void ConfigureDialog::fillActionComboBox(QComboBox *_cb, QStringList _actions, QString _select) {
	kdDebugFuncIn(trace);

	_cb->clear();
	_cb->insertItem("");

	for ( QStringList::Iterator it = _actions.begin(); it != _actions.end(); ++it ) {
		QString _tmp = *it;

		QString _desc = mapActionToDescription( _tmp );
		if (!_desc.isEmpty()) {
			_cb->insertItem( _desc );
			if (_tmp == _select) {
				_cb->setCurrentItem(_cb->count()-1);
			}
		}
	}

	kdDebugFuncOut(trace);
}

/* ---- END helper functions SECTION ---- */

#include "configuredialog.moc"
