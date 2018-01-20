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

/*! 
 * \file 	settings.cpp
 * \brief 	In this file can be found the settings ( read ) related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2005
 */

// KDE Header
#include <klocale.h>

// QT Header 

// own headers
#include "settings.h"

/*! This is the default constructor of the class Settings. */
Settings::Settings()
{
	kconfig = new KConfig("kpowersaverc", true );
	kde = new KDE_Settings();
	load_kde();
	load_general_settings();
}

/*! This is the default destructor of the class Settings. */
Settings::~Settings()
{
	delete kconfig;
	delete kde;
}


/*!
 * Loads the scheme settings from kpowersaverc and fills the related variables.
 * \param schemeName QString with the name (realname not i18n() version) of the 
 *		     scheme which setting should be load. If the scheme could not
 *		     be loaded, this function try to load "default-scheme"
 * \return the result of the load
 * \retval true  if the settings could be loaded
 * \retval false if there was no group named like schemeName or named "default-scheme"
 */
bool Settings::load_scheme_settings(QString schemeName){
	
	kconfig->reparseConfiguration();
	bool setToDefault = false;

	if( schemeName == "Performance" || schemeName == i18n("Performance"))
		 schemeName = "Performance";
	else if( schemeName == "Powersave" || schemeName == i18n("Powersave"))
		 schemeName = "Powersave";
	else if( schemeName == "Presentation" || schemeName == i18n("Presentation"))
		 schemeName = "Presentation";
	else if( schemeName == "Acoustic" || schemeName == i18n("Acoustic"))
		 schemeName = "Acoustic";
	
	if(kconfig->hasGroup(schemeName) || kconfig->hasGroup("default-scheme") ){
		if(kconfig->hasGroup(schemeName)) kconfig->setGroup(schemeName);
		else {
			// fallback to 'default-scheme'
			kconfig->setGroup("default-scheme");
			schemeName = "default-scheme";
			setToDefault = true;
		}
		currentScheme = schemeName;
		
		specSsSettings = kconfig->readBoolEntry("specSsSettings",false);
		disableSs = kconfig->readBoolEntry("disableSs",false);
		blankSs = kconfig->readBoolEntry("blankSs",false);
		specPMSettings = kconfig->readBoolEntry("specPMSettings",false);
		disableDPMS = kconfig->readBoolEntry("disableDPMS",false);
		
		int i_standby = kconfig->readNumEntry("standbyAfter", -1);
		if (i_standby >= 0) standbyAfter = i_standby;
		else {
			kconfig->setGroup("default-scheme");
			i_standby = kconfig->readNumEntry("standbyAfter", -1);
			if(i_standby >= 0) {
				standbyAfter = i_standby;
			}
			else standbyAfter = 0;
			// reset the group
			kconfig->setGroup(schemeName);
		}
		
		int i_suspend = kconfig->readNumEntry("suspendAfter", -1);
		if (i_suspend >= 0) suspendAfter = i_suspend;
		else {
			kconfig->setGroup("default-scheme");
			i_suspend = kconfig->readNumEntry("suspendAfter", -1);
			if(i_suspend >= 0) {
				suspendAfter = i_suspend;
			}
			else suspendAfter = 0;
			// reset the group
			kconfig->setGroup(schemeName);
		}
		
		int i_poweroff = kconfig->readNumEntry("powerOffAfter", -1);
		if (i_poweroff >= 0) powerOffAfter = i_poweroff;
		else {
			kconfig->setGroup("default-scheme");
			i_poweroff = kconfig->readNumEntry("powerOffAfter", -1);
			if(i_poweroff >= 0) {
				powerOffAfter = i_poweroff;
			}
			else powerOffAfter = 0;
			// reset the group
			kconfig->setGroup(schemeName);
		}
		
		brightness = kconfig->readBoolEntry("enableBrightness",false);
		brightnessValue = kconfig->readNumEntry("brightnessPercent", -1);
		if (brightnessValue == -1) {
			kconfig->setGroup("default-scheme");
			brightnessValue = kconfig->readNumEntry("brightnessPercent", 100);
			// reset the group
			kconfig->setGroup(schemeName);
		}

		int i_autoInactiveActionAfter = kconfig->readNumEntry("autoInactiveActionAfter", -1);
		if (i_autoInactiveActionAfter >= 0) autoInactiveActionAfter = i_autoInactiveActionAfter;
		else {
			kconfig->setGroup("default-scheme");
			i_autoInactiveActionAfter = kconfig->readNumEntry("autoInactiveActionAfter", -1);
			if(i_autoInactiveActionAfter >= 0) {
				autoInactiveActionAfter = i_autoInactiveActionAfter;
			}
			else autoInactiveActionAfter = 0;
			// reset the group
			kconfig->setGroup(schemeName);
		}
		
		QString _autoInactiveAction = kconfig->readEntry("autoInactiveAction", "NULL");
		if( _autoInactiveAction != "NULL") {
			autoInactiveAction = _autoInactiveAction;
		}
		else {
			kconfig->setGroup("default-scheme");
			_autoInactiveAction = kconfig->readEntry("autoInactiveAction", "NULL");
			if(_autoInactiveAction != "NULL") autoInactiveAction = _autoInactiveAction;
			else autoInactiveAction = "_NONE_";
			// reset the group
			kconfig->setGroup(schemeName);
		}
		
		autoSuspend = kconfig->readBoolEntry("autoSuspend",false);
		autoInactiveSBlistEnabled = kconfig->readBoolEntry("autoInactiveSchemeBlacklistEnabled",false);
		autoInactiveSBlist = kconfig->readListEntry("autoInactiveSchemeBlacklist", ',');
		
		int i_autoDimmAfter = kconfig->readNumEntry("autoDimmAfter", -1);
		if (i_autoDimmAfter >= 0) autoDimmAfter = i_autoDimmAfter;
		else {
			kconfig->setGroup("default-scheme");
			i_autoDimmAfter = kconfig->readNumEntry("autoDimmAfter", -1);
			if(i_autoDimmAfter >= 0) {
				autoDimmAfter = i_autoDimmAfter;
			}
			else autoDimmAfter = 0;
			// reset the group
			kconfig->setGroup(schemeName);
		}

		int i_autoDimmTo = kconfig->readNumEntry("autoDimmTo", -1);
		if (i_autoDimmTo >= 0) autoDimmTo = i_autoDimmTo;
		else {
			kconfig->setGroup("default-scheme");
			i_autoDimmTo = kconfig->readNumEntry("autoDimmAfter", -1);
			if(i_autoDimmTo >= 0) {
				autoDimmTo = i_autoDimmTo;
			}
			else autoDimmTo = 0;
			// reset the group
			kconfig->setGroup(schemeName);
		}

		autoDimm = kconfig->readBoolEntry("autoDimm",false);
		autoDimmSBlistEnabled = kconfig->readBoolEntry("autoDimmSchemeBlacklistEnabled",false);
		autoDimmSBlist = kconfig->readListEntry("autoDimmSchemeBlacklist", ',');

		disableNotifications = kconfig->readBoolEntry("disableNotifications",false);
		
		QString _cpufreqpolicy =  kconfig->readEntry("cpuFreqPolicy", "NULL");
		if( _cpufreqpolicy == "NULL") {
			kconfig->setGroup("default-scheme");
			_cpufreqpolicy = kconfig->readEntry("cpuFreqPolicy", "NULL");
			// reset the group
			kconfig->setGroup(schemeName);
		} 
		if (_cpufreqpolicy.startsWith("DYNAMIC")) {
			cpuFreqPolicy = DYNAMIC;
		} else if (_cpufreqpolicy.startsWith("PERFORMANCE")) {
			cpuFreqPolicy = PERFORMANCE;
		} else if (_cpufreqpolicy.startsWith("POWERSAVE")) {
			cpuFreqPolicy = POWERSAVE;
		} else {
			// set as default
			cpuFreqPolicy = DYNAMIC;
		}

		cpuFreqDynamicPerformance = kconfig->readNumEntry("cpuFreqDynamicPerformance", -1);
		if( cpuFreqDynamicPerformance == -1) {
			kconfig->setGroup("default-scheme");
			cpuFreqDynamicPerformance = kconfig->readNumEntry("cpuFreqDynamicPerformance", 51);
			// reset the group
			kconfig->setGroup(schemeName);
		} 

		return true;
	}
	else return false;
}


/*!
 * Loads the general settings from kpowersaverc and fills the related variables.
 * \return the result of the load
 * \retval true  if the settings could be loaded
 * \retval false if there was no group named 'General'
 */
bool Settings::load_general_settings(){
	
	kconfig->reparseConfiguration();

	if(kconfig->hasGroup("General")) {
		kconfig->setGroup("General");
		
		lockOnSuspend = kconfig->readBoolEntry("lockOnSuspend",true);
		lockOnLidClose = kconfig->readBoolEntry("lockOnLidClose",true);
		autostart = kconfig->readBoolEntry("Autostart",false);
		autostartNeverAsk = kconfig->readBoolEntry("AutostartNeverAsk",false);
		psMsgAsPassivePopup = kconfig->readBoolEntry("psMsgAsPassivePopup",false);
		forceDpmsOffOnLidClose = kconfig->readBoolEntry("forceDpmsOffOnLidClose",false);
		unmountExternalOnSuspend = kconfig->readBoolEntry("unmountExternalOnSuspend",true);
		callSetPowerSaveOnAC = kconfig->readBoolEntry("callSetPowerSaveOnAC",true);

		lockmethod = kconfig->readEntry("lockMethod", "NULL");
		if(lockmethod == "NULL") lockmethod = "automatic";
	
		autoInactiveGBlist = kconfig->readListEntry("autoInactiveBlacklist", ',');
		autoDimmGBlist = kconfig->readListEntry("autoDimmBlacklist", ',');
		
		autoSuspendCountdown = kconfig->readBoolEntry("AutoSuspendCountdown", false);
		autoSuspendCountdownTimeout = kconfig->readNumEntry("AutoSuspendCountdownTimeOut", 30);

		timeToFakeKeyAfterLock = kconfig->readNumEntry("timeToFakeKeyAfterLock", 5000);

		schemes = kconfig->readListEntry("schemes", ',');
		ac_scheme = kconfig->readEntry("ac_scheme", "Performance");
		battery_scheme = kconfig->readEntry("battery_scheme", "Powersave");

		// Read battery levels and related actions
		batteryWarningLevel = kconfig->readNumEntry("batteryWarning", 12);
		batteryLowLevel = kconfig->readNumEntry("batteryLow", 7);
		batteryCriticalLevel = kconfig->readNumEntry("batteryCritical", 2);

		batteryWarningLevelAction = mapActionToType(kconfig->readEntry("batteryWarningAction",""));
		if (batteryWarningLevelAction == BRIGHTNESS) {
			batteryWarningLevelActionValue = kconfig->readNumEntry("batteryWarningActionValue", -1);
		}
		batteryLowLevelAction = mapActionToType(kconfig->readEntry("batteryLowAction",""));
		if (batteryLowLevelAction == BRIGHTNESS) {
			batteryLowLevelActionValue = kconfig->readNumEntry("batteryLowActionValue", -1);
		}
		batteryCriticalLevelAction = mapActionToType(kconfig->readEntry("batteryCriticalAction",""));
		if (batteryCriticalLevelAction == BRIGHTNESS) {
			batteryCriticalLevelActionValue = kconfig->readNumEntry("batteryCriticalActionValue", -1);
		}

		lidcloseAction = mapActionToType(kconfig->readEntry("ActionOnLidClose",""));
		if (lidcloseAction == BRIGHTNESS) {
			lidcloseActionValue =  kconfig->readNumEntry("ActionOnLidCloseValue", -1);
		}
		// avoid logout dialog since this make no sence with lidclose
		if (lidcloseAction == LOGOUT_DIALOG) {
			lidcloseAction = NONE;
		}

		powerButtonAction = mapActionToType(kconfig->readEntry("ActionOnPowerButton",""));
		if (powerButtonAction == BRIGHTNESS) {
			powerButtonActionValue =  kconfig->readNumEntry("ActionOnPowerButtonValue", -1);
		}

		sleepButtonAction = mapActionToType(kconfig->readEntry("ActionOnSleepButton",""));
		if ((sleepButtonAction != GO_SUSPEND2RAM) && (sleepButtonAction != GO_SUSPEND2DISK)) {
			sleepButtonAction = NONE;
		}

		s2diskButtonAction = mapActionToType(kconfig->readEntry("ActionOnS2DiskButton",""));
		if ((s2diskButtonAction != GO_SUSPEND2RAM) && (s2diskButtonAction != GO_SUSPEND2DISK)) {
			s2diskButtonAction = NONE;
		}

		return true;
	}
	else return false;
}


/*!
 * Map the string value from the config file to the type from \ref action
 * \param _action	a QString to map
 * \return a integer value with the result of the mapping as \ref action
 */
action Settings::mapActionToType (QString _action) {
	
	if (_action.isEmpty()) {
		return NONE;
	} else if (_action.startsWith("SHUTDOWN")) {
		return GO_SHUTDOWN;
	} else if (_action.startsWith("LOGOUT_DIALOG")) {
		return LOGOUT_DIALOG;
	} else if (_action.startsWith("SUSPEND2DISK")) {
		return GO_SUSPEND2DISK;
	} else if (_action.startsWith("SUSPEND2RAM")) {
		return GO_SUSPEND2RAM;
	} else if (_action.startsWith("CPUFREQ_POWERSAVE")) {
		return CPUFREQ_POWERSAVE;
	} else if (_action.startsWith("CPUFREQ_DYNAMIC")) {
		return CPUFREQ_DYNAMIC;
	} else if (_action.startsWith("CPUFREQ_PERFORMANCE")) {
		return CPUFREQ_PERFORMANCE;
	} else if (_action.startsWith("BRIGHTNESS")) {
		return BRIGHTNESS;
	} else {
		return UNKNOWN_ACTION; 
	}
}

/*!
 * Loads the default KDE Settings from the different configfiles and store
 * them to a \ref KDE_Settings 'object'.
 * \retval true  if the settings could be loaded
 * \retval false if there was a error/problem 
 */
void Settings::load_kde(){
	KConfig *_kconfig = new KConfig("kcmdisplayrc", true );
	
	/* KDE settings [DisplayEnergy] from kcmdisplayrc */
	if(_kconfig->hasGroup("DisplayEnergy")) {
		_kconfig->setGroup("DisplayEnergy");
		kde->displayEnergySaving = _kconfig->readBoolEntry("displayEnergySaving", true);
		kde->displayStandby = _kconfig->readNumEntry("displayStandby", 7);
		kde->displaySuspend = _kconfig->readNumEntry("displaySuspend", 13);
		kde->displayPowerOff = _kconfig->readNumEntry("displayPowerOff", 19);
	}
        delete _kconfig;	
	_kconfig = new KConfig("kdesktoprc", true );
	/* KDE settings [ScreenSaver] from kdesktoprc */
	if(_kconfig->hasGroup("ScreenSaver")) {
		_kconfig->setGroup("ScreenSaver");
		kde->enabled = _kconfig->readBoolEntry("Enabled", true);
		kde->lock = _kconfig->readBoolEntry("Lock", true);

		QString _savername = _kconfig->readEntry("Saver", "KBlankscreen.desktop");
		if (_savername.startsWith("KBlankscreen.desktop"))
			kde->blanked = true;
		else 
			kde->blanked = false;
	}
	delete _kconfig;
}

