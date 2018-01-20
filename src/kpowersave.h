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

#ifndef _KPOWERSAVE_H_
#define _KPOWERSAVE_H_

// this is needed to avoid typedef clash with X11/Xmd.h (X11/Xproto.h)
#ifndef QT_CLEAN_NAMESPACE
#define QT_CLEAN_NAMESPACE
#endif

// Global Header
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// KDE - Header
#include <kprocess.h>
#include <ksystemtray.h>
#include <dcopobject.h>

// QT - Header
#include <qpopupmenu.h>

// kpowersave - Header
#include "autosuspend.h"
#include "autodimm.h"
#include "configuredialog.h"
#include "countdowndialog.h"
#include "detaileddialog.h"
#include "hardware.h"
#include "kpowersave_debug.h"
#include "logviewer.h"
#include "screen.h"
#include "settings.h"


/*! 
*  \file 	kpowersave.h
*  \brief 	Headerfile for kpowersave.cpp and the class \ref kpowersave.
*/
 /*! 
 *  \class 	kpowersave
 *  \brief 	The central class for the kpowersave-applet
 *  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 *  \author 	Thomas Renninger, <trenn@suse.de> 
 *  \date    	2004 - 2007
 */

class kpowersave : public KSystemTray, public DCOPObject
{
	Q_OBJECT
	K_DCOP

private:

	// permanent pointers
	//! to configure kpowersave
	KConfig *config;	
	//! instance of \ref screen
	screen *display;
	//! instance of \ref settings
	Settings *settings;
	//! pointer to class HardwareInfo object
	HardwareInfo *hwinfo;
	//! instance of \ref autosuspend
	autosuspend *autoSuspend;
	//! instance of \ref autodimm
	autodimm * autoDimm;

	// temporary pointer, use them if needed
	//! KProcess to start YaST-module for configuration
	KProcess *yast2;
	//! instance of \ref ConfigureDialog
	/*! the implemtation and all related functions for the configure dialog*/
	ConfigureDialog *configDlg;
	//! instance of \ref LogViewer
	LogViewer *logview;
	//! instance of \ref countDownDialog
	countDownDialog *countdown;

	
	//! struct wth information about suspend states and permissions
	SuspendStates suspend;
	//! enum with the last called suspend, this need to be reset if needed
	int calledSuspend;

	//! instance of \ref detaileddialog 
	detaileddialog *detailedDlg;

	//! represent the state of opened \ref detailedDlg
	/*!
	* This boolean represent information about the detailedDlg. Use this to inform
	* if the dialog is displayed.
	* \li true:  if dialog is displayed
	* \li false: if not
	*/
	bool detailedIsShown;
	//! represent the state of the configure dialog
	/*!
	* This boolean represent information about the configure dialog. Use this to inform
	* if the dialog is displayed.
	* \li true:  if dialog is displayed
	* \li false: if not
	*/
	bool config_dialog_shown;

	//! represent background status of the kickerapplet icon
	/*!
	* This boolean represent information about the background of the kickerapplet icon.
	* \li true:  if iconbackground is colored
	* \li false: if iconbackground is not colored
	*/
	bool icon_BG_is_colored;
	//! represent color status of the kickerapplet icon
	/*!
	* This boolean represent the state of the kickerapplet icon. It don't display 
	* information about the icon backgrund like \ref icon_BG_is_colored . It say 
	* only that the icon is in state to have a blinking background .
	* \li true:  if icon set to be colored
	* \li false: if icon isn't set to be colored
	*/
	bool icon_set_colored;
	//! represent the state of icon changing
	/*!
	* This boolean represent information about changing the icon background. It's used to
	* change the intervall of redraw the kickerapplet icon.
	* \li true:  if the state of icon is changed
	* \li false: if the state of icon isn't change 
	*/
	bool icon_state_changed;
        //! represent the state of the suspend/progress dialog
	/*!
	* This boolean represent information about the current state of
        * visible of the suspend/progress dialog.
	* \li true:  if the dialog is shown
	* \li false: if not
	*/
	bool suspend_dialog_shown;
	//! represent display state of HAL error message
	/*!
	* This boolean represent information about the current state of
        * the HAL error message
	* \li true:  if the errormessage was already displayed in this context
	* \li false: if not
	*/
	bool hal_error_shown;

	//! tell if the display should get dimmed down
	/*!
	* This boolean represent information if the display should get dimmed down
	* within the \ref do_dimm() function or not.
	* \li true:  if the display should get dimmed down
	* \li false: if the display should get dimmed up
	*/
	bool autoDimmDown;

	//! to temporary hold the resume result
	int resume_result;

	//! a menu entry ID
	/*! contains the ID of the menuentry for kpowersave configure Dialog */
	int CONFIGURE_ID;
	
	int CONFIGURE_EVENTS_ID;
	
	//! a menu entry ID 
	/*! contains the ID of the menuentry for YaST-configuration */
	int YAST_MODULE_MENU_ID;
	//! a menu seperator ID
	/*! contains the ID of the separator between YaST-entry and the sleep-states */
	int SLEEP_SEPARATOR_MENU_ID;
	//! a menu entry ID
	/*! contains the ID of the menuentry for suspend-to-disk */
	int SUSPEND2DISK_MENU_ID;
	//! a menu entry ID
	/*! contains the ID of the menuentry for suspend-to-ram */
	int SUSPEND2RAM_MENU_ID;
	//! a menu entry ID
	/*! contains the ID of the menuentry for stand-by */
	int STANDBY_MENU_ID;
	//! a menu seperator ID
	/*! contains the ID of the separator between sleep-states-entries and the cpufreq-submenu */
	int CPUFREQ_SEPARATOR_MENU_ID;
	//! a submenu ID
	/*! contains the ID of the cpufreq sub menu*/
	int CPUFREQ_MENU_ID;
	//! a menu seperator ID
	/*! contains the ID of the separator between cpufreq- and scheme-submenu*/
	int SCHEME_SEPARATOR_MENU_ID;
	//! a submenu ID
	/*! contains the ID of the scheme submenu*/
	int SCHEME_MENU_ID;
	//! a menu seperator ID
	/*! contains the ID of the separator between scheme-submenu and autosuspend*/
	int AUTOSUSPEND_SEPARATOR_MENU_ID;
	//! a menu entry ID
	/*! contains the ID of the menuentry for disable inactivity actions */
	int AUTOSUSPEND_MENU_ID;
	//! a menu seperator ID
	/*! contains the ID of the separator between autosuspend and Help*/
	int HELP_SEPARATOR_MENU_ID;
	//! a menu ID
	/*! contains the ID of the help menu*/
	int HELP_MENU;
	
	//! number of white pixel in the kickerapplet icon
	/*!
	* This integer value represent the number of white pixel in the icon for
	* the kickerapplet. with this 'global' variable we don't need to count the
	* white pixel on every redraw.
	*/
	int countWhiteIconPixel;

	//! QTimer-interval for icon background
	/*! 
	* Time intervall to set the colored background of the batteryicon on/off.
	* The value is 1000 msec/ 1 sec.
	*/
	static const int BAT_icon_BG_intervall = 1000;

	//! QTimer-interval for display HAL error message
	/*! 
	* Time intervall to delay display the HAL error message to prevent displayed
	* the message if only HAL or powersave is restarted. The value is 15000 msec/ 15 sec.
	*/
	static const int HAL_ERROR_MSG_intervall = 15000;
	
        //! type of current running suspend
        /*! QString store the name of the current running suspend*/
	QString suspendType;
	
	//! current name of the pixmap 
	/*! 
	* Contains the current name of the icon/pixmap. The value must be a filename of a existing iconfile.
	* \sa power_icon, no_battery_icon, charge_icon, battery, battery_RED or battery_ORANGE
	*/
	QString pixmap_name;

	//! contains the 'real' schemenames
	/*!
	* This list store the real schemenames (not the i18n()-version)
	* regarding to position in the scheme-menu
	*/ 
	QStringList org_schemenames;
	
	//! a sub-menu of the kickerapplet  
	/*! QPopupMenu for the cpufreq-entries. */
	QPopupMenu *speed_menu;
	//! a sub-menu of the kickerapplet
	/*! QPopupMenu for the scheme-entries. */
	QPopupMenu *scheme_menu;
	//! a sub-menu of the kickerapplet
	/*! QPopupMenu for the help-entries. */
	QPopupMenu *help_menu;


	//! icon-pixmap
	/*! QPixmap with the (pre-)loaded icon from \ref  pixmap_name .*/
	QPixmap pixmap;
	//! icon-pixmap
	/*! QPixmap with the full draw applet Icon (to be used in other classes) .*/
	QPixmap fullIcon;
	
	
	//! Timer for the blinking Icon background
	/*!
	* This timer is used to let blink the background of a icon in kicker.
	* The timerinterval is defined in \ref BAT_icon_BG_intervall .
	*/
	QTimer *BAT_WARN_ICON_Timer;
	//! Timer to delay the HAL error message
	/*!
	* This timer is used to add a delay befor display the HAL error message
	* The timerinterval is defined in \ref HAL_ERROR_MSG_intervall .
	*/
	QTimer *DISPLAY_HAL_ERROR_Timer;
	//! Timer to dimm down/up the brightness
	/*!
	 * This timer is used dimm the display up and down. The timerinterval 
         * depends on calculated timePerStep in the calling function.
	 */
	QTimer *AUTODIMM_Timer;
	
	//! draw all icon related things for \ref redrawPixmap()
	void drawIcon();
	//! to intialise the menu for the kickerapplet
        void initMenu();
	//! load the icon for \ref redrawPixmap()
	void loadIcon();
	//! draw/redraw the icon for the kickerapplet
	void redrawPixmap();
	//! to set the screensaver settings
	void setSchemeSettings();
	//! to update the Tooltip of the kickerapplet
	void updateTooltip();

	//! Eventhandler to catch mouse-press-events and react 
	void mousePressEvent( QMouseEvent *qme );
	//! Event handler to catch mouse wheel events and react
	void wheelEvent( QWheelEvent *qwe );
	//! Eventhandler to catch mouse enter events and react
	void enterEvent( QEvent *qee);

	//! to handle mount/umount on resume/suspend
	bool handleMounts ( bool suspend );

	//! to get the i18n string for a suspend type
	QString getSuspendString (int type);

private slots:

	//! send command for stand-by to the HAL daemon
	bool do_standby();
	//! send command for suspend_to_disk to the HAL daemon
	bool do_suspend2disk();
	//! send command for suspend_to_RAM to the HAL daemon
	bool do_suspend2ram();
	
	//! show warning dialog or call autosuspend if signal \ref inactivity::inactivityTimeExpired() recieved
	void do_autosuspendWarn();
	//! execute the autosuspend
	bool do_autosuspend(bool chancel);
	//! starts the Yast2-power-management module
	void do_config();
	//! sets the new scheme with all settings
	void do_setActiveScheme( int );
	//! called if icon background must be changed
	void do_setIconBG();
	//! to set the autosuspend on/off over the menu
	void do_setAutosuspend();
	//! sets the CPU Freq policy via the HAL daemon
	void do_setSpeedPolicy( int );
	//! called if there are problems with starting yast module
	void slotConfigProcessExited( KProcess * );
	//! called to open the kpowersave help
	void slotHelp();
	//! called to open the kpowersave About dialog
	void slotAbout();
	//! called to open website to report bugs
	void slotReportBug();
	//! called if the configure dialog is destroyed
	void observeConfigDlg();
	//! called if user exit from kpowersave
	void _quit();

	//! called if the user get inactive and the display should get dimmed down
	void do_downDimm();
	//! called if the user get active again and the display should get dimmed up
	void do_upDimm();
	//! SLOT do do the dimmining for autodimm feature
	void do_dimm();
	//! SLOT to set autodimm related stuff and start autodimm monitoring
	void setAutoDimm( bool resumed );

	//! to update the main menu of the kickerapplet
	/*! this is bound to generalDataChanged singal in pdaemon */
	void update();
	//! to update the scheme-menu within the main menu
	void updateSchemeMenu();
	//! to update the menu with the cpu frequency within the main menu
	void updateCPUFreqMenu();

        //! this set \ref suspendType from signal
        void setSuspendType( QString suspendtype );
	//! this set the autosuspend and start the monitoring
	void setAutoSuspend( bool );

	//! called for critical battery event SHUTDOWN
	void handleCriticalBatteryActionCall();
	//! set for a battery status the related actions
	void handleActionCall ( action action, int value, bool checkAC = false, bool batWarnCall = false );

	//! this lock/reactivate the screen if a lidcloseStatus() signal is triggered
	void handleLidEvent( bool closed );
	//! show the login dialog after locked the screen
	void activateLoginScreen();

	//! handle event for press power button and call action
	void handlePowerButtonEvent();
	//!  handle event for press s2ram/sleep button and call action
	void handleSleepButtonEvent();
	//!  handle event for press s2disk button and call action
	void handleS2DiskButtonEvent();
	
	//! handle changes of the session state
	void handleSessionState (bool state);

	//! to show the kpowersave configure_dialog
	void showConfigureDialog();
	//! to show the KNotify config dialog
	void showConfigureNotificationsDialog();

	//! this emit the KNotify event for a battery warning state state
	void notifyBatteryStatusChange ( int type, int state );
	//! this emit the KNotify event for change AC status
	void handleACStatusChange ( bool acstate, bool notifyEvent = true );
	//! this emit the KNotify events if scheme switched
	void notifySchemeSwitch();
	//! this emit the KNotify events if the machine go to suspend/Standby
	void notifySuspend( int );
	//! to independent  handleResumeSignal from event loop
	void forwardResumeSignal( int result );
	//! this emit the KNotify events if the machine resumed
	void handleResumeSignal();

	//! to display HAL error msg
	void showHalErrorMsg( );
	//! this is invoked to display powersave error message
	void showDBusErrorMsg( int );
	//! this show a blocking dialog from kpowersave with the given message
	void showErrorMessage( QString msg );

	//! this is called when detailed dialog is closed
	void closedetaileddialog();

public:

	//! default constructor
	kpowersave( bool force_acpi_check = false, bool trace_func = false);
	//! default destructor
	virtual ~kpowersave();
	
k_dcop:	
	//! dcop function to lock the screen
	bool lockScreen();
	//! dcop function to set a scheme 
	bool do_setScheme( QString );
	//! dcop function to set CPU Freq policy 
	bool do_setCPUFreqPolicy( QString );
	//! dcop function to send 'suspend to disk' command to powersaved
	bool do_suspendToDisk();
	//! dcop function to send 'suspend to RAM' command to powersaved
	bool do_suspendToRAM();
	//! dcop function to send 'standby' command to powersaved
	bool do_standBy();
	//! dcop function to set the brightness down
	bool do_brightnessDown(int percentageStep = -1);
	//! dcop function to set the brightness up
	bool do_brightnessUp(int percentageStep = -1);

	//! dcop function to disable/stop autosuspend
	void disableAutosuspend( bool );

	//! dcop function to show the detailed dialog
	void showDetailedDialog();
	//! dcop function to open the configure dialog
	bool openConfigureDialog();

        //! dcop function to find out if kpowersave manages DPMS
        bool currentSchemeManagesDPMS();
	//! dcop funtion to get the current brightness level
	int brightnessGet();

	//! dcop function to return the name of the current scheme 
	QString currentScheme ();
	//! dcop function to return the current cpufreqpolicy
	QString currentCPUFreqPolicy();
	
	//! dcop function to return the supported sleeping states
	QStringList allowed_sleepingStates();
	//! dcop function to return the schemes
	QStringList listSchemes();
	//! dcop function to return the supported CPU 
	QStringList listCPUFreqPolicies();
};

#endif // _KPOWERSAVE_H_
