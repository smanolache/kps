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
 * \file 	hardware.cpp
 * \brief 	In this file can be found the hardware information related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2006-2007
 */

// include global header
#include <fcntl.h>

// include QT header
#include <qtimer.h>
#include <qdir.h>

// include own header
#include "hardware.h"
#include "kpowersave_debug.h"
#include "privileges.h"

#include <memory>
#include "dbus_properties.hpp"
#include "UPowerProperties.hpp"
#include <fstream>
#include <unistd.h>
#include <cstdio>
#include <sys/wait.h>

using kps::modified_props_type;
using kps::dict_type;
using kps::devices_type;

/*! The default constructor of the class HardwareInfo */
HardwareInfo::HardwareInfo() {
	kdDebugFuncIn(trace);

	// init members
	acadapter = true;
	lidclose = false;
	dbus_terminated = true;
	laptop = false;
	brightness = false;
	brightness_in_hardware = false;
	schedPowerSavings = false;
	sessionIsActive = true;	 // assume as first we are active

	// update everything the first time
	update_info_ac_changed = true;
	update_info_cpufreq_policy_changed = true;
	update_info_primBattery_changed = true;
	
	currentCPUFreqPolicy = UNKNOWN_CPUFREQ;
	primaryBatteriesWarnLevel = 12;
	primaryBatteriesLowLevel = 7;
	primaryBatteriesCriticalLevel = 2;

	allUDIs = QStringList();
	consoleKitSession = "";
	BatteryList.setAutoDelete(true); // the list owns the objects
	
	primaryBatteries = new BatteryCollection(BAT_PRIMARY);
	setPrimaryBatteriesWarningLevel(); // force default settings

	// connect to D-Bus and HAL
	dbus_HAL = new dbusHAL();
	if (dbus_HAL->isConnectedToDBUS())
		dbus_terminated = false;
	else
		kdError() << "Could not connect to D-Bus." << endl;

	checkConsoleKitSession(); // initialises consoleKitSession, sessionIsActive
	checkPowermanagement(); // initialises has_ACPI, has_PMU, has_APM => they are not really used
	checkIsLaptop(); // initialises laptop
	checkBrightness(); // write to /sys/class/backlight/intel_backlight/brightness
	checkCPUFreq(); // initialises cpuFreq (bool), cpuFreqAllowed (bool), cpuFreqGovernor (ondemand, userspace, conservative, powersave, performance), and currentCPUFreqPolicy (dynamic, powersave, performance)
// /sys/devices/system/cpu0/cpufreq/*
	// getSchedPowerSavings(); 
	checkSuspend(); // initialises the suspend_states object, /sys/power/state, can be done with /org.freedesktop.login1
	intialiseHWInfo(); // initialises lidclose and adds the lid's device udi

	updatePrimaryBatteries();

	// connect to signals
	connect(dbus_HAL, SIGNAL(msgReceived_withStringString(
					 msg_type, QString, QString)),
		this, SLOT(processMessage( msg_type, QString, QString)));
	connect(dbus_HAL, SIGNAL(property_changed(
					 msg_type, const char *,
					 const kps::modified_props_type&)),
		this, SLOT(process_changed_props(
				   msg_type, const char *,
				   const kps::modified_props_type&)));
	connect(dbus_HAL, SIGNAL(backFromSuspend(int)), this,
		SLOT(handleResumeSignal(int)));

	kdDebugFuncOut(trace);
}

/*! The default desctuctor of the class HardwareInfo */
HardwareInfo::~HardwareInfo() {
	kdDebugFuncIn(trace);

	delete dbus_HAL;
	dbus_HAL = nullptr;

	kdDebugFuncOut(trace);
}

/*!
 * This funtion is used to handle the reconnect to the D-Bus daemon
 */
void HardwareInfo::reconnectDBUS() {
	kdDebugFuncIn(trace);

	if (dbus_HAL->isConnectedToDBUS()) {
		kdDebugFuncOut(trace);
		return;
	}

	if (!dbus_HAL->reconnect()) {
		if (!dbus_HAL->isConnectedToDBUS()) {
			//reconnect failed
			emit dbusRunning(DBUS_NOT_RUNNING);
			QTimer::singleShot(4000, this,
					   SLOT(reconnectDBUS()));
		} else {
			// reset everything, we are reconnected
			dbus_terminated = false;
			emit dbusRunning(DBUS_RUNNING);
		}
	} else {
		// reset everything, we are reconnected
		dbus_terminated = false;
		reinitHardwareInfos();
		// TODO Really 0?
		emit dbusRunning(0);
	}  

	kdDebugFuncOut(trace);
}

/*!
 * This funtion is used to reinit all hardware information.
 * \return 		Boolean with result of the call
 * \retval true		if reinit HW infos correct
 * \retval false	if not
 */
bool HardwareInfo::reinitHardwareInfos() {
	kdDebugFuncIn(trace);
	
	if (!dbus_HAL->isConnectedToDBUS()) {
		kdDebugFuncOut(trace);
		return false;
	}
	/* first cleanup */
	acadapter = true;
	lidclose = false;
	laptop = false;
	brightness = false;
	has_APM = false;
	has_ACPI = false;

	update_info_ac_changed = true;
	update_info_cpufreq_policy_changed = true;
	update_info_primBattery_changed = true;
	
	allUDIs = QStringList();

	BatteryList.clear();
	// this resets the primaryBatteries
	primaryBatteries->refreshInfo(BatteryList);

	// check the current desktop session again
	checkConsoleKitSession();

	/* reinit hardware data */
	checkPowermanagement();
	checkIsLaptop();
	checkBrightness();
	checkCPUFreq();
	checkSuspend();
	intialiseHWInfo();
	// getSchedPowerSavings();
	updatePrimaryBatteries();

	kdDebugFuncOut(trace);	
	return true;
}

/*!
 * This funtion is used to parse a message from D-Bus for the different 
 * messagetypes and events.
 * \param type 		a \ref msg_type which should be parse/processed
 * \param message 	the message
 * \param value 	an optional message value as e.g. message string
 */
void
HardwareInfo::processMessage(msg_type type, QString message, QString value) {
	kdDebugFuncIn(trace);

	switch(type) {
	case ACPI_EVENT:
		// we don't handle acpi events here atm
		break;
	case DBUS_EVENT:
		kdDebug() << "D-Bus Event: " << value << endl;
		if (message.startsWith("dbus.terminate")){
			dbus_terminated = true;
			QTimer::singleShot(4000, this, SLOT(reconnectDBUS()));
		}
		break;
	case UPOWER_DEVICE:
		if (0 == message.compare("DeviceAdded"))
			add_device(value.ascii());
		else if (0 == message.compare("DeviceRemoved"))
			remove_device(value.ascii());
		else {
			kdDebug() << "Unknown UPOWER_DEVICE message: "
				  << message << endl;
		}
		break;
	case CONSOLEKIT_SESSION_ACTIVE:
		if (!message.isEmpty() && !value.isEmpty()) {
			if (message.latin1() == consoleKitSession) {
				sessionIsActive = "1" == value;
				QTimer::singleShot(
					50, this,
					SLOT(emitSessionActiveState()));
			} else
				if (trace)
					kdDebug() << "CONSOLEKIT_SESSION_"
						"ACTIVE: not our session"
						  << endl;
		}
		break;
	case POLICY_POWER_OWNER_CHANGED:
		if (message.startsWith("NOW_OWNER")) {
			// TODO: add code
		} else if (message.startsWith("OTHER_OWNER")){
			// TODO: add code
		}
		break;
	default:
		kdDebug() << "Recieved unknown package type: " << type << endl;
		break;
	}

	kdDebugFuncOut(trace);
}

void
HardwareInfo::process_changed_props(msg_type type, const char *udi,
				    const modified_props_type& props) {
	kdDebugFuncIn(trace);

	switch(type) {
	case UPOWER_PROPERTY_CHANGED:
		handle_upower_changed_props(udi, props);
		break;
	default:
		kdDebug() << "Recieved unknown package type: " << type << endl;
		break;
	}

	kdDebugFuncOut(trace);
}

void
HardwareInfo::handle_upower_changed_props(const char *udi,
					  const modified_props_type& props) {
	static const char BAT_PFX[] =
		"/org/freedesktop/UPower/devices/battery_";

	kdDebugFuncIn(trace);

	if (0 == strcmp(udi, "/org/freedesktop/UPower/devices/line_power_AC0"))
		handle_ac_props_change(props);
	else if (strlen(udi) >= sizeof(BAT_PFX) &&
		 0 == strncmp(udi, BAT_PFX, sizeof(BAT_PFX) - 1))
		handle_battery_props_change(udi, props);
	else if (0 == strcmp(udi, "/org/freedesktop/UPower/devices/"
			     "DisplayDevice"))
		handle_display_device_props_change(props);
	else
		kdDebug() << "Unknown UPower device" << endl;

	kdDebugFuncOut(trace);
}

void
HardwareInfo::handle_ac_props_change(const modified_props_type& props) {
	kdDebugFuncIn(trace);

	signal_ac_change(props.modified);

	kdDebugFuncOut(trace);
}

void
HardwareInfo::signal_ac_change(const dict_type& props) {
	kdDebugFuncIn(trace);

	dict_type::const_iterator i = props.find("Online");
	if (props.end() != i) {
		bool online = std::any_cast<bool>(i->second);
		if (online != acadapter) {
			acadapter = online;
			update_info_ac_changed = true;
			emit ACStatus(acadapter);
		} else {
			update_info_ac_changed = false;
		}
	} else
		update_info_ac_changed = false;

	kdDebugFuncOut(trace);
}

void
HardwareInfo::handle_battery_props_change(const char *udi,
					  const modified_props_type& props) {
	kdDebugFuncIn(trace);

	updateBatteryValues(udi, props.modified);

	kdDebugFuncOut(trace);
}

void
HardwareInfo::handle_display_device_props_change(
	const modified_props_type& props) {
	kdDebugFuncIn(trace);

	kdDebugFuncOut(trace);
}

/*!
 * This SLOT is used to fetch the resume signal and multiplex. If needed some
 * actions after resume, do this here.
 * \param result 	integer with the result of the resume/suspend
 */
void HardwareInfo::handleResumeSignal(int result) {
	if (trace)
		kdDebug() << funcinfo <<  "IN: " << "(int result: "
			  << result << ")"<< endl;

	if (-1 == result) {
		// check if time since suspend is higher than 6 hours, 
		// the magic D-Bus timeout for pending calls
		if (calledSuspend.elapsed() > 21600000) {
			emit resumed(INT_MAX);
		}
	} else {
		emit resumed(result);
	}
	
	calledSuspend = QTime();
	kdDebugFuncOut(trace);
}

bool
HardwareInfo::add_device(const char *udi) {
	kdDebugFuncIn(trace);

	DBusConnection *conn = dbus_HAL->get_DBUS_connection();
	if (!conn) {
		kdError() << "add_device: null dbus connection" << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	dict_type props = kps::upower_get_all_props(conn, udi);
	if (!isBattery(props)) {
		kdDebug() << "add_device: " << udi << " is not a battery. "
			"Not adding." << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	bool primary = isPrimary(props);
	if (!add_battery(udi, primary, props)) {
		kdDebug() << "add_device: " << udi << " already managed. "
			"Not added." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (primary)
		updatePrimaryBatteries();
	emit primaryBatteryAddedRemoved();
	emit primaryBatteryChanged();

	kdDebugFuncOut(trace);
	return true;
}

bool
HardwareInfo::remove_device(const char *udi) {
	kdDebugFuncIn(trace);

	QStringList::iterator i = allUDIs.find(udi);
	if (allUDIs.end() == i) {
		kdDebug() << udi << " is not managed by us. Not removing."
			  << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	bool found = false;
	allUDIs.erase(i);
	for (Battery *bat = BatteryList.first(); bat; bat = BatteryList.next())
		if (0 == strcmp(udi, bat->getUdi())) {
			BatteryList.remove(bat);
			found = true;
			break;
		}

	if (found)
		primaryBatteries->refreshInfo(BatteryList, true);

	kdDebugFuncOut(trace);
	return found;
}

/*! 
 * This function checks the session for the running KPowersave instance
 * \return 		Boolean with result of operation
 * \retval true		if the query/check could get finished
 * \retval false	on every error
 */
bool HardwareInfo::checkConsoleKitSession () {
	kdDebugFuncIn(trace);

	bool retval = false;

	if (!dbus_HAL->isConnectedToDBUS()) {
		kdDebugFuncOut(trace);
		return false;
	}

	char *cookie = getenv("XDG_SESSION_COOKIE");

	if (nullptr == cookie) {
		kdDebug() << "Could not get XDG_SESSION_COOKIE from "
			"environment" << endl;
		sessionIsActive = true;
		kdDebugFuncOut(trace);
		return false;
	}

	char *reply;
	if (!dbus_HAL->dbusSystemMethodCall(CK_SERVICE, CK_MANAGER_OBJECT, 
					    CK_MANAGER_IFACE,
					    "GetSessionForCookie", 
					    &reply, DBUS_TYPE_OBJECT_PATH,
					    DBUS_TYPE_STRING, &cookie,
					    DBUS_TYPE_INVALID)) {
		kdDebugFuncOut(trace);
		return false;
	}

	if (nullptr == reply) {
		kdDebugFuncOut(trace);
		return false;
	}

	if (trace) 
		kdDebug() << "GetSessionForCookie returned: " << reply << endl;
		
	dbus_bool_t i_reply;
	consoleKitSession = reply;
	
	if (!dbus_HAL->dbusSystemMethodCall(CK_SERVICE,
					    consoleKitSession.c_str(), 
					    CK_SESSION_IFACE, "IsActive", 
					    &i_reply, DBUS_TYPE_BOOLEAN,
					    DBUS_TYPE_INVALID)) {
		kdError() << "Could get session cookie and session name, "
			"but not the status of the session. Assume for now " 
			"the Session is inactive!" << endl;
		sessionIsActive = false;
		kdDebugFuncOut(trace);
		return false;
	}

	sessionIsActive = 0 != i_reply;
	if (trace) 
		kdDebug() << "IsActive returned: " << sessionIsActive << endl;

	kdDebugFuncOut(trace);
	return retval;
}


// --> set some values for devices
/*!
 * This function set the warning level for the primary battery collection
 * If all give param are -1 or not set this function force the current 
 * settings to the primary battery collection.
 * \param _warn		value for the state BAT_WARN or -1
 * \param _low		value for the state BAT_LOW or -1
 * \param _crit		value for the state BAT_CRIT or -1
 */
void
HardwareInfo::setPrimaryBatteriesWarningLevel(int _warn, int _low, int _crit) {
	kdDebugFuncIn(trace);

	if (trace)
		kdDebug() << funcinfo << "IN: " << "warn: " << _warn
			  << " low: " << _low << " crit: " << _crit << endl;

	if (_warn > -1 && _low > -1 && _crit > -1) {
		primaryBatteriesWarnLevel = _warn;
		primaryBatteriesLowLevel = _low;
		primaryBatteriesCriticalLevel = _crit;
	}

	if (primaryBatteries) {
		primaryBatteries->setWarnLevel(primaryBatteriesWarnLevel);
		primaryBatteries->setLowLevel(primaryBatteriesLowLevel);
		primaryBatteries->setCritLevel(primaryBatteriesCriticalLevel);
		if (!BatteryList.isEmpty())
			primaryBatteries->refreshInfo(BatteryList, true);
	}

	kdDebugFuncOut(trace);
}

// --> init HW information section -- START <---

/*!
 * The function checks if the machine is a laptop.
 */
void HardwareInfo::checkIsLaptop () {
	kdDebugFuncIn(trace);

	std::ifstream f;
	f.exceptions(std::ifstream::badbit | std::ifstream::failbit);
	try {
		f.open("/sys/class/dmi/id/chassis_type");
		unsigned int n;
		f >> n;
		switch (n) {
		case 8:
		case 9:
		case 10:
		case 11:
		case 14:
			laptop = true;
			break;
		default:
			laptop = false;
			break;
		}
	} catch (const std::exception& e) {
		kdError() << "Could not open /sys/class/dmi/id/chassis_type: "
			  << e.what() << endl;
		laptop = false;
	}

	kdDebugFuncOut(trace);
}

/*!
 * The function checks wether the machine support ACPI/APM/PMU or not.
 */
void HardwareInfo::checkPowermanagement() {
	kdDebugFuncIn(trace);
	
	has_ACPI = true;

	kdDebugFuncOut(trace);
}


/*!
 * The function checks wether the machine can suspend/standby.
 */
void HardwareInfo::checkSuspend() {
	kdDebugFuncIn(trace);	

	suspend_states.standby_can = false;
	suspend_states.standby = false;
	suspend_states.standby_allowed = -1;

	char *can_suspend = nullptr;
	if (!dbus_HAL->dbusSystemMethodCall(
		    "org.freedesktop.login1", "/org/freedesktop/login1",
		    "org.freedesktop.login1.Manager", "CanSuspend",
		    &can_suspend, DBUS_TYPE_STRING, DBUS_TYPE_INVALID)) {
		kdError() << "Error querying org.freedesktop.login1.Manager."
			"CanSuspend" << endl;
		suspend_states.suspend2ram = false;
		suspend_states.suspend2ram_can = false;
		suspend_states.suspend2ram_allowed = -1;
	}
	if (can_suspend) {
		kdDebug() << "org.freedesktop.login1.Manager.CanSuspend "
			"returned " << can_suspend << '.' << endl;
		if (0 == strcmp("yes", can_suspend)) {
			suspend_states.suspend2ram = true;
			suspend_states.suspend2ram_can = true;
			suspend_states.suspend2ram_allowed = 1;
		} else if (0 == strcmp("no", can_suspend)) {
			suspend_states.suspend2ram = true;
			suspend_states.suspend2ram_can = true;
			suspend_states.suspend2ram_allowed = 0;
		} else if (0 == strcmp("na", can_suspend)) {
			suspend_states.suspend2ram = false;
			suspend_states.suspend2ram_can = false;
			suspend_states.suspend2ram_allowed = 0;
		} else { // challenge
			suspend_states.suspend2ram = true;
			suspend_states.suspend2ram_can = true;
			suspend_states.suspend2ram_allowed = 0;
		}
	} else {
		kdError() << "org.freedesktop.login1.Manager.CanSuspend "
			  "returned null." << endl;
		suspend_states.suspend2ram = false;
		suspend_states.suspend2ram_can = false;
		suspend_states.suspend2ram_allowed = -1;
	}
	char *can_hibernate = nullptr;
	if (!dbus_HAL->dbusSystemMethodCall(
		    "org.freedesktop.login1", "/org/freedesktop/login1",
		    "org.freedesktop.login1.Manager", "CanHibernate",
		    &can_hibernate, DBUS_TYPE_STRING, DBUS_TYPE_INVALID)) {
		kdError() << "Error querying org.freedesktop.login1.Manager."
			"CanHibernate" << endl;
		suspend_states.suspend2disk = false;
		suspend_states.suspend2disk_can = false;
		suspend_states.suspend2disk_allowed = -1;
	}
	if (can_hibernate) {
		kdDebug() << "org.freedesktop.login1.Manager.CanHibernate "
			"returned " << can_hibernate << '.' << endl;
		if (0 == strcmp("yes", can_hibernate)) {
			suspend_states.suspend2disk = true;
			suspend_states.suspend2disk_can = true;
			suspend_states.suspend2disk_allowed = 1;
		} else if (0 == strcmp("no", can_hibernate)) {
			suspend_states.suspend2disk = true;
			suspend_states.suspend2disk_can = true;
			suspend_states.suspend2disk_allowed = 0;
		} else if (0 == strcmp("na", can_hibernate)) {
			suspend_states.suspend2disk = false;
			suspend_states.suspend2disk_can = false;
			suspend_states.suspend2disk_allowed = 0;
		} else {
			suspend_states.suspend2disk = true;
			suspend_states.suspend2disk_can = true;
			suspend_states.suspend2disk_allowed = 0;
		}
	} else {
		kdError() << "org.freedesktop.login1.Manager.CanHibernate "
			  "returned null." << endl;
		suspend_states.suspend2disk = false;
		suspend_states.suspend2disk_can = false;
		suspend_states.suspend2disk_allowed = -1;
	}

	kdDebugFuncOut(trace);
}

/*!
 * The function checks wether the machine support CPU frequency changes
 * via HAL.
 */
void HardwareInfo::checkCPUFreq() {
	kdDebugFuncIn(trace);
	
	cpuFreq = 0 == access("/usr/local/bin/cpufreq-governor-helper", X_OK);

	cpuFreqAllowed = dbus_HAL->check_auth("eu.domeniul-m.cpufreq-governor");

	checkCurrentCPUFreqPolicy();

	kdDebugFuncOut(trace);
}

/*!
 * The function check the currently selected CPU Frequency policy
 * \return the current policy
 */
cpufreq_type HardwareInfo::checkCurrentCPUFreqPolicy() {
	kdDebugFuncIn(trace);

	if (!cpuFreq) {
		kdDebugFuncOut(trace);
		return currentCPUFreqPolicy;
	}
	cpufreq_type _current = UNKNOWN_CPUFREQ;
	try {
		std::ifstream f;
		f.exceptions(std::ifstream::badbit | std::ifstream::failbit);
		f.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
		f >> cpuFreqGovernor;
		f.close();
		if ("performance" == cpuFreqGovernor)
			_current = PERFORMANCE;
		else if ("powersave" == cpuFreqGovernor)
			_current = POWERSAVE;
		if (trace)
			kdDebug() << "Current governor: "
				  << cpuFreqGovernor.c_str() << endl;
	} catch (const std::exception& e) {
		kdError() << "Error reading current CPU frequency governor"
			  << endl;
	}
	if (_current != currentCPUFreqPolicy) {
		currentCPUFreqPolicy = _current;
		update_info_cpufreq_policy_changed = true;
		emit currentCPUFreqPolicyChanged();
	} else
		update_info_cpufreq_policy_changed = false;

	kdDebugFuncOut(trace);
	return currentCPUFreqPolicy;
}


/*!
 * The function checks wether the machine provide a brightness interface and init 
 * (if needed) brightness information.
 */
void HardwareInfo::checkBrightness() {
	kdDebugFuncIn(trace);

	brightness = 0 == access("/usr/lib/xserver-xorg-video-intel/"
				 "xf86-video-intel-backlight-helper", X_OK);
	currentBrightnessLevel = -1;
	availableBrightnessLevels = -1;
	if (brightness) {
		try {
			std::ifstream f;
			f.exceptions(std::ifstream::badbit |
				     std::ifstream::failbit);
			f.open("/sys/class/backlight/intel_backlight/"
			       "max_brightness");
			f >> availableBrightnessLevels;
			if (trace)
				kdDebug() << "Available brightness levels: "
					  << availableBrightnessLevels << endl;
		} catch (const std::exception& e) {
			kdError() << "Could not read /sys/class/backlight/"
				"max_brightness: " << e.what() << endl;
		}
	}
	brightnessAllowed =
		dbus_HAL->check_auth("org.x.xf86-video-intel.backlight-helper");
	udis.remove("laptop_panel");

	checkCurrentBrightness();

	if (trace)
		kdDebug() << "Brightness: " << brightness << ", allowed: "
			  << brightnessAllowed << endl;

	kdDebugFuncOut(trace);
}


/*!
 * The function check the current brigthness
 */
void HardwareInfo::checkCurrentBrightness() {
	kdDebugFuncIn(trace);

	if (!brightness) {
		kdDebugFuncOut(trace);
		return;
	}

	try {
		std::ifstream f;
		f.exceptions(std::ifstream::badbit | std::ifstream::failbit);
		f.open("/sys/class/backlight/intel_backlight/brightness");
		f >> currentBrightnessLevel;
		if (trace)
			kdDebug() << "Current brightness: "
				  << currentBrightnessLevel << endl;
	} catch (const std::exception& e) {
		kdError() << "Could not read /sys/class/backlight/brightness: "
			  << e.what() << endl;
	}
	kdDebugFuncOut(trace);
}

bool
HardwareInfo::add_battery(const std::string& uid, bool primary,
			  const dict_type& props) {
	kdDebugFuncIn(trace);
	if (allUDIs.contains(uid.c_str())) {
		kdDebugFuncOut(trace);
		return false;
	}
	allUDIs.append(uid.c_str());
	std::unique_ptr<Battery> bat(new Battery(uid, props));
	if (primary)
		connect(bat.get(), SIGNAL(changedBattery()), this,
			SLOT(updatePrimaryBatteries()));
	BatteryList.append(bat.release());
	kdDebugFuncOut(trace);
	return true;
}

bool
HardwareInfo::add_ac(const std::string& uid, const dict_type& props) {
	kdDebugFuncIn(trace);
	udis.insert("acadapter", new QString(uid.c_str()));
	if (allUDIs.contains(uid.c_str())) {
		kdDebugFuncOut(trace);
		return false;
	}
	allUDIs.append(uid.c_str());
	signal_ac_change(props);
	kdDebugFuncOut(trace);
	return true;
}

/*!
 * The function initialise the hardware information and collect all
 * initial information from HAL.
 * \return boolean with result of the operation
 * \retval true  if successful
 * \retval false else, if a error occurs
 */
bool HardwareInfo::intialiseHWInfo() {
	kdDebugFuncIn(trace);

	QStringList ret;

	if (!dbus_HAL->isConnectedToDBUS()) 
		return false;

	DBusConnection *conn = dbus_HAL->get_DBUS_connection();
	if (conn) {
		devices_type devs = kps::upower_get_devices(conn);
		for (const std::string& dev : devs) {
			dict_type props =
				kps::upower_get_all_props(conn, dev.c_str());
			std::optional<uint32_t> t = kps::upower_dev_type(props);
			if (!t)
				continue;
			enum BAT_TYPE type = Battery::get_type(*t);
			switch (type) {
			case LINE_POWER: // line power
				add_ac(dev, props);
				break;
			case BAT_PRIMARY: // battery
				add_battery(dev, true, props);
				break;
			case BAT_UPS: // ups
				add_battery(dev, false, props);
				break;
			default:
				break;
			}
		}
	}

/*
	if( dbus_HAL->halFindDeviceByString("button.type", "lid", &ret)) {
		// there should be normaly only one device, but let be sure
		for ( QStringList::iterator it = ret.begin(); it != ret.end(); ++it ) {
			// we need a deep copy
			udis.insert("lidclose", new QString( *it ));
			if (!allUDIs.contains( *it ))
				allUDIs.append( *it );
			checkLidcloseState();
		}
	}
*/
	kdDebugFuncOut(trace);
	return true;
}

/*!
 * The function checks the state of the Lidclose button.
 */
void HardwareInfo::checkLidcloseState() {
	kdDebugFuncIn(trace);

/*
	if (udis["lidclose"]) {
		bool _state;

		if (dbus_HAL->halGetPropertyBool(*udis["lidclose"] , "button.state.value", &_state )) {
			if (_state != lidclose) {
				lidclose = _state;
				emit lidcloseStatus( lidclose );
			}
		} else {
			lidclose = false;
		}
	}
*/
	kdDebugFuncOut(trace);
}

void
HardwareInfo::updateBatteryValues(const char *udi, const dict_type& props) {
	kdDebugFuncIn(trace);

	if (udi && '\0' != *udi && allUDIs.contains(udi)) {
		// find effected battery object
		Battery *bat;
		for (bat = BatteryList.first(); bat; bat = BatteryList.next())
			if (0 == strncmp(udi, bat->getUdi(), strlen(udi)))
				// found a battery with udi
				bat->update(props);
	} else {
		kdDebug() << "UDI is empty or not in the list of monitored "
			"devices: " << udi << endl;
	}

	kdDebugFuncOut(trace);
	return;
}

/*!
 * This function refresh the information for the primary battery collection.
 */
void
HardwareInfo::updatePrimaryBatteries() {
	kdDebugFuncIn(trace);

	if (BatteryList.isEmpty()) {
		// this resets the primaryBatteries
		primaryBatteries->refreshInfo(BatteryList);
		kdDebugFuncOut(trace);
		return;
	}

	setPrimaryBatteriesWarningLevel(); // this calls refreshInfo

	if (primaryBatteries->getNumBatteries() < 1) {
		kdDebugFuncOut(trace);
		return;
	}

	connect(primaryBatteries, SIGNAL(batteryChanged()),
		this, SLOT(setPrimaryBatteriesChanges()));
	connect(primaryBatteries,
		SIGNAL(batteryWarnState(int,int)), this,
		SLOT(emitBatteryWARNState(int,int)));

	kdDebugFuncOut(trace);
}

/*!
 * This function set the change status for the primary battery collection
 */
void
HardwareInfo::setPrimaryBatteriesChanges() {
	kdDebugFuncIn(trace);

	update_info_primBattery_changed = true;
	emit primaryBatteryChanged();

	kdDebugFuncOut(trace);
}

/*!
 * This slot emit a signal if a warning state of a battery reached
 */
void
HardwareInfo::emitBatteryWARNState(int type, int state) {
	kdDebugFuncIn(trace);

	if (type == BAT_PRIMARY) 
		emit primaryBatteryChanged();
	else
		emit generalDataChanged();

	emit batteryWARNState(type, state);

	kdDebugFuncOut(trace);
}

// --> init HW information section -- END <---
// --> HAL method call (trigger actions) section -- START <---

/*!
 * Function to trigger a suspend via HAL 
 * \param suspend 	enum of suspend_type with the requested suspend
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool HardwareInfo::suspend(suspend_type suspend ) {
	kdDebugFuncIn(trace);
	
	calledSuspend = QTime();

	if (!dbus_HAL->isConnectedToDBUS()) {
		kdDebugFuncOut(trace);
		return false;
	}

	switch (suspend) {
	case SUSPEND2DISK:
		if (!suspend_states.suspend2disk) {
			kdDebug() << "The machine does not support suspend to "
				"disk." << endl;
			kdDebugFuncOut(trace);
			return false;
		}
		if (0 == suspend_states.suspend2disk_allowed) {
			kdWarning() << "Policy forbids user to trigger suspend"
				" to disk" << endl;
			kdDebugFuncOut(trace);
			return false;
		}
		if (!dbus_HAL->dbusMethodCallSuspend("Hibernate")) {
			kdDebugFuncOut(trace);
			return false;
		}
		calledSuspend.start();
		kdDebugFuncOut(trace);
		return true;
	case SUSPEND2RAM:
		if (!suspend_states.suspend2ram) {
			kdDebug() << "The machine does not support suspend to "
				"ram." << endl;
			kdDebugFuncOut(trace);
			return false;
		}
		if (0 == suspend_states.suspend2ram_allowed) {
			kdWarning() << "Policy forbids user to trigger suspend"
				" to ram" << endl;
			kdDebugFuncOut(trace);
			return false;
		}
		if (!dbus_HAL->dbusMethodCallSuspend("Suspend")) {
			kdDebugFuncOut(trace);
			return false;
		}
		calledSuspend.start();
		kdDebugFuncOut(trace);
		return true;
	case STANDBY:
		kdDebug() << "The machine does not support standby." << endl;
		kdDebugFuncOut(trace);
		return false;
	default:
		kdDebug() << "Unknown suspend method: " << suspend << '.'
			  << endl;
		kdDebugFuncOut(trace);
		return false;
	}
}

/*!
 * Function to set brightness via HAL (if supported by hardware)
 * \param level		Integer with the level to set, (range: 0 - \ref availableBrightnessLevels )
 * \param percent	Integer with the brightness percentage to set
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool
HardwareInfo::setBrightness(int level, int percent) {
	if (trace)
		kdDebug() << funcinfo << "IN: " << "level: " << level
			  << " percent: " << percent << endl;

	if (-1 == level && percent >= 0) {
		if (0 == percent)
			level = 0;
		else if (percent >= 98)
			level = availableBrightnessLevels - 1;
		else {
			level = availableBrightnessLevels * percent / 100.0;
			if (level > availableBrightnessLevels - 1)
				level = availableBrightnessLevels -1;
			kdDebug() << "percentage mapped to new level: "
				  << level << endl;
		}
	}

	if (!brightness)
		checkBrightness();

	if (!brightness || level < 0 || level >= availableBrightnessLevels) {
		kdError() << "Change brightness or requested level not "
			"supported " << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (currentBrightnessLevel == level) {
		kdDebug() << "Brightness level not changed, requested level == current level" << endl;
		kdDebugFuncOut(trace);
		return true;
	}

	int fd[2];
	if (-1 == pipe(fd)) {
		kdError() << "Error creating pipes" << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	pid_t p;
	switch (p = fork()) {
	case -1:
		kdError() << "Error forking pkexec" << endl;
		kdDebugFuncOut(trace);
		return false;
	case 0:
		close(fd[1]);
		if (-1 == dup2(fd[0], STDIN_FILENO)) {
			kdError() << "Error redirecting pipe to stdin" << endl;
			exit(0);
		}
		if (-1 == execlp("pkexec", "pkexec", "/usr/lib/"
				 "xserver-xorg-video-intel/"
				 "xf86-video-intel-backlight-helper",
				 "intel_backlight", nullptr)) {
			kdError() << "Error executing pkexec" << endl;
			exit(1);
		}
		// no return
	default: {
		close(fd[0]);
		char b[22];
		int n = snprintf(b, sizeof(b), "%d", level);
		if (n != write(fd[1], b, n))
			kdError() << "Error writing to the pipe to pkexec"
				  << endl;
		close(fd[1]);
		int s;
		if (-1 == waitpid(p, &s, 0)) {
			kdError() << "Waitpid error" << endl;
			break;
		}
		if (!WIFEXITED(s)) {
			kdError() << "pkexec did not exit normally" << endl;
			break;
		}
		int ec = WEXITSTATUS(s);
		if (0 != ec) {
			kdError() << "pkexec exited with code " << ec << endl;
			break;
		}
		break;
	}
	}

	// check for actual brightness level to be sure everything was set
	// correctly

	checkCurrentBrightness();

	kdDebugFuncOut(trace);
	return true;
} 

/*!
 * Function to set the CPU frequency policy via HAL. 
 * \param cpufreq 	enum of cpufreq_type with the policy to set
 * \param limit 	integer with range 0 - 100 (only if cpufreq == DYNAMIC)
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool
HardwareInfo::setCPUFreq(cpufreq_type cpufreq, int limit) {
	kdDebugFuncIn(trace);
	if (trace)
		kdDebug() << funcinfo << "IN: " <<  "cpufreq_type: "
			  << cpufreq << " limit: " << limit << endl;

	if (!cpuFreq) {
		kdDebug() << "This machine does not support change the CPU "
			"Freq." << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	
	if (0 == cpuFreqAllowed) {
		kdDebug() << "Not having the needed privileges to set the CPU "
			"freq." << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	
	if (checkCurrentCPUFreqPolicy() == cpufreq) {
		kdDebug() << "Didn't change Policy, was already set." << endl;
		kdDebugFuncOut(trace);
		return true;
	}

	switch (cpufreq) {
	case PERFORMANCE:
		if (!setCPUFreqGovernor("performance")) {
			kdDebug() << "Could not set CPU Freq to performance "
				"policy" << endl;
			kdDebugFuncOut(trace);
			return false;
		}
		break;
	case POWERSAVE:
		if (!setCPUFreqGovernor("powersave")) {
			kdDebug() << "Could not set CPU Freq to powersave "
				"policy." << endl;
			kdDebugFuncOut(trace);
			return false;
		}
		break;
	default:
		kdDebug() << "Unknown cpufreq_type: " << cpufreq << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	
	// check if the policy was really set (and emit signal)
	if (checkCurrentCPUFreqPolicy() != cpufreq) {
		kdDebugFuncOut(trace);
		return false;
	}
//	update_info_cpufreq_policy_changed = true;
//	emit currentCPUFreqPolicyChanged();
	kdDebugFuncOut(trace);
	return true;
}

/*!
 * Function to set the CPU governor via HAL. 
 * \param governor	char * with the name of the governor
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool
HardwareInfo::setCPUFreqGovernor(const char *governor) {
	kdDebugFuncIn(trace);

	kdDebug() << "Try to set CPUFreq to governor: " << governor << endl;

	pid_t p;
	switch (p = fork()) {
	case -1:
		kdDebug() << "Error forking pkexec" << endl;
		kdDebugFuncOut(trace);
		return false;
	case 0:
		if (-1 == execlp("pkexec", "pkexec", "/usr/local/bin/"
				 "cpufreq-governor-helper",
				 governor, nullptr)) {
			kdDebug() << "Error executing pkexec" << endl;
			exit(1);
		}
		// no return
	default: {
		int s;
		if (-1 == waitpid(p, &s, 0)) {
			kdDebug() << "Waitpid error" << endl;
			break;
		}
		if (!WIFEXITED(s)) {
			kdDebug() << "pkexec did not exit normally" << endl;
			break;
		}
		int ec = WEXITSTATUS(s);
		if (0 != ec) {
			kdDebug() << "pkexec exited with code " << ec << endl;
			break;
		}
		break;
	}
	}

	kdDebugFuncOut(trace);
	return true;
}


/*!
 * Function to set the powersave mode (incl. e.g. disk settings) via HAL. 
 * \param  on		boolean which tell if enable/disable powersave mode
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool
HardwareInfo::setPowerSave(bool on) {
	kdDebugFuncIn(trace);

	if (!dbus_HAL->check_auth("eu.domeniul-m.powersave")) {
		kdDebug() << "Not having the required privileges to set the"
			" powersave features." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	pid_t p;
	switch (p = fork()) {
	case -1:
		kdDebug() << "Error forking pkexec" << endl;
		kdDebugFuncOut(trace);
		return false;
	case 0:
		if (-1 == execlp("pkexec", "pkexec", "/usr/sbin/pm-powersave",
				 on ? "true" : "false", nullptr)) {
			kdDebug() << "Error executing pkexec" << endl;
			exit(1);
		}
		// no return
	default: {
		int s;
		if (-1 == waitpid(p, &s, 0)) {
			kdDebug() << "Waitpid error" << endl;
			break;
		}
		if (!WIFEXITED(s)) {
			kdDebug() << "pkexec did not exit normally" << endl;
			break;
		}
		int ec = WEXITSTATUS(s);
		if (0 != ec) {
			kdDebug() << "pkexec exited with code " << ec << endl;
			break;
		}
		break;
	}
	}

	kdDebugFuncOut(trace);
	return true;
}

/*!
 * Function to call GetSchedPowerSavings() via HAL. 
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool
HardwareInfo::getSchedPowerSavings() {
	kdDebugFuncIn(trace);

	if (!dbus_HAL->isConnectedToDBUS()) {
		kdDebugFuncOut(trace);
		return false;
	}


/*
	dbus_bool_t ret;
	//NOTE: the kernel only allow 1/0 for the related sysfs entry
	if (!dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, HAL_COMPUTER_UDI, 
					     HAL_CPUFREQ_IFACE, "GetSchedPowerSavings",
					     &ret, DBUS_TYPE_BOOLEAN, DBUS_TYPE_INVALID)) {
		schedPowerSavings = false;
		kdWarning() << "Could not call GetSchedPowerSavings() " << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	schedPowerSavings = 0 != ret;
*/	
	schedPowerSavings = false;

	kdDebugFuncOut(trace);
	return true;
}

/*!
 * Function to call SetSchedPowerSavings() via HAL. Note: this would only work on
 * Multiprocessor/-core machines.
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool
HardwareInfo::setSchedPowerSavings(bool enable) {
	kdDebugFuncIn(trace);

	if (!dbus_HAL->isConnectedToDBUS()) {
		kdDebugFuncOut(trace);
		return false;
	}

/*
	dbus_bool_t _tmp = static_cast<dbus_bool_t>(enable);

	//NOTE: the kernel only allow 1/0 for the related sysfs entry
	if (!dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, HAL_COMPUTER_UDI,
					    HAL_CPUFREQ_IFACE,
					    "SetCPUFreqPerformance", 
					    DBUS_TYPE_BOOLEAN, &_tmp,
					    DBUS_TYPE_INVALID)) {
		kdWarning() << "Could not call SetSchedPowerSavings() " << endl;
		return false;
	}
*/

	kdDebugFuncOut(trace);
	return true;
}


// --> HAL method call (trigger actions) section -- END <---

// --> private helper functions/slots to forward/handle events  -- START <--
//     need this functions to make events from HAL/D-Bus independent
//     from QT event loop and to allow QT3 D-Bus bindings to get not
//     blocked by normal KDE/QT (GUI) calls
/*!
 * Function to emit the signal for the Power button.
 */ 
void HardwareInfo::emitPowerButtonPressed() {
	if (sessionIsActive) {
		emit powerButtonPressed();
	} else {
		kdWarning() << "Session is not active, don't react on power "
			"button event!" << endl;
	}
}

/*!
 * Function to emit the signal for the Sleep button.
 */
void HardwareInfo::emitSleepButtonPressed() {
	if (sessionIsActive) {
		emit sleepButtonPressed();
	} else {
		kdWarning() << "Session is not active, don't react on sleep "
			"button event!" << endl;
	}
}

/*!
 * Function to emit the signal for the s2disk button.
 */
void HardwareInfo::emitS2diskButtonPressed() {
	if (sessionIsActive) {
		emit s2diskButtonPressed();
	} else {
		kdWarning() << "Session is not active, don't react on "
			"suspend2disk button event!" << endl;
	}
}

/*!
 * Function to emit the signal about changes in the session state
 */
void HardwareInfo::emitSessionActiveState() {
	if (sessionIsActive) {
		if (!dbus_HAL->aquiredPolicyPowerInterface()) {
			dbus_HAL->aquirePolicyPowerIface();
		}
	} else {
		if (dbus_HAL->aquiredPolicyPowerInterface()) {
			dbus_HAL->releasePolicyPowerIface();
		}
	}

	emit desktopSessionIsActive(sessionIsActive);
}

/*!
 * Function to set the brightess a step up.
 * \param percentageStep Integer of next step should get set
 * \return result of the operation
 * \retval true		if could get set 
 * \retval false	else
 */
bool HardwareInfo::setBrightnessUp(int percentageStep) {
	kdDebugFuncIn(trace);
	
	checkCurrentBrightness();

	if (!supportBrightness() || getCurrentBrightnessLevel() < 0 || 
            getMaxBrightnessLevel() - 1 == getCurrentBrightnessLevel()) {
		kdDebugFuncOut(trace);
		return false;
	}

	int setTo = 0;
	int minPercStep = 10;
	int currentPerc = 100.0 * getCurrentBrightnessLevel() /
		(getMaxBrightnessLevel() - 1);

	if (percentageStep > 0 && percentageStep <= 100 - currentPerc)
		minPercStep = percentageStep;

	if (currentPerc + minPercStep > 100)
		// set to 100 %
		setTo = getMaxBrightnessLevel() -1;
	else {
		setTo = (getMaxBrightnessLevel() - 1) *
			(currentPerc + minPercStep) / 100.0;
		if (setTo == getCurrentBrightnessLevel() &&
		    setTo < getMaxBrightnessLevel() - 1)
			++setTo;
	}
	
	if (trace)
		kdDebug() << "Max: " << getMaxBrightnessLevel() 
			  << " Current: " << getCurrentBrightnessLevel() 
			  << " minPercStep: " << minPercStep
			  << " currentPerc: " << currentPerc
			  << " setTo: " << setTo << endl;
	
	bool retval = setBrightness(setTo, -1);
	kdDebugFuncOut(trace);
	return retval;
}

/*!
 * Function to set the brightess a step up.
 * \param percentageStep Integer of next step should get set
 * \return result of the operation
 * \retval true		if could get set 
 * \retval false	else
 */
bool
HardwareInfo::setBrightnessDown(int percentageStep) {
	kdDebugFuncIn(trace);

	checkCurrentBrightness();

	if (!supportBrightness() || getCurrentBrightnessLevel() <= 0) {
		kdDebugFuncOut(trace);
		return false;
	}

	int setTo = 0;
	int minPercStep = 10;
	int currentPerc = 100.0 * getCurrentBrightnessLevel() /
		(getMaxBrightnessLevel() - 1);
	
	if (percentageStep > 0 && percentageStep < currentPerc)
		minPercStep = percentageStep;
	
	if (currentPerc - minPercStep < 0)
		setTo = 0;
	else {
		setTo = (getMaxBrightnessLevel() - 1) *
			(currentPerc - minPercStep) / 100.0;
		if (setTo == getCurrentBrightnessLevel() && setTo > 0)
			--setTo;
	}
	
	if (trace)
		kdDebug() << "Max: " << getMaxBrightnessLevel() 
			  << " Current: " << getCurrentBrightnessLevel() 
			  << " minPercStep: " << minPercStep
			  << " currentPerc: " << currentPerc
			  << " setTo: " << setTo << endl;
	
	bool retval = setBrightness(setTo, -1);
	kdDebugFuncOut(trace);
	return retval;
}

/*!
 * Function to handle the signal for the brightness up button/key
 */
void
HardwareInfo::brightnessUpPressed() {
	kdDebugFuncIn(trace);
	
	if (!brightness) {
		kdDebugFuncOut(trace);
		return;
	}
	if (!sessionIsActive) {
		kdWarning() << "Session is not active, don't react on "
			"brightness up key event!" << endl;
		kdDebugFuncOut(trace);
		return;
	}
	if (currentBrightnessLevel >= availableBrightnessLevels) {
		kdWarning() << "Could not set brightness to higher level, "
			"it's already set to max." << endl;
		kdDebugFuncOut(trace);
		return;
	}

	setBrightnessUp();

	kdDebugFuncOut(trace);
}

/*!
 * Function to handle the signal for the brightness down button/key
 */
void
HardwareInfo::brightnessDownPressed() {
	kdDebugFuncIn(trace);

	if (!brightness) {
		kdDebugFuncOut(trace);
		return;
	}
	if (!sessionIsActive) {
		kdWarning() << "Session is not active, don't react on brightness down key event!" << endl;
		kdDebugFuncOut(trace);
		return;
	}
	if (currentBrightnessLevel <= 0) {
		kdWarning() << "Could not set brightness to lower level, "
			"it's already set to min." << endl;
		kdDebugFuncOut(trace);
		return;
	}

	setBrightnessDown();

	kdDebugFuncOut(trace);
}

// --> private helper slots to forward/handle events -- END <--

// --> get private members section -- START <---

/*!
 * The function return the current state of the ac adapter.
 * \return boolean with the current state 
 * \retval true 	if adapter is present/connected or unknown
 * \retval false 	if not 
 */
bool HardwareInfo::getAcAdapter() const {
	return acadapter;
}

/*!
 * The function return the current state of the lidclose button.
 * \return boolean with the current state 
 * \retval true 	if the lid is closed
 * \retval false 	if the lid is opend
 */
bool HardwareInfo::getLidclose() const {
	return lidclose;
}

/*!
 * The function return the maximal available brightness level
 * \return Integer with max level or -1 if not supported
 */
int HardwareInfo::getMaxBrightnessLevel() const {
	return brightness ? availableBrightnessLevels : -1;
}

/*!
 * The function return the current brightness level
 * \return Integer with max level or -1 if not supported or unkown
 */
int HardwareInfo::getCurrentBrightnessLevel() const {
	return brightness ? currentBrightnessLevel : -1;
}

/*!
 * The function return the current set CPU Frequency Policy
 * \return Integer with currently set Policy or -1 if not supported or unkown
 */
int HardwareInfo::getCurrentCPUFreqPolicy() const {
	return currentCPUFreqPolicy;
}

/*!
 * The function return information if the system support the different
 * suspend/standby methodes and if the user can call them.
 * \return struct with information from \ref suspend_states
 * TODO: check if we maybe should replace this by more different functions
 */
SuspendStates HardwareInfo::getSuspendSupport() const {
	return suspend_states;
}

/*!
 * The function return a pointer to the battery collection of primary batteries.
 * \return BatteryCollection with type == PRIMARY
 */
BatteryCollection* HardwareInfo::getPrimaryBatteries() const {
	return primaryBatteries;
}

/*!
 * The function return all batteries
 * \return QPtrList<Battery>
 */
QPtrList<Battery> HardwareInfo::getAllBatteries() const {
	return BatteryList;
}


/*!
 * The function return the status of \ref laptop.
 * \return boolean with info if machine is a laptop 
 * \retval true 	if a laptop
 * \retval false 	else/if not a laptop
 */
bool HardwareInfo::isLaptop() const {
	return laptop;
}

/*!
 * The function return info if there is a working connection to D-Bus and HAL.
 * This mean if we get hardwareinformation
 * \return boolean with info if D-Bus and HAL work
 * \retval true 	if connected
 * \retval false 	if not connected
 */
bool HardwareInfo::isOnline() const {
	return !dbus_terminated;
}

/*!
 * The function return the status of \ref has_ACPI.
 * \return boolean with info if machine support ACPI 
 * \retval true 	if support ACPI
 * \retval false 	else
 */
bool HardwareInfo::hasACPI() const {
	return has_ACPI;
}

/*!
 * The function return the status of \ref has_APM.
 * \return boolean with info if machine support APM
 * \retval true 	if support APM
 * \retval false 	else
 */
bool HardwareInfo::hasAPM() const {
	return has_APM;
}

/*!
 * The function return the status of \ref has_PMU.
 * \return boolean with info if machine support PMU 
 * \retval true 	if support PMU
 * \retval false 	else
 */
bool HardwareInfo::hasPMU() const {
	return has_PMU;
}

/*!
 * The function return the status of \ref brightness.
 * \return boolean with info if machine support brightness changes via HAL
 * \retval true 	if support brightness changes
 * \retval false 	else
 */
bool HardwareInfo::supportBrightness() const {
	return brightness;
}

/*!
 * The function return the status of \ref cpuFreq.
 * \return boolean with info if machine support change the CPU frequency via HAL
 * \retval true 	if support brightness changes
 * \retval false 	else
 */
bool HardwareInfo::supportCPUFreq() const {
	return cpuFreq;
}

/*!
 * The function return the status of \ref sessionIsActive.
 * \return boolean with info if current desktop session is marked in ConsoleKit as activ
 * \retval true 	if the current session is active
 * \retval false 	else
 */
bool HardwareInfo::currentSessionIsActive() const {
	return sessionIsActive;
}

/*!
 * The function return \ref cpuFreqAllowed and tell by this if the user is allowed
 * to change the CPU Freq.
 * \return \ref cpuFreqAllowed
 * \retval	0 allowed
 * \retval	1 not allowed
 * \retval	-1 unknown
 */
int HardwareInfo::isCpuFreqAllowed() const {
	return cpuFreqAllowed;
}

/*! check if the org.freedesktop.Policy.Power interface has an owner
 * \return boolean with info if org.freedesktop.Policy.Power interface has an owner or not
 * \retval true 	if there is a owner
 * \retval false 	else
 */
bool HardwareInfo::isPolicyPowerIfaceOwned() {
	return dbus_HAL->isPolicyPowerIfaceOwned();
}


// --> get private members section -- END <---

bool
HardwareInfo::isBattery(const char *udi) {
	DBusConnection *conn = dbus_HAL->get_DBUS_connection();
	if (!conn) {
		kdError() << "isBattery: null dbus connection" << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	return isBattery(kps::upower_get_all_props(conn, udi));
}

bool
HardwareInfo::isBattery(const dict_type& props) const {
	dict_type::const_iterator i = props.find("Type");
	if (props.end() == i) {
		kdError() << "Properties contain no 'Type' property"
			  << endl;
		return false;
	}
	const uint32_t *n = std::any_cast<uint32_t>(&i->second);
	if (!n) {
		kdError() << "'Type' property is not a uint32"
			  << endl;
		return false;
	}
	switch (*n) {
	case 2: // battery
	case 3: // ups
	case 5: // mouse
	case 6: // keyboard
	case 8: // phone
		return true;
	case 0: // unknown
	case 1: // line power, not a battery
	case 4: // monitor
	case 7: // pda
	default:
		return false;
	}
}

bool
HardwareInfo::isPrimary(const dict_type& props) const {
	dict_type::const_iterator i = props.find("Type");
	if (props.end() == i) {
		kdError() << "Properties contain no 'Type' property"
			  << endl;
		return false;
	}
	const uint32_t *n = std::any_cast<uint32_t>(&i->second);
	if (!n) {
		kdError() << "'Type' property is not a uint32"
			  << endl;
		return false;
	}
	switch (*n) {
	case 2:
		return true;
	case 0: // unknown
	case 1: // line power, not a battery
	case 3:
	case 4: // monitor
	case 5:
	case 6:
	case 8: // phone
	case 7: // pda
	default:
		return false;
	}
}

#include "hardware.moc"
