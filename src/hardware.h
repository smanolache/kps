 /**************************************************************************
 *   Copyright (C) 2006-2007 by Danny Kukawka                              *
 *                              <dkukawka@suse.de>, <danny.kukawka@web.de> *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
/*! 
*  \file 	hardware.h
*  \brief 	Headerfile for hardwareinfo.cpp. This file collect/hold all
*		Hardware information as e.g. battery and ac state. 
*/

/*! 
*  \class 	HardwareInfo
*  \brief 	class for hardware information related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2006-2007
*/

#ifndef _HARDWARE_H_
#define _HARDWARE_H_

// Global Header
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// QT - Header
#include <qstring.h>
#include <qobject.h>
#include <qvaluelist.h>
#include <qptrlist.h>
#include <qdict.h>

#include "dbusHAL.h"
#include "hardware_battery.h"
#include "hardware_batteryCollection.h"
#include <string>

#include "dbus_properties.hpp"

enum suspend_type {
	SUSPEND2DISK,
	SUSPEND2RAM,
	STANDBY
};

enum cpufreq_type {
	UNKNOWN_CPUFREQ = -1,
	PERFORMANCE,
	DYNAMIC,
	POWERSAVE
};

enum device_type {
	BATTERY,
	AC_ADAPTER,
	BUTTON_SLEEP,
	BUTTON_POWER,
	LID,
	LAPTOP_PANEL,
	UNKNOWN_DEVICE
};

enum ERROR_MSG {
	DBUS_NO_RIGHTS, 
	DBUS_NOT_RUNNING,
	DBUS_RUNNING
};

//! hold information if suspend/standby/pm actions are supported and allowed
/*!
* This dictionary contains information about the available pm capabilities and
* the related interfaces in HAL.
*/
typedef struct SuspendStates {
	//! true if the machine support suspend2ram and the interface is available
	bool suspend2ram;
	//! true if the machine support suspend2ram, but no interface available
	bool suspend2ram_can;
	//! true if the machine support suspend2ram and PolicyKit allow to call the interface
	int suspend2ram_allowed;
	//! true if the machine support suspend2disk and the interface is available
	bool suspend2disk;
	//! true if the machine support suspend2disk, but no interface available
	bool suspend2disk_can;
	//! true if the machine support suspend2disk and PolicyKit allow to call the interface
	int suspend2disk_allowed;
	//! true if the machine support standby and the interface is available
	bool standby;
	//! true if the machine support standby, but no interface available
	bool standby_can;
	//! true if the machine support standby and PolicyKit allow to call the interface
	int standby_allowed;

	SuspendStates () { 
		suspend2ram = false;
		suspend2ram_can = false;
		suspend2ram_allowed = -1;
		suspend2disk = false;
		suspend2disk_can = false;
		suspend2disk_allowed = -1;
		standby = false;
		standby_can = false;
		standby_allowed = -1;
	}
} SuspendStates;

class HardwareInfo : public QObject{

	Q_OBJECT

private: 
	//! pointer to the dbusHAL connection class
	dbusHAL *dbus_HAL;

	//! hold udis of special hardware execpt batteries represented by a QString pairs (name,udi)
	/*!
	* This directory handle udis for known fixed devices as e.g ac adapter. This devices are
	* currently available:
	* \li acadapter
	* \li lidclose
	* \li laptop_panel
	*/
	QDict<QString> udis;

	//! hold the UDIs of all hardware we handle atm
	/*!
	 * This QStringList contains the list of UDIs we handle at the moment in this
	 * class. This should be used to handle device events from HAL for devices we
	 * want to monitor
	 */
	QStringList allUDIs;

	//! hold information if suspend/standby/pm actions are supported and allowed
	/*!
	* This dictionary contains information about the available pm capabilities and
	* the related interfaces in HAL.
	*/
	SuspendStates suspend_states;

	QTime calledSuspend;

	//! hold the list of pointers to all relevant batteries
	QPtrList<Battery> BatteryList;
	//! hold the information about all primary batteries
	BatteryCollection *primaryBatteries;

	//! hold the name of the CPU Freq governor from the last check
	std::string cpuFreqGovernor;

	//! hold the ConsoleKit name/path of the actual session
	std::string consoleKitSession;

	//! enum with the currently active CPU Freq policy
	/*! This enum contains the enum with the currently set CPU Freq Policy. */
	cpufreq_type currentCPUFreqPolicy;

	//! the state of the ac adapter
	/*! 
	* This boolean represent information about the AC adapter state.
	* \li true: 	if AC adapter is present
	* \li false: 	it AC adapter is not present
	*/
	bool acadapter;
	//! the state of the lidclose button
	/*! 
	* This boolean represent information about the Lid button state.
	* \li true: 	if lid is closed 
	* \li false: 	else
	*/
	bool lidclose;
	//! if the machine support APM
	/*! 
	* This boolean represent information if the machine support APM or not.
	* \li true: 	if APM supported
	* \li false: 	else
	*/
	bool has_APM;
	//! if the machine support ACPI
	/*! 
	* This boolean represent information if the machine support ACPI or not.
	* \li true: 	if ACPI supported
	* \li false: 	else
	*/
	bool has_ACPI;
	//! if the machine support PMU (ppc power management)
	/*! 
	* This boolean represent information if the machine support PMU or not.
	* \li true: 	if PMU supported
	* \li false: 	else
	*/
	bool has_PMU;
	//! if the machine support change CPU Freq via HAL interface
	/*! 
	* This boolean represent information if the machine support change the 
	* CPU freqency via HAL.
	* \li true: 	if supported
	* \li false: 	else
	*/
	bool cpuFreq;
	//! if the machine support change *SchedPowerSavings methodes via HAL interface
	/*! 
	* This boolean represent information if the machine support change the 
	* SchedPowerSavings methodes via HAL.
	* \li true: 	if supported
	* \li false: 	else
	*/
	bool schedPowerSavings;
	//! if the machine support change brightness
	/*! 
	* This boolean represent information if the machine support brightness changes.
	* \li true: 	if supported
	* \li false: 	else
	*/
	bool brightness;
	//! if brightness get controled via keyevents in hardware
	/*! 
	* This boolean represent information if the machine handle brightness button
	* and keyevents in hardware. If so KPowersave should ignore key events.
	* \li true: 	if handled in hardware
	* \li false: 	else
	*/
	bool brightness_in_hardware;
	//! if the machine is a laptop
	/*! 
	* This boolean represent information if the machine is a laptop.
	* \li true: 	if the machine is a laptop
	* \li false: 	else
	*/
	bool laptop;

	//! if the current desktop session is active
	/*! 
	* This boolean represent information if the current desktop session in
        * Which KPowersave runs is marked in ConsoleKit as active or not.
	* \li true: 	if active
	* \li false: 	else
	*/
	bool sessionIsActive;

	//! if a battery was removed/added on the last device event
	/*! 
	* This boolean represent information if a battery was added or removed
	* on the last device event
	* \li true: 	if removed/added
	* \li false: 	else
	*/
	bool batteryRemovedAdded;
	

	//! if the current user can use the CPU Freq interface
	/*! 
	* This integer tell if the current user is allowed to change the 
	* CPU Frequency policy via the HAL interface
	* \li 1:	if allowed
	* \li 0:	if not allowed
	* \li -1:	if unknown (e.g. there is no policy/PolicyKit)
	*/
	int cpuFreqAllowed;
	//! if the current user can use the brightness interface
	/*! 
	* This integer tell if the current user is allowed to change the 
	* brightness via the HAL interface
	* \li 1:	if allowed
	* \li 0:	if not allowed
	* \li -1:	if unknown (e.g. there is no policy/PolicyKit)
	*/
	int brightnessAllowed;


	//! Integer with the number of current brightness
	/*! This contains the current brighness level. */
	int currentBrightnessLevel;
	//! Integer with the number of availabl brightness level
	/*! This contains the number of available brightness levels for internal usage. */
	int availableBrightnessLevels;

	//! interger with the current warning level
	int primaryBatteriesWarnLevel;
	//! interger with the current low level
	int primaryBatteriesLowLevel;
	//! interger with the current critical level
	int primaryBatteriesCriticalLevel;

	// --> functions
	//! check if the machine support ACPI/APM/PMU or not
	void checkPowermanagement();
	//! check the possible suspend/standby states
	void checkSuspend();
	//! check if the machine support change CPU Freq via HAL
	void checkCPUFreq();
	//! check the current brightness level
	void checkCurrentBrightness();
	//! check if the machine is a laptop
	void checkIsLaptop();

	//! initalise all hardware information
	bool intialiseHWInfo();
	//! reinit all hardware information
	bool reinitHardwareInfos();
	//! to check the current ConsoleKit session
	bool checkConsoleKitSession();
	//! to check if we should handle a device
/*
	bool checkIfHandleDevice ( QString _udi, int *type );
*/
	//! to set the CPUFreq governor
	bool setCPUFreqGovernor( const char *governor );
	//! to get the state of SchedPowerSave setting of kernel/HAL
	bool getSchedPowerSavings();
	//! to set the state of SchedPowerSave setting of kernel/HAL
	bool setSchedPowerSavings( bool enable );

	//! find and update a battery information
/*
	void updateBatteryValues (QString udi, QString property);
*/
	void updateBatteryValues(const char *, const kps::dict_type&);

	bool isBattery(const char *);
	bool isBattery(const kps::dict_type&) const;
	bool isPrimary(const kps::dict_type&) const;

	void handle_upower_changed_props(const char *,
					 const kps::modified_props_type&);
	void handle_ac_props_change(const kps::modified_props_type&);
	void signal_ac_change(const kps::dict_type&);
	void handle_battery_props_change(const char *,
					 const kps::modified_props_type&);
	void handle_display_device_props_change(const kps::modified_props_type&);

	bool add_battery(const std::string& uid, bool, const kps::dict_type&);
	bool add_ac(const std::string& uid, const kps::dict_type&);
	bool add_device(const char *udi);
	bool remove_device(const char *udi);

private slots:

	//! to fetch events from D-Bus and handle them
	void processMessage (msg_type type, QString message, QString value);
	//! to fetch events from D-Bus and handle them
	void process_changed_props(msg_type type, const char *udi,
				   const kps::modified_props_type&);
	//! to update \ref primaryBatteries
	void updatePrimaryBatteries();
	//! to set \ref update_info_primBattery_changed
	void setPrimaryBatteriesChanges();
	//! check the state of the lidclose button
	void checkLidcloseState();
	//! check if brightness change is possible
	void checkBrightness();
	
	//! SLOT to handle the reconnect to D-Bus
	void reconnectDBUS(); 

	//! SLOT to forward signal about changed battery warning state
	void emitBatteryWARNState(int type, int state);
	
	//! SLOT to handle resume and forward a signal for resume
	void handleResumeSignal(int result);

	//! SLOT to handle device remove and add events
	/* void handleDeviceRemoveAdd (); */

	//! to emit signal for power button
	void emitPowerButtonPressed();
	//! to emit signal for sleep button
	void emitSleepButtonPressed();
	//! to emit signal for s2disk button
	void emitS2diskButtonPressed();
	//! to emit signal for session state
	void emitSessionActiveState();

	//! to handle signal for brightness Up buttons/keys
	void brightnessUpPressed();
	//! to handle signal for brightness Down buttons/keys
	void brightnessDownPressed();

signals:
	//! signal for larger data changes
	void generalDataChanged();
	//! emited if the CPU Freq Policy changed
	void currentCPUFreqPolicyChanged();

	//! signal the AC adapter
	void ACStatus(bool);
	//! signal for the lidclose button
	void lidcloseStatus(bool);
	//! signal for pressed the power button
	void powerButtonPressed();
	//! signal for pressed sleep (suspend2ram) button
	void sleepButtonPressed();
	//! signal for pressed the suspend2disk (hibernate) button
	void s2diskButtonPressed();

	//  battery related signals
	//! signal if the primary battery collection changed
	void primaryBatteryChanged();
	//! signal that a battery was removed/added (only needed for power_supply case)
	void primaryBatteryAddedRemoved();
	//! signal to inform about reaching a warning state
	void batteryWARNState(int type, int state);

	// Error signals
	//! signal if the D-Bus daemon terminate and restart
	void dbusRunning(int);

	//! signal if the IsActive state of 
	void desktopSessionIsActive(bool);

	//! signal if we are back from resume
	void resumed(int success);

public:

	// update related info --> need to be reset if the data was read
	//! tells if the  CPUFreq Policy changed
	/*! 
	* This boolean represent information about CPUFreq Policy changes.
	* \li true: if something changed
	* \li false: if nothing changed (or this is reset to false if the message was consumed)
	*/
	bool update_info_cpufreq_policy_changed;
	//! tells if the AC status changed
	/*! 
	* This boolean represent information about AC status changes.
	* \li true: if something changed
	* \li false: if nothing changed (or this is reset to false if the message was consumed)
	*/
	bool update_info_ac_changed;
	//! tells if the primary battery collection changed some values
	/*! 
	* This boolean represent information about primary battery changes.
	* \li true: if something changed
	* \li false: if nothing changed (or this is reset to false if the message was consumed)
	*/
	bool update_info_primBattery_changed;

	//! boolean which tell us if the D-Bus daemon was terminated
	/*!
	* This boolean contains information if the D-Bus daemon terminated and
	* we recieved "dbus.terminate"
	* \li true:  If D-Bus terminated
	* \li false: If D-Bus not terminated
	*/
	bool dbus_terminated;

	// --> functions
	//! default constructor
	HardwareInfo();
	//! default destructor
	~HardwareInfo();

	// to get private members 
	//! get info about support of suspend/standby
	SuspendStates getSuspendSupport() const;
	//! get a pointer to the primary batteries
	BatteryCollection* getPrimaryBatteries() const;
	//! get all batteries
	QPtrList<Battery> getAllBatteries() const;
	
	//! check the currently set CPU Frequency Policy
	cpufreq_type checkCurrentCPUFreqPolicy();

	//! get max brightness level
	int getMaxBrightnessLevel() const;
	//! get current brightness level
	int getCurrentBrightnessLevel() const;
	//! get currently CPU Frequency Policy
	int getCurrentCPUFreqPolicy() const;
	//! if the user is allowed to change CPU Freq PolicyKit
	int isCpuFreqAllowed() const;

	//! if org.freedesktop.Policy.Power has a owner
	bool isPolicyPowerIfaceOwned();

	//! get state of the AC adapter
	bool getAcAdapter() const;
	//! get the state of the lid button
	bool getLidclose() const;
	//! check if the machine is a latop
	bool isLaptop() const;
	//! check if there is a connection to D-Bus _and_ HAL 
	bool isOnline() const;
	//! check if the machine support ACPI
	bool hasACPI() const;
	//! check if the machine support APM
	bool hasAPM() const;
	//! check if the machine support PMU
	bool hasPMU() const;
	//! check if the machine support change the CPU frequency
	bool supportCPUFreq() const;
	//! check if the machine support brightness changes
	bool supportBrightness() const;
	//! check if the current session is active
	bool currentSessionIsActive() const;

	// --> functions to call a HAL interface and trigger an action 
	//! execute/trigger a suspend via the HAL interface
	bool suspend(suspend_type suspend);
	//! set the brightness via HAL interface	
	bool setBrightness(int level, int percent = -1);
	//! to set the brightness down
	bool setBrightnessDown(int percentageStep = -1);
	//! to set the brightness up
	bool setBrightnessUp(int percentageStep = -1);
	//! set the CPU frequency policy/speed
	bool setCPUFreq(cpufreq_type cpufreq, int limit = 51 );
	//! call SetPowerSave method on HAL.
	bool setPowerSave(bool on);

	//! function to set warning states for the primary battery collection
	void setPrimaryBatteriesWarningLevel(int _warn = -1, int _low = -1,
					     int _crit = -1 );
};

#endif
