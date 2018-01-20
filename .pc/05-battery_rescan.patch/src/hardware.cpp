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

/*! The default constructor of the class HardwareInfo */
HardwareInfo::HardwareInfo() {
	kdDebugFuncIn(trace);

	// init members
	acadapter = true;
	lidclose = false;
	dbus_terminated = true;
	hal_terminated = true;
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
	consoleKitSession = QString();
	BatteryList.setAutoDelete( true ); // the list owns the objects
	
	primaryBatteries = new BatteryCollection(BAT_PRIMARY);
	setPrimaryBatteriesWarningLevel(); // force default settings

	// connect to D-Bus and HAL
	dbus_HAL = new dbusHAL();
	if (dbus_HAL->isConnectedToDBUS()) {
		dbus_terminated = false;
		if (dbus_HAL->isConnectedToHAL()) {
			hal_terminated = false;
		} else {
			kdError() << "Could not connect to HAL" << endl;
		}
	} else {
		kdError() << "Could not connect to D-Bus & HAL" << endl;
	}

	checkConsoleKitSession();
	checkPowermanagement();
	checkIsLaptop();
	checkBrightness();
	checkCPUFreq();
	// getSchedPowerSavings(); 
	checkSuspend();
	intialiseHWInfo();

	updatePrimaryBatteries();

	// connect to signals
	connect(dbus_HAL, SIGNAL(msgReceived_withStringString( msg_type, QString, QString )),
		this, SLOT(processMessage( msg_type, QString, QString )));
	connect(dbus_HAL, SIGNAL(backFromSuspend(int)), this, SLOT(handleResumeSignal(int)));

	kdDebugFuncOut(trace);
}

/*! The default desctuctor of the class HardwareInfo */
HardwareInfo::~HardwareInfo() {
	kdDebugFuncIn(trace);

	delete dbus_HAL;
	dbus_HAL = NULL;

	kdDebugFuncOut(trace);
}

/*!
 * This funtion is used to handle the reconnect to the D-Bus daemon
 */
void HardwareInfo::reconnectDBUS() {
	kdDebugFuncIn(trace);

	if (!dbus_HAL->isConnectedToDBUS()) {
		bool _reconnect = dbus_HAL->reconnect();

		if (!_reconnect && !dbus_HAL->isConnectedToDBUS()) {
			//reconnect failed
			emit dbusRunning(DBUS_NOT_RUNNING);
			QTimer::singleShot(4000, this, SLOT(reconnectDBUS()));
		} else if (!_reconnect && dbus_HAL->isConnectedToDBUS()) {
			// reset everything, we are reconnected
			dbus_terminated = false;
			hal_terminated = true;
			emit dbusRunning(DBUS_RUNNING);
		} else if (_reconnect) {
			// reset everything, we are reconnected
			dbus_terminated = false;
			hal_terminated = false;
			reinitHardwareInfos();
			emit dbusRunning(hal_terminated);
			emit halRunning(DBUS_RUNNING);
		}  
	} 

	kdDebugFuncOut(trace);
}

/*!
 * This funtion is used to reinit all hardware information.
 * \return 		Boolean with result of the call
 * \retval true		if reinit HW infos correct
 * \retval false	if not
 */
bool HardwareInfo::reinitHardwareInfos () {
	kdDebugFuncIn(trace);
	
	if (dbus_HAL->isConnectedToDBUS() && dbus_HAL->isConnectedToHAL()) {
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
		primaryBatteries = new BatteryCollection(BAT_PRIMARY);

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
	} else {
		kdDebugFuncOut(trace);
		return false;
	}
}

/*!
 * This funtion is used to parse a message from D-Bus for the different 
 * messagetypes and events.
 * \param type 		a \ref msg_type which should be parse/processed
 * \param message 	the message
 * \param value 	an optional message value as e.g. message string
 */
void HardwareInfo::processMessage (msg_type type, QString message, QString value) {
	kdDebugFuncIn(trace);

	switch(type) {
		case ACPI_EVENT:
			// we don't handle acpi events here atm
			break;
		case DBUS_EVENT:
			kdDebug() << "D-Bus Event: " << value << endl;
			if ( message.startsWith("dbus.terminate")){
				// TODO: addapt from pdaemon
				dbus_terminated = true;
				QTimer::singleShot(4000, this, SLOT(reconnectDBUS()));
			}
			else if ( message.startsWith("hal.")) {
				if ( message.startsWith("hal.terminate")) {
					hal_terminated = true;
					emit halRunning(false);
					// TODO: update data
					emit generalDataChanged();
				} else if ( message.startsWith("hal.started")) {
					hal_terminated = false;
					reinitHardwareInfos();
					emit halRunning( true );
					// TODO: update data
					emit generalDataChanged();
				}
				// TODO: addapt from pdaemon
			}
			break;
		case HAL_DEVICE:
			// --> we can maybe ignore these events except for batteries, not shure atm
			int _type;

			if (message.startsWith("DeviceAdded")) {
				if (checkIfHandleDevice(value, &_type)) {
					switch (_type) {
						case BATTERY:
						case AC_ADAPTER:
						case BUTTON_SLEEP:
						case BUTTON_POWER:
						case LID:
							// TODO: handle code if needed actually not 
							break;
						case LAPTOP_PANEL:
							checkBrightness();
							break;
						default:
							kdDebug() << "New device added Device udi: "
								  << value << "type: " << _type << endl;
							break;
					}
				}
			} else if (message.startsWith("DeviceRemoved")) {
				if (allUDIs.contains(value)) {
					if (checkIfHandleDevice(value, &_type)) {
						switch (_type) {
							case BATTERY:
							case AC_ADAPTER:
							case BUTTON_SLEEP:
							case BUTTON_POWER:
							case LID:
								// TODO: handle code if needed
								break;
							case LAPTOP_PANEL:
								checkBrightness();
								break;
							default:
								kdDebug() << "Couldn't handle unknown device" << endl;
								break;
						}
					}
				} else {
					kdDebug() << "Not monitored device removed: " << value << endl;
				}
			} else {
				kdDebug() << "Unknown HAL_DEVICE message: " << message << endl;
			}
			break;
		case HAL_PROPERTY_CHANGED:
			if (!message.isEmpty() && allUDIs.contains( message )) {
				if (value.startsWith( "ac_adapter.present" )) {
					QTimer::singleShot(50, this, SLOT(checkACAdapterState()));
				} else if (value.startsWith( "battery." )) {
					// this is a battery event
					updateBatteryValues(message, value);
				} else if (value.startsWith( "button.state.value" )) {
					if (message.startsWith( *udis["lidclose"] )) {
						QTimer::singleShot(50, this, SLOT(checkLidcloseState()));
					}
				} else if (value.startsWith( "laptop_panel")) {
					if (message.startsWith( *udis["laptop_panel"] )) {
						QTimer::singleShot(50, this, SLOT(checkBrightness()));
					}
				}
				// TODO: add needed code
			} else {
				kdDebug() << "HAL_PROPERTY_CHANGED for unmonitored device: " << value << endl;
			}
			break;
		case HAL_CONDITION:
			// TODO: Check if we really need to monitor this events. We get maybe also 
			//	 HAL_PROPERTY_CHANGED event for the key
			if (message.startsWith("ButtonPressed")) {
				kdDebug() << "ButtonPressed event from HAL: " << value << endl;
				if (value.startsWith("lid")) {
					QTimer::singleShot(50, this, SLOT(checkLidcloseState()));
				} else if (value.startsWith("power")) {
					QTimer::singleShot(50, this, SLOT(emitPowerButtonPressed()));
				} else if (value.startsWith("sleep") || value.startsWith("suspend")) {
					QTimer::singleShot(50, this, SLOT(emitSleepButtonPressed()));
				} else if (value.startsWith("hibernate")) {
					QTimer::singleShot(50, this, SLOT(emitS2diskButtonPressed()));
				} else if (value.startsWith("brightness-")) {
					if (!brightness_in_hardware && value.endsWith("-up"))
						QTimer::singleShot(50, this, SLOT(brightnessUpPressed()));
					else if (!brightness_in_hardware && value.endsWith("-down"))
						QTimer::singleShot(50, this, SLOT(brightnessDownPressed()));
				}
			} else {
				kdDebug() << "Unmonitored HAL_CONDITION: " << message << " : " << value << endl;
			}
			break;
		case CONSOLEKIT_SESSION_ACTIVE:
			if (!message.isEmpty() && !value.isEmpty()) {
				if (message == consoleKitSession) {
					if (value == "1") {
						sessionIsActive = true;
					} else {
						sessionIsActive = false;
					}
					QTimer::singleShot(50, this, SLOT(emitSessionActiveState()));
				} else {
					if (trace)
						kdDebug() << "CONSOLEKIT_SESSION_ACTIVE: not our session" << endl;
				}
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

/*!
 * This SLOT is used to fetch the resume signal and multiplex. If needed some
 * actions after resume, do this here.
 * \param result 	integer with the result of the resume/suspend
 */
void HardwareInfo::handleResumeSignal (int result) {
	if (trace) kdDebug() << funcinfo <<  "IN: " << "(int result: " << result << ")"<< endl;

	if (result == -1) {
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

/*! 
 * This function checks the session for the running KPowersave instance
 * \return 		Boolean with result of operation
 * \retval true		if the query/check could get finished
 * \retval false	on every error
 */
bool HardwareInfo::checkConsoleKitSession () {
	kdDebugFuncIn(trace);

	bool retval = false;

	if (dbus_HAL->isConnectedToDBUS()) {
		char *reply;
		char *cookie = getenv("XDG_SESSION_COOKIE");

		if (cookie == NULL) {
			kdDebug() << "Could not get XDG_SESSION_COOKIE from environment" << endl;
			sessionIsActive = true;
		} else {
			if (dbus_HAL->dbusSystemMethodCall( CK_SERVICE, CK_MANAGER_OBJECT, 
							CK_MANAGER_IFACE, "GetSessionForCookie", 
							&reply, DBUS_TYPE_OBJECT_PATH,
							DBUS_TYPE_STRING, &cookie,
							DBUS_TYPE_INVALID)) {
				if (trace) 
					kdDebug() << "GetSessionForCookie returned: " << reply << endl;
				
				if (reply != NULL) {
					dbus_bool_t i_reply;
					consoleKitSession = reply;
	
					if (dbus_HAL->dbusSystemMethodCall( CK_SERVICE, consoleKitSession, 
									    CK_SESSION_IFACE, "IsActive", 
									    &i_reply, DBUS_TYPE_BOOLEAN,
									    DBUS_TYPE_INVALID)) {
						sessionIsActive = ((i_reply != 0) ? true: false);
						if (trace) 
							kdDebug() << "IsActive returned: " << sessionIsActive << endl;
	
						retval = true;
					} else {
						kdError() << "Could get session cookie and session name, but not "
							<< "but not the status of the session. Assume for now " 
							<< "the Session is inactive!" << endl;
						sessionIsActive = false;
					}
				}
			}
		}
	}

	kdDebugFuncOut(trace);
	return retval;
}


/*! 
 * This function check for a given UDI, if we should handle a device
 * \param _udi		QString with the UDI of the device
 * \param *type		pointer to a integer to return the type of the device, see \ref device_type
 * \return 		Boolean with info if we should handle the device.
 * \retval true		if we should handle
 * \retval false	if not
 */
bool HardwareInfo::checkIfHandleDevice ( QString _udi, int *type) {
	kdDebugFuncIn(trace);
	
	QStringList _cap;
	bool ret = true;
	
	if (dbus_HAL->halGetPropertyStringList( _udi, "info.capabilities", &_cap) && !_cap.isEmpty()) {
		if (_cap.contains("ac_adapter")) {
			*type = BATTERY;
		} else if (_cap.contains("button")) {
			QString _val;
			if (dbus_HAL->halGetPropertyString( _udi, "button.type", &_val)) {
				if (_val.startsWith("lid")) {
					*type = LID;
				} else if ( _val.startsWith("power")) {
					*type = BUTTON_POWER;
				} else if ( _val.startsWith("sleep")) {
					*type = BUTTON_SLEEP;
				} else {
					ret = false;
				}
			} else {
				ret = false;
			}
		} else if (_cap.contains("battery")) {
			*type = BATTERY;
		} else if (_cap.contains("laptop_panel")) {
			*type = LAPTOP_PANEL;
		} else {
			ret = false;
			kdDebug() << "Device with capability " << _cap.join(", ") << " unhandled" << endl;
		}
	} else {
		ret = false;
	}
	
	if (!ret) *type = UNKNOWN_DEVICE;
	
	kdDebugFuncOut(trace);
	return ret;
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
void HardwareInfo::setPrimaryBatteriesWarningLevel (int _warn, int _low, int _crit ) {
	if (trace) kdDebug() << funcinfo << "IN: " << "warn: " << _warn << " low: " << _low << " crit: " << _crit << endl;

	if (_warn > -1 && _low > -1 && _crit > -1 ){
		primaryBatteriesWarnLevel = _warn;
		primaryBatteriesLowLevel = _low;
		primaryBatteriesCriticalLevel = _crit;
	}

	if (primaryBatteries) {
		primaryBatteries->setWarnLevel( primaryBatteriesWarnLevel );
		primaryBatteries->setLowLevel( primaryBatteriesLowLevel );
		primaryBatteries->setCritLevel( primaryBatteriesCriticalLevel );
		if (!BatteryList.isEmpty()) {
			primaryBatteries->refreshInfo( BatteryList, true );
		}
	}

	kdDebugFuncOut(trace);
}

// --> init HW information section -- START <---

/*!
 * The function checks if the machine is a laptop.
 */
void HardwareInfo::checkIsLaptop () {
	kdDebugFuncIn(trace);

	QString ret;
	
	if (dbus_HAL->halGetPropertyString(HAL_COMPUTER_UDI, "system.formfactor", &ret)) {

		if (!ret.isEmpty() && ret.startsWith("laptop"))
			laptop = true;
		else 
			laptop = false;
	} else {
		// error case
		laptop = false;
	}

	kdDebugFuncOut(trace);
}

/*!
 * The function checks wether the machine support ACPI/APM/PMU or not.
 */
void HardwareInfo::checkPowermanagement() {
	kdDebugFuncIn(trace);
	
	QString ret;

	has_APM = false;
	has_ACPI = false;
	has_PMU = false;
	
	if (dbus_HAL->halGetPropertyString( HAL_COMPUTER_UDI, "power_management.type", &ret)) {
	
		if (ret.isEmpty()) {
			return;
		} else if (ret.startsWith("acpi")) {
			has_ACPI = true;
		} else if (ret.startsWith("apm")) {
			has_APM = true;
		} else if (ret.startsWith("pmu")) {
			has_PMU = true;
		}
	}

	kdDebugFuncOut(trace);
}


/*!
 * The function checks wether the machine can suspend/standby.
 */
void HardwareInfo::checkSuspend() {
	kdDebugFuncIn(trace);
	
	QStringList ret;
	bool _ret_b = false;

	suspend_states = SuspendStates();

	if (dbus_HAL->halGetPropertyStringList( HAL_COMPUTER_UDI, HAL_PM_IFACE ".method_names",
						&ret )) {
		// check for suspend2ram
		if (dbus_HAL->halGetPropertyBool( HAL_COMPUTER_UDI, "power_management.can_suspend", &_ret_b ) ||
		    dbus_HAL->halGetPropertyBool( HAL_COMPUTER_UDI, "power_management.can_suspend_to_ram", 
						  &_ret_b )) {
			suspend_states.suspend2ram_can = _ret_b;
			if (_ret_b) {
				if (ret.contains( "Suspend" )) {
					suspend_states.suspend2ram = true;
					suspend_states.suspend2ram_allowed = dbus_HAL->isUserPrivileged(PRIV_SUSPEND,
													HAL_COMPUTER_UDI);
			}
			} else {
				suspend_states.suspend2ram = false;
				suspend_states.suspend2ram_allowed = -1;
			}
		} else {
			suspend_states.suspend2ram_can = false;
			suspend_states.suspend2ram = false;
			suspend_states.suspend2ram_allowed = -1;
		}

		// check for suspend2disk
		if (dbus_HAL->halGetPropertyBool( HAL_COMPUTER_UDI, "power_management.can_hibernate", &_ret_b ) ||
		    dbus_HAL->halGetPropertyBool( HAL_COMPUTER_UDI, "power_management.can_suspend_to_disk", 
						  &_ret_b )) {
			suspend_states.suspend2disk_can = _ret_b;
			if (_ret_b) {
				if (ret.contains( "Hibernate" )) {
					suspend_states.suspend2disk = true;
					suspend_states.suspend2disk_allowed =
								 dbus_HAL->isUserPrivileged(PRIV_HIBERNATE,
											    HAL_COMPUTER_UDI);
			}
			} else {
				suspend_states.suspend2disk = false;
				suspend_states.suspend2disk_allowed = -1;
			}
		} else {
			suspend_states.suspend2disk_can = false;
			suspend_states.suspend2disk = false;
			suspend_states.suspend2disk_allowed = -1;
		}

		// check for StandBy
		if (dbus_HAL->halGetPropertyBool( HAL_COMPUTER_UDI, "power_management.can_standby", &_ret_b )) {
			suspend_states.standby_can = _ret_b;
			if (_ret_b) {
				if (ret.contains( "Standby" )) {
					suspend_states.standby = true;
					suspend_states.standby_allowed = dbus_HAL->isUserPrivileged(PRIV_STANDBY,
												    HAL_COMPUTER_UDI);
			}
			} else {
				suspend_states.standby = false;
				suspend_states.standby_allowed = -1;
			}
		} else {
			suspend_states.standby_can = false;
			suspend_states.standby = false;
			suspend_states.standby_allowed = -1;
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * The function checks wether the machine support CPU frequency changes
 * via HAL.
 */
void HardwareInfo::checkCPUFreq() {
	kdDebugFuncIn(trace);
	
	bool ret = false;

	if (dbus_HAL->halQueryCapability( HAL_COMPUTER_UDI, "cpufreq_control", &ret )) {
		cpuFreq = ret;
		cpuFreqAllowed = dbus_HAL->isUserPrivileged( PRIV_CPUFREQ, HAL_COMPUTER_UDI);

		checkCurrentCPUFreqPolicy();
	} else {
		cpuFreq = false;
	}

	kdDebugFuncOut(trace);
}

/*!
 * The function check the currently selected CPU Frequency policy
 * \return the current policy
 */
cpufreq_type HardwareInfo::checkCurrentCPUFreqPolicy() {
	kdDebugFuncIn(trace);
	
	char *gov;

	cpufreq_type _current = UNKNOWN_CPUFREQ;

	if (cpuFreq) {

		if (dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, HAL_COMPUTER_UDI, 
						    HAL_CPUFREQ_IFACE, "GetCPUFreqGovernor", 
						    &gov, DBUS_TYPE_STRING, DBUS_TYPE_INVALID)) {
			if (gov != NULL) {
				kdDebug() << "got CPU Freq gov: " << gov << endl;
				if (!strcmp(gov, "ondemand") || !strcmp(gov, "userspace") ||
				    !strcmp(gov, "conservative")) {
					_current = DYNAMIC;
				} else if (!strcmp(gov, "powersave")) {
					_current = POWERSAVE;
				} else if (!strcmp(gov, "performance")) {
					_current = PERFORMANCE;
				} else {
					kdError() << "Got unknown CPUFreq Policy back: " << gov << endl;
				}
				cpuFreqGovernor = gov;
			}
			else {
				kdWarning() << "Could not get information about current governor" << endl;
			}
		} else {
			kdWarning() << "Could not get information about current governor" << endl;
		}
	} else {
		kdWarning() << "CPU Frequency interface not supported by machine or HAL" << endl;
	}

	if (_current != currentCPUFreqPolicy) {
		currentCPUFreqPolicy = _current;
		update_info_cpufreq_policy_changed = true;
		emit currentCPUFreqPolicyChanged();
	} else {
		update_info_cpufreq_policy_changed = false;
	}
	
	kdDebugFuncOut(trace);
	return currentCPUFreqPolicy;
}


/*!
 * The function checks wether the machine provide a brightness interface and init 
 * (if needed) brightness information.
 */
void HardwareInfo::checkBrightness() {
	kdDebugFuncIn(trace);

	QStringList devices;

	brightness = false;
	currentBrightnessLevel = -1;
	availableBrightnessLevels = -1;

	if( dbus_HAL->halFindDeviceByCapability("laptop_panel", &devices)) {
		if (devices.isEmpty()) {
			udis.remove("laptop_panel");
			kdDebug() << "no device with category laptop_panel found" << endl;
			kdDebugFuncOut(trace);
			return;
		} else {
			int retval;
			
	
			kdDebug() << "laptop_panel device found: " << devices.first() << endl;
			// we should asume there is only one laptop panel device in the system
			if (dbus_HAL->halGetPropertyInt(devices.first(), "laptop_panel.num_levels", &retval )) {
				udis.insert("laptop_panel", new QString( devices.first() ));
				if (!allUDIs.contains( devices.first() ))
					allUDIs.append( devices.first() );
				
				if (retval > 1) {
					dbus_HAL->halGetPropertyBool(devices.first(), "laptop_panel.brightness_in_hardware",
							             &brightness_in_hardware);

					availableBrightnessLevels = retval;
#ifdef USE_LIBHAL_POLICYCHECK
					brightnessAllowed = dbus_HAL->isUserPrivileged( PRIV_LAPTOP_PANEL,
										        devices.first());
					// TODO: check brightnessAllowed
#endif
					brightness = true;
					// get the current level via GetBrightness
					checkCurrentBrightness();
				} else {
					kdError() << "Found a Panel, but laptop_panel.num_levels < 2, which means "
						  << "KPowersave can't set usefull values" << endl;
				}	
			} 
		}
	}
	
	kdDebugFuncOut(trace);
}


/*!
 * The function check the current brigthness
 */
void HardwareInfo::checkCurrentBrightness() {
	kdDebugFuncIn(trace);

	if (brightness) {
		int retval;
		// get the current level via GetBrightness
		if (dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, *udis["laptop_panel"], HAL_LPANEL_IFACE, 	
						    "GetBrightness", &retval, DBUS_TYPE_INT32,
						    DBUS_TYPE_INVALID ) ) {
			currentBrightnessLevel = (int) retval;
		}
	}
	
	kdDebugFuncOut(trace);
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

	if (!dbus_HAL->isConnectedToDBUS() || !dbus_HAL->isConnectedToHAL()) 
		return false;

	if( dbus_HAL->halFindDeviceByCapability("ac_adapter", &ret)) {
		// there should be normaly only one device, but let be sure
		for ( QStringList::iterator it = ret.begin(); it != ret.end(); ++it ) {
			// we need a deep copy
			udis.insert("acadapter", new QString( *it ));
			if (!allUDIs.contains( *it ))
				allUDIs.append( *it );
			checkACAdapterState();
		}
	}
	
	ret.clear();

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

	ret.clear();

	// find batteries and fill battery information
	if( dbus_HAL->halFindDeviceByCapability("battery", &ret)) {
		if (!ret.isEmpty()) {
			// there should be normaly only one device, but let be sure
			for ( QStringList::iterator it = ret.begin(); it != ret.end(); ++it ) {
				if (!allUDIs.contains( *it ))
					allUDIs.append( *it );
				BatteryList.append( new Battery(dbus_HAL, *it) );
			}
		
			// connect to signals for primary batteries:
			Battery *bat;
			for (bat = BatteryList.first(); bat; bat = BatteryList.next() ) {
				if (bat->getType() == BAT_PRIMARY) {
					connect(bat, SIGNAL(changedBattery()),this, SLOT(updatePrimaryBatteries()));
				}
			}
		}
	}

	kdDebugFuncOut(trace);
	return true;
}

/*!
 * The function/SLOT checks the state of the AC adapter.
 */
void HardwareInfo::checkACAdapterState() {
	kdDebugFuncIn(trace);

	if ( udis["acadapter"] ) {
		bool _state;

		if (dbus_HAL->halGetPropertyBool(*udis["acadapter"] , "ac_adapter.present", &_state )) {
			if (_state != acadapter) {
				acadapter = _state;
				update_info_ac_changed = true;
				emit ACStatus( acadapter );
			} else {
				update_info_ac_changed = false;
			}
		} else {
			// we use true as default e.g. for workstations
			acadapter = true;
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * The function checks the state of the Lidclose button.
 */
void HardwareInfo::checkLidcloseState() {
	kdDebugFuncIn(trace);

	if ( udis["lidclose"] ) {
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

	kdDebugFuncOut(trace);
}

/*!
 * This funtion is used to call a update of a battery value for a given 
 * UDI and the given changed property
 * \param udi 		QString with the UDI of the battery
 * \param property	QString with the changed property
 */
void HardwareInfo::updateBatteryValues (QString udi, QString property) {
	kdDebugFuncIn(trace);

	if (!udi.isEmpty() && allUDIs.contains( udi )) {
		// find effected battery object
		Battery *bat;
		for (bat = BatteryList.first(); bat; bat = BatteryList.next() ) {
			if (udi.startsWith( bat->getUdi())) {
				// found a battery with udi
				bat->updateProperty(udi, property);
			}
		}
	} else {
		kdDebug() << "UDI is empty or not in the list of monitored devices: " << udi << endl;
	}

	kdDebugFuncOut(trace);
	return;
}

/*!
 * This function refresh the information for the primary battery collection.
 */
void HardwareInfo::updatePrimaryBatteries () {
	kdDebugFuncIn(trace);

	if (!BatteryList.isEmpty()) {
		if (primaryBatteries->getNumBatteries() < 1) {
			setPrimaryBatteriesWarningLevel();
			primaryBatteries->refreshInfo( BatteryList );
			connect(primaryBatteries, SIGNAL(batteryChanged()), this, 
				SLOT(setPrimaryBatteriesChanges()));
			connect(primaryBatteries, SIGNAL(batteryWarnState(int,int)), this,
				SLOT(emitBatteryWARNState(int,int)));
		} else {
			setPrimaryBatteriesWarningLevel();
			primaryBatteries->refreshInfo( BatteryList );
		}
	} else {
		primaryBatteries = new BatteryCollection(BAT_PRIMARY);
	}

	kdDebugFuncOut(trace);
}

/*!
 * This function set the change status for the primary battery collection
 */
void HardwareInfo::setPrimaryBatteriesChanges () {
	kdDebugFuncIn(trace);

	update_info_primBattery_changed = true;
	emit primaryBatteryChanged();

	kdDebugFuncOut(trace);
}

/*!
 * This slot emit a signal if a warning state of a battery reached
 */
void HardwareInfo::emitBatteryWARNState (int type, int state) {
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
bool HardwareInfo::suspend( suspend_type suspend ) {
	kdDebugFuncIn(trace);

	calledSuspend = QTime();

	if (dbus_HAL->isConnectedToDBUS() && dbus_HAL->isConnectedToHAL()) {
		switch (suspend) {
			case SUSPEND2DISK:
				if (suspend_states.suspend2disk && (suspend_states.suspend2disk_allowed != 0)) {
					if (dbus_HAL->dbusMethodCallSuspend("Hibernate")) {
						calledSuspend.start();
						return true;
					} else {
						return false;
					}
				} else {
					if ( !suspend_states.suspend2disk ) 
						kdDebug() << "The machine does not support suspend to disk." << endl;
					else
						kdWarning() << "Policy forbid user to trigger suspend to disk" << endl;

					return false;
				}
				break;
			case SUSPEND2RAM:
				if (suspend_states.suspend2ram && (suspend_states.suspend2ram_allowed != 0)) {
					if (dbus_HAL->dbusMethodCallSuspend("Suspend")) {
						calledSuspend.start();
						return true;
					} else {
						return false;
					}
				} else {
					if ( !suspend_states.suspend2ram ) 
						kdDebug() << "The machine does not support suspend to ram." << endl;
					else
						kdWarning() << "Policy forbid user to trigger suspend to ram" << endl;
					
					return false;
				}
				break;
			case STANDBY:
				if (suspend_states.standby && (suspend_states.standby_allowed != 0)) {
					if (dbus_HAL->dbusMethodCallSuspend("Standby")) {
						calledSuspend.start();
						return true;
					} else {
						return false;
					}
				} else {
					if ( !suspend_states.standby ) 
						kdDebug() << "The machine does not support standby." << endl;
					else
						kdWarning() << "Policy forbid user to trigger standby" << endl;

					return false;
				}
				break;
			default:
				return false;
		}
	}
	
	kdDebugFuncOut(trace);
	return false;
}

/*!
 * Function to set brightness via HAL (if supported by hardware)
 * \param level		Integer with the level to set, (range: 0 - \ref availableBrightnessLevels )
 * \param percent	Integer with the brightness percentage to set
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool HardwareInfo::setBrightness ( int level, int percent ){
	if (trace) kdDebug() << funcinfo << "IN: " << "level: " << level << " percent: " << percent << endl;

	bool retval = false;

	if ((level == -1) && (percent >= 0)) {
		if (percent == 0) {
			level = 0;
		} else if (percent >= 98) {
			level = (availableBrightnessLevels - 1);
		} else {
			level = (int)((float)availableBrightnessLevels * ((float)percent/100.0));
			if (level > (availableBrightnessLevels -1))
				level = availableBrightnessLevels -1;
			kdDebug() << "percentage mapped to new level: " << level << endl;
		}
	}

	if (dbus_HAL->isConnectedToDBUS() && dbus_HAL->isConnectedToHAL()) {
		if (!brightness)
			checkBrightness();

		if (!brightness || (level < 0 ) || (level >= availableBrightnessLevels)) {
			kdError() << "Change brightness or requested level not supported " << endl;
		} else {
			if (currentBrightnessLevel == level) {
				kdDebug() << "Brightness level not changed, requested level == current level" << endl;
				retval = true;
			} else {
				if (dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, *udis["laptop_panel"], 
								    HAL_LPANEL_IFACE, "SetBrightness", 
								    DBUS_TYPE_INT32, &level,
								    DBUS_TYPE_INVALID )) {
					retval = true;
				} 
			}
		}
	}

	// check for actual brightness level to be sure everything was set correct
	checkCurrentBrightness();
	kdDebugFuncOut(trace);
	return retval;
} 

/*!
 * Function to set the CPU frequency policy via HAL. 
 * \param cpufreq 	enum of cpufreq_type with the policy to set
 * \param limit 	integer with range 0 - 100 (only if cpufreq == DYNAMIC)
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool HardwareInfo::setCPUFreq ( cpufreq_type cpufreq, int limit ) {
	if (trace) kdDebug() << funcinfo << "IN: " <<  "cpufreq_type: " << cpufreq << " limit: " << limit << endl;

	if (!cpuFreq) {
		kdError() << "This machine does not support change the CPU Freq via HAL" << endl;
		return false;
	}
	
	if (cpuFreqAllowed  == 0) {
		kdError() << "Could not set CPU Freq, this not the needed privileges." << endl;
		return false;
	}
	
	if (dbus_HAL->isConnectedToDBUS() && dbus_HAL->isConnectedToHAL()) {
		dbus_bool_t consider = (dbus_bool_t) getAcAdapter();
		QStringList dynamic;

		if (checkCurrentCPUFreqPolicy() == cpufreq) {
			if (cpufreq == DYNAMIC && !cpuFreqGovernor.startsWith("ondemand")) {
				kdDebug() << "CPU Freq Policy is already DYNAMIC, but not governor is currently "
					  << "not 'ondemand'. Try to set ondemand governor." << endl;
			} else {
				kdDebug() << "Didn't change Policy, was already set." << endl;
				return true;
			}
		}

		switch (cpufreq) {
			case PERFORMANCE:
				if (!setCPUFreqGovernor("performance")) {
					kdError() << "Could not set CPU Freq to performance policy" << endl;
					return false;
				}
				break;
			case DYNAMIC:
				dynamic << "ondemand" << "userspace" << "conservative";

				for (QStringList::Iterator it = dynamic.begin(); it != dynamic.end(); it++){
					kdDebug() << "Try to set dynamic CPUFreq to: " << *it << endl;
					
					if (setCPUFreqGovernor((*it).latin1())) {
						kdDebug() << "Set dynamic successful to: " << *it << endl;
						break;
					}
				}
			
				// set dynamic performance limit
				if (!dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, HAL_COMPUTER_UDI,
								     HAL_CPUFREQ_IFACE,
								     "SetCPUFreqPerformance", 
								     DBUS_TYPE_INT32, &limit,
								     DBUS_TYPE_INVALID)) {
					kdError() << "Could not call/set SetCPUFreqPerformance with value: "
					          << limit << endl;
				}

				// correct set ondemand
				if (!dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, HAL_COMPUTER_UDI,
								     HAL_CPUFREQ_IFACE,
								     "SetCPUFreqConsiderNice",
								     DBUS_TYPE_BOOLEAN, &consider,
								     DBUS_TYPE_INVALID)) {
					kdError() << "Couldn't set SetCPUFreqConsiderNice for DYNAMIC" << endl;
				}

				break;
			case POWERSAVE:
				if (!setCPUFreqGovernor("powersave")) {
					kdError() << "Could not set CPU Freq to powersave policy" << endl;
					return false;
				}
				break;
			default:
				kdWarning() << "Unknown cpufreq_type: " << cpufreq << endl;
				return false;
		}
		
		// check if the policy was really set (and emit signal)
		if (checkCurrentCPUFreqPolicy() == cpufreq) {
//			update_info_cpufreq_policy_changed = true;
//			emit currentCPUFreqPolicyChanged();
			return true;
		} else {
			return false;
		}
	} else {	
		return false;
	}
}

/*!
 * Function to set the CPU governor via HAL. 
 * \param governor	char * with the name of the governor
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool HardwareInfo::setCPUFreqGovernor( const char *governor ) {
	kdDebugFuncIn(trace);

	int reply;
	bool ret = true;

	kdDebug() << "Try to set CPUFreq to governor: " << governor << endl;
	if (!dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, HAL_COMPUTER_UDI, 
					     HAL_CPUFREQ_IFACE, "SetCPUFreqGovernor",
					     &reply, DBUS_TYPE_INVALID,
					     DBUS_TYPE_STRING, &governor, 
					     DBUS_TYPE_INVALID)) {
		kdError() << "Could not set CPU Freq to governor: " << governor << endl;
		ret = false;
	}
	
	kdDebugFuncOut(trace);
	return ret;
}


/*!
 * Function to set the powersave mode (incl. e.g. disk settings) via HAL. 
 * \param  on		boolean which tell if enable/disable powersave mode
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool HardwareInfo::setPowerSave( bool on ) {
	kdDebugFuncIn(trace);

	bool retval = false;

	if (dbus_HAL->isConnectedToDBUS() && dbus_HAL->isConnectedToHAL()) {
		dbus_bool_t _tmp = (dbus_bool_t) on;
		int reply;

#ifdef USE_LIBHAL_POLICYCHECK
		if (dbus_HAL->isUserPrivileged(PRIV_SETPOWERSAVE, HAL_COMPUTER_UDI) != 0) {
#else
		if (true) {
#endif
			if (!dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, HAL_COMPUTER_UDI, 
							     HAL_PM_IFACE, "SetPowerSave",
							     &reply, DBUS_TYPE_INT32,
							     DBUS_TYPE_BOOLEAN, &_tmp,
							     DBUS_TYPE_INVALID)) {
				kdError() << "Could not call/set SetPowerSave on HAL, " 
					  << "could be a bug in HAL spec" << endl;
			} else {
				retval = true;
			}
		} else {
			kdError() << "The user isn't allowed to call SetPowerSave() on HAL. "
				  << "Maybe KPowersave run not in a active session." << endl;
		}
	} 
	
	kdDebugFuncOut(trace);
	return retval;
}

/*!
 * Function to call GetSchedPowerSavings() via HAL. 
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool HardwareInfo::getSchedPowerSavings() {
	kdDebugFuncIn(trace);

	bool returnval = false;

	if (dbus_HAL->isConnectedToDBUS() && dbus_HAL->isConnectedToHAL()) {
		dbus_bool_t ret;

		//NOTE: the kernel only allow 1/0 for the related sysfs entry
		if (dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, HAL_COMPUTER_UDI, 
						    HAL_CPUFREQ_IFACE, "GetSchedPowerSavings",
					            &ret, DBUS_TYPE_BOOLEAN, DBUS_TYPE_INVALID)) {
			schedPowerSavings = ((ret != 0) ? true: false);
			returnval = true;
		} else {
			schedPowerSavings = false;
			kdWarning() << "Could not call GetSchedPowerSavings() " << endl;
		}
	}

	kdDebugFuncOut(trace);
	return returnval;
}

/*!
 * Function to call SetSchedPowerSavings() via HAL. Note: this would only work on
 * Multiprocessor/-core machines.
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool HardwareInfo::setSchedPowerSavings( bool enable ) {
	kdDebugFuncIn(trace);

	bool retval = false;

	if (dbus_HAL->isConnectedToDBUS() && dbus_HAL->isConnectedToHAL()) {
		dbus_bool_t _tmp = (dbus_bool_t) enable;

		//NOTE: the kernel only allow 1/0 for the related sysfs entry
		if (dbus_HAL->dbusSystemMethodCall( HAL_SERVICE, HAL_COMPUTER_UDI,
						    HAL_CPUFREQ_IFACE,
						    "SetCPUFreqPerformance", 
						    DBUS_TYPE_BOOLEAN, &_tmp,
						    DBUS_TYPE_INVALID)) {
			retval = true;
		} else {
			kdWarning() << "Could not call SetSchedPowerSavings() " << endl;
		}
	}

	kdDebugFuncOut(trace);
	return retval;
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
		kdWarning() << "Session is not active, don't react on power button event!" << endl;
	}
}

/*!
 * Function to emit the signal for the Sleep button.
 */
void HardwareInfo::emitSleepButtonPressed() {
	if (sessionIsActive) {
		emit sleepButtonPressed();
	} else {
		kdWarning() << "Session is not active, don't react on sleep button event!" << endl;
	}
}

/*!
 * Function to emit the signal for the s2disk button.
 */
void HardwareInfo::emitS2diskButtonPressed() {
	if (sessionIsActive) {
		emit s2diskButtonPressed();
	} else {
		kdWarning() << "Session is not active, don't react on suspend2disk button event!" << endl;
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
	
	bool retval = false;
	
	checkCurrentBrightness();

	if (supportBrightness() && (getCurrentBrightnessLevel() >= 0) && 
            (getCurrentBrightnessLevel() != (getMaxBrightnessLevel()-1))) {
		int setTo = 0;
		int minPercStep = 10;
		int currentPerc = (int)(((float)getCurrentBrightnessLevel()/(float)(getMaxBrightnessLevel()-1))*100.0);

		if (percentageStep > 0 && (percentageStep <= (100-currentPerc))) {
			minPercStep = percentageStep;
		} 

		if ((currentPerc + minPercStep) > 100) {
			// set to 100 %
			setTo = getMaxBrightnessLevel() -1;
		} else {
			setTo = (int)(((float)(getMaxBrightnessLevel()-1))*(((float)(currentPerc + minPercStep))/100.0));
			if ((setTo == getCurrentBrightnessLevel()) && (setTo <  (getMaxBrightnessLevel() -1))) {
				setTo++;
			}
		}

		if (trace) {
			kdDebug() << "Max: " << getMaxBrightnessLevel() 
				  << " Current: " << getCurrentBrightnessLevel() 
				  << " minPercStep: " << minPercStep
				  << " currentPerc: " << currentPerc
				  << " setTo: " << setTo << endl;
		}
	
		retval = setBrightness(setTo, -1);
	}
 
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
bool HardwareInfo::setBrightnessDown(int percentageStep) {
	kdDebugFuncIn(trace);

	bool retval = false;

	checkCurrentBrightness();

	if (supportBrightness() && (getCurrentBrightnessLevel() > 0)) {
		int setTo = 0;
		int minPercStep = 10;
		int currentPerc = (int)(((float)getCurrentBrightnessLevel()/(float)(getMaxBrightnessLevel()-1))*100.0);

		if (percentageStep > 0 && (percentageStep < currentPerc)) {
			minPercStep = percentageStep;
		}

		if ((currentPerc - minPercStep) < 0) {
			setTo = 0;
		} else {
			setTo = (int)(((float)(getMaxBrightnessLevel()-1))*(((float)(currentPerc - minPercStep))/100.0));
			if ((setTo == getCurrentBrightnessLevel()) && (setTo > 0)) {
				setTo--;
			}
		}

		if (trace) {
			kdDebug() << "Max: " << getMaxBrightnessLevel() 
				  << " Current: " << getCurrentBrightnessLevel() 
				  << " minPercStep: " << minPercStep
				  << " currentPerc: " << currentPerc
				  << " setTo: " << setTo << endl;
		}

		retval = setBrightness(setTo, -1);
	}

	kdDebugFuncOut(trace);
	return retval;
}

/*!
 * Function to handle the signal for the brightness up button/key
 */
void HardwareInfo::brightnessUpPressed() {
	kdDebugFuncIn(trace);
	
	if (brightness) {
		if (!sessionIsActive) {
			kdWarning() << "Session is not active, don't react on brightness up key event!" << endl;
		} else {
			if (currentBrightnessLevel < availableBrightnessLevels) {
				setBrightnessUp();
			} else {
				kdWarning() << "Could not set brightness to higher level, it's already set to max." << endl;
			}
		}
	}
	kdDebugFuncOut(trace);
}

/*!
 * Function to handle the signal for the brightness down button/key
 */
void HardwareInfo::brightnessDownPressed() {
	kdDebugFuncIn(trace);

	if (brightness) {
		if (!sessionIsActive) {
			kdWarning() << "Session is not active, don't react on brightness down key event!" << endl;
		} else {
			if (currentBrightnessLevel > 0) {
				setBrightnessDown();
			} else {
				kdWarning() << "Could not set brightness to lower level, it's already set to min." << endl;
			}
		}
	}
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
	if (brightness)
		return availableBrightnessLevels;
	else
		return -1;
}

/*!
 * The function return the current brightness level
 * \return Integer with max level or -1 if not supported or unkown
 */
int HardwareInfo::getCurrentBrightnessLevel() const {
	if (brightness)
		return currentBrightnessLevel;
	else
		return -1;
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
	return (!dbus_terminated && !hal_terminated);
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
int HardwareInfo::isCpuFreqAllowed () {
	cpuFreqAllowed = dbus_HAL->isUserPrivileged( PRIV_CPUFREQ, HAL_COMPUTER_UDI);
	return cpuFreqAllowed;
}

/*! check if the org.freedesktop.Policy.Power interface has an owner
 * \return boolean with info if org.freedesktop.Policy.Power interface has an owner or not
 * \retval true 	if there is a owner
 * \retval false 	else
 */
bool HardwareInfo::isPolicyPowerIfaceOwned () {
	return dbus_HAL->isPolicyPowerIfaceOwned();
}


// --> get private members section -- END <---

#include "hardware.moc"
