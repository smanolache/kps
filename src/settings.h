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
*  \file 	settings.h
*  \brief 	Headerfile for settings.cpp and the class \ref Settings.
*/

#ifndef SETTINGS_H
#define SETTINGS_H

// KDE - Header
#include <kconfig.h>

// QT - Header
#include <qstring.h>
#include <qstringlist.h>

// own headers
#include "hardware.h"

enum action{
	UNKNOWN_ACTION = -2,
	NONE = -1,
	GO_SHUTDOWN,
	LOGOUT_DIALOG,
	GO_SUSPEND2RAM,
	GO_SUSPEND2DISK,
	SWITCH_SCHEME,
	BRIGHTNESS,
	CPUFREQ_POWERSAVE,
	CPUFREQ_DYNAMIC,
	CPUFREQ_PERFORMANCE
};

/*! 
*  \class 	KDE_Settings
*  \brief 	class/object for the KDE default settings
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2005
*/
class KDE_Settings {

public:
	/* KDE settings [DisplayEnergy] */
	//! if KDE enable DPMS
	/*!
	 * This boolean tells if KDE enable DPMS by global settings.
	 * \li true:  if KDE enable DPMS
	 * \li false: if not
	 */
	bool displayEnergySaving;
	//! time for DPMS standby
	/*!
	 * This integer represent the time in minutes after which the 
	 * display should do to stand-by.
	 */
	int displayStandby;
	//! time for DPMS suspend 
	/*!
	 * This integer represent the time in minutes after which the 
	 * display should suspend.
	 */
	int displaySuspend;
	//! time for DPMS power-off
	/*!
	 * This integer represent the time in minutes after which the 
	 * display should power off.
	 */
	int displayPowerOff;
	
	/* KDE settings [ScreenSaver] */
	//! if the KDE screensaver is enabled
	/*!
	 * This boolean tells if the KDE screensaver is enabled.
	 * \li true:  if screensaver enabled
	 * \li false: if screensaver disabled
	 */
	bool enabled;
	//! if KDE should lock the screen
	/*!
	 * This boolean tells if KDE lock the screen.
	 * \li true:  if lock the screen
	 * \li false: if not
	 */
	bool lock;

	//! if KDE already only blank the screensaver
	/*!
	 * This boolean tells if KDE already only use the blank screensaver
	 * \li true:  if blank screensaver
	 * \li false: if else
	 */
	bool blanked;
	
};

/*! 
*  \class 	Settings
*  \brief 	class for the Settings ( read ) related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2005
*/
class Settings{

public:

	//! default constructor
	Settings();
	//! default destructor
	virtual ~Settings();
	
	//! a instance of the KDE global settings.
	KDE_Settings *kde;
	
	/* START ************************** General settings *******************************/
	//! name of the default "onAC" scheme
	QString ac_scheme;
	//! name of the default "on Battery" scheme
	QString battery_scheme;
	//! a list with the names of the currently available schemes.
	/*!
	 * List with the names of the currently configured schemes. The name of the 
	 * schemes are the same as we use for the related selection in the configure file.
	 * Note: Be sure that the name for the default schemes is not translated !!!
	 */
	QStringList schemes;

	//! the name of the lock method
	/*!
	 * This QString contains a alias to the selected method for lock screen. Possible values:
	 * \li automatic for automatically selected (sequence: kscreensaver, xscreensaver, xlock)
	 * \li kscreensaver for KDE KScreensaver
	 * \li xscreensaver for XScreensaver (default used on GNOME)
	 * \li xlock for xlock
	 */
	QString lockmethod;

	//! if the messages from powersave should popup as Kpassivepopup
	/*!
	 * This boolean tells if the messages from powersave should be displayed
	 * as KPassivePopup or as KMessageBox::error 
	 * \li true:  if use KPassivePopup
	 * \li false: if not 
	 */
	bool psMsgAsPassivePopup;
	
	//! if the screen should be locked on suspend
	/*!
	 * This boolean tells if the screen should be locked befor a suspend or standby.
	 * \li true:  if the screen should be locked 
	 * \li false: if not lock the screen
	 */
	bool lockOnSuspend;
	//! if the screen should be locked on 'lid close' event
	/*!
	 * This boolean tells if the screen should be locked on a 'Lid close' event.
	 * \li true:  if the screen should be locked
	 * \li false: if not lock the screen
	 */
	bool lockOnLidClose;
	//! if kpowersave starts on login
	/*!
	 * This boolean tells if kpowersave should be start automatically on userlogin.
	 * The value could be changed trough the configuration dialog and if the user 
	 * stop kpowersave.
	 * \li true:  if autostart
	 * \li false: if not start on login
	 */
	bool autostart;
	//! if kpowersave ask for the user for autostart
	/*!
	 * This boolean tells if kpowersave ask for autostart settings if user stop kpowersave.
	 * \li true:  if ask on stop
	 * \li false: if never ask 
	 */
	bool autostartNeverAsk;
	//! if kpowersave should force DPMS shutdown for display for lidclose
	/*!
	 * This boolean tells if kpowersave should force dpms shutdown for the display if a 
	 * lidclose event was recieved
	 * \li true:  if shutdown display
	 * \li false: if not
	 */
	bool forceDpmsOffOnLidClose;
	//! if kpowersave should call SetPowerSave() on HAL
	/*!
	 * This boolean tells if kpowersave should call SetPowerSave on HAL depending
	 * on the AC state.
	 * \li true:  if should call
	 * \li false: if not
	 */
	bool callSetPowerSaveOnAC;

	//! time after resume to fake keyevent
	/*!
	 * This integere contains the time after wich kpowersave should fake a keyevent
	 * to show the login dialog if the desktop was locked on suspend. Value is in msec.
	 */
	int timeToFakeKeyAfterLock;
	
	//! percentag value of battery level for warning state
	/*!
	 * This integer represent the remaining percentag of the battery 
	 * where we reach the battery warning level. This is a value between 0 and 100.
	 */
	int batteryWarningLevel;
	//! percentag value of battery level for low state
	/*!
	 * This integer represent the remaining percentag of the battery 
	 * where we reach the battery low level. This is a value between 0 and 100 and
	 * should be lower than \ref batteryWarningLevel and higher than \ref batteryCriticalLevel
	 */
	int batteryLowLevel;
	//! percentag value of battery level for critical state
	/*!
	 * This integer represent the remaining percentag of the battery 
	 * where we reach the battery critical level. This is a value between 0 and 100 and
	 * should be lower than \ref batteryLowLevel and higher than 0
	 */
	int batteryCriticalLevel;
	//! the action that should be called if the warning level is reached
	action batteryWarningLevelAction;
	//! to \ref batteryWarningLevelAction related value
	int batteryWarningLevelActionValue;
	//! the action that should be called if the low level is reached
	action batteryLowLevelAction;
	//! to \ref batteryLowLevelAction related value
	int batteryLowLevelActionValue;
	//! the action that should be called if the critical level is reached
	action batteryCriticalLevelAction;
	//! to \ref batteryCriticalLevelAction related value
	int batteryCriticalLevelActionValue;

	//! the action that should be called if the lid closed
	action lidcloseAction;
	//! to \ref lidcloseAction related value
	int lidcloseActionValue;

	//! the action that should be called if the power button get pressed
	action powerButtonAction;
	//! to \ref powerButtonAction related value
	int powerButtonActionValue;

	//! the action that should be called if the suspend2ram/sleep button get pressed
	action sleepButtonAction;
	//! the action that should be called if the suspend2disk button get pressed
	action s2diskButtonAction;

	/* END   ************************** General settings *******************************/
	/* START ************************ Scheme settings and values ***********************/

	//! name of the scheme representing the current settings
	QString currentScheme;
	
	// ---------- Screensaver/DPMS section ------------ //
	//! if kpowersave use own screensaver settings
	/*!
	 * This boolean represent kpowersave userspecific screensaver settings. If kpowersave
	 * use own screensaver settings the KDE or GNOME settings are overwritten.
	 * \li true:  if kpowersave use own settings
	 * \li false: if kpowersave don't change any screensaver settings
	 */
	bool specSsSettings;
	//! if kpowersave should disable the screensaver
	/*!
	 * This boolean tells if kpowersave should disable the screensaver.
	 * \li true:  if kpowersave should disable the screensaver
	 * \li false: if not
	 */
	bool disableSs;
	//! if kpowersave should blank only the screen
	/*!
	 * This boolean tells if kpowersave should blank only the screen instead of
	 * using the global selected KDE or GNOME screensaver.
	 * \li true:  if kpowersave should blank only the screen
	 * \li false: if not and don't change anything
	 */
	bool blankSs;
	//! if kpowersave use own DPMS settings
	/*!
	 * This boolean tells if kpowersave should use own userspecific settings for
	 * Display PowerManagement Settings. If this value is true kpowersave overwritte 
	 * the KDE or GNOME global settings.
	 * \li true:  if kpowersave use own DPMS settings
	 * \li false: if kpowersave don't change DPMS settings
	 */
	bool specPMSettings;
	//! if kpowersave should disable DPMS
	/*!
	 * This boolean tells if kpowersave should disable DPMS. If this is used,
	 * kpowersave overwrite the KDE or GNOME global settings.
	 * \li true:  if kpowersave should disable dpms
	 * \li false: if not
	 */
	bool disableDPMS;
	//! time for DPMS standby
	/*!
	 * This integer represent the time in minutes after which the 
	 * display should do to stand-by.
	 */
	int standbyAfter;
	//! time for DPMS suspend
	/*!
	 * This integer represent the time in minutes after which the 
	 * display should suspend.
	 */
	int suspendAfter;
	//! time for DPMS power-off
	/*!
	 * This integer represent the time in minutes after which the 
	 * display should power off.
	 */
	int powerOffAfter;

	// ------------ Brightness section ---------------- //
	//! if brightness is enabled for the current scheme
	/*!
	 * This boolean tells if brightness is enabled for the 
	 * current scheme
	 * \li true:  if brightness is enabled
	 * \li false: if not
	 */
	bool brightness;
	//! the value for the brighness
	/*!
	 * This integer represent the value to which the brigthness 
	 * should be set. This value is in percentage.
	 */
	int brightnessValue;
	
	// ------------ Autosuspend section ---------------- //
	//! if autosuspend is enabled for the current scheme
	/*!
	 * This boolean tells if autosuspend is enabled for the 
	 * current scheme
	 * \li true:  if autosuspend is enabled
	 * \li false: if not
	 */
	bool autoSuspend;
	//! if scheme specific blacklist is enabled
	/*!
	 * This boolean tells if a scheme specific blacklist
	 * ( autoInactiveSchemeBlacklist ) should be used. 
	 * \li true:  if use scheme specific blacklist
	 * \li false: if not
	 */
	bool autoInactiveSBlistEnabled;
	//! time of user inactivity to execute a defined action
	/*!
	 * This integer represent the time in minutes after which kpowersave
	 * should execute a specific through \ref autoInactiveAction defined
	 * action.
	 */
	int autoInactiveActionAfter;
	//! action which execute after a defined time of inactivity 
	/*!
	 * This QString contains the action/command which should be execute
	 * after a trough \ref autoInactiveActionAfter defined time. If nothing
	 * should happens this QString is empty or '_NONE_'
	 */
	QString autoInactiveAction;
	//! general list with running programs which prevent the autosuspend
	/*!
	 * This QStringList contains names of programs which prevent, if one of
	 * them is running/active the autossuspend. 
	 */ 
	QStringList autoInactiveGBlist;
	//! scheme list with running programs which prevent the autosuspend
	/*!
	 * This QStringList contains names of programs which prevent, if one of
	 * them is running/active the autossuspend. 
	 */ 
	QStringList autoInactiveSBlist;

	// ------ Autosuspend countdown dialog section -------- //
	//! if KPowersave should display a dialog with a warning for autosuspend
	/*!
	 * This boolean tells if KPowersave should show a warning dialog
	 * with a countdown before call the autosuspend.
	 * \li true:  if show dialog
	 * \li false: if not
	 */
	bool autoSuspendCountdown;
	//! how long the autosuspend warn dialog should be shown
	/*!
	 * This integer represent the time in seconds how long the autosuspend
	 * warning dialog should be shown. This time get stripped from 
	 * \ref autoInactiveActionAfter to be sure the suspend get called within
	 * the expected time.
	 */
	int autoSuspendCountdownTimeout;
	
	// -------------- Autodimm section ------------------- //
	//! if autodimm is enabled for the current scheme
	/*!
	 * This boolean tells if autodimm is enabled for the current scheme
	 * \li true:  if autosuspend is enabled
	 * \li false: if not
	 */
	bool autoDimm;
	//! if a scheme specific autodimm blacklist is enabled
	/*!
	 * This boolean tells if a scheme specific autdimm blacklist
	 * ( autoDimmSBlist ) should be used. 
	 * \li true:  if use scheme specific blacklist
	 * \li false: if not
	 */
	bool autoDimmSBlistEnabled;
	//! time of user inactivity to dimm the display
	/*!
	 * This integer represent the time in minutes after which kpowersave
	 * should dimm the display to the level defined by \ref autoDimmTo .
	 */
	int autoDimmAfter;
	//! percentage to which the display should dimmed to
	/*!
	 * This integer represent the brightness percentage to which the 
	 * display should get dimmed if the user is the via \ref autoDimmAfter
	 * defined time inactivit;
	 */
	int autoDimmTo;
	//! general list with running programs which prevent the autodimm
	/*!
	 * This QStringList contains names of programs which prevent, if one of
	 * them is running/active the autodimm of the display panel. 
	 */ 
	QStringList autoDimmGBlist;
	//! scheme specific list with running programs which prevent the autodimm
	/*!
	 * This QStringList contains names of programs which prevent, if one of
	 * them is running/active the autodimm of the display panel. This list is 
	 * scheme specific.
	 */ 
	QStringList autoDimmSBlist;
	

	// ------------ CPU Frequency section ---------------- //
	//! represent the CPU Frequency policy to set
	cpufreq_type cpuFreqPolicy;
	//! represent the performance level (how triggerhappy) for dynamic cpu freq policy
	int cpuFreqDynamicPerformance;

	// --------------- misc section -------------------- //
	//! if scheme disabled notifications
	/*!
	 * This boolean tells if a scheme should disable notifications.
	 * \li true:  if disable notifications
	 * \li false: if not
	 */
	bool disableNotifications;

	//! if umount/remount external volumes on suspend
	/*!
	 * This boolean tells if KPowersave should umount external partitions
	 * before suspend and remount them after resume.
	 * \li true:  if umount/remount
	 * \li false: if not
	 */
	bool unmountExternalOnSuspend;

	/* END ************************ Scheme settings and values ******************************/
	
	/******************* Functions ********************/	
	//! to load the general settings
	bool load_general_settings();
	//! to load the global KDE settings
	void load_kde();
	//! to load settings of a specific scheme
	bool load_scheme_settings(QString);
	
private:
	
	//! configuration of kpowersave
	KConfig *kconfig;

	//! to map a battery action string to the related type
	action mapActionToType (QString _action);
};
#endif
