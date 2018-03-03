 /**************************************************************************
 *   Copyright (C) 2006-2007 by Danny Kukawka                              *
 *                           <dkukawka@suse.de>, <danny.kukawka@web.de>    *
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
 * \file 	hardware_battery.cpp
 * \brief 	In this file can be found the Battery related code.
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2006-2007
 */

#include "hardware_battery.h"

#include "UPowerProperties.hpp"
#include "dbus_properties.hpp"
#include <string>
#include "kpowersave_debug.h"

using kps::dict_type;

/*! The default constructor of the class Battery. */
/*
Battery::Battery(dbusHAL* _dbus_HAL) : dbus_HAL(_dbus_HAL) {
	kdDebugFuncIn(trace);

	udi = QString();
	initialized = false;
	
	initDefault();
	init(NULL);

	kdDebugFuncOut(trace);
}	
*/
/*! The default constructor of the class Battery. */ 
/*
Battery::Battery( dbusHAL* _dbus_HAL, QString _udi ) : dbus_HAL(_dbus_HAL), udi(_udi) {
	if (trace) kdDebug() << funcinfo << "IN , udi: " << udi << endl;
	
	initialized = false;

	initDefault();
	init(NULL);

	kdDebugFuncOut(trace);
}
*/
Battery::Battery(const std::string& udi__, const dict_type& props)
	: udi(udi__.c_str())
	, technology("UNKNOWN")
	, capacity_state("ok")
	, charge_level_unit("Wh")
	, serial("")
	, type(BAT_UNKNOWN)
	, state(BAT_NORM)
	, charging_state(UNKNOWN_STATE)
	, charge_level_current(0)
	, charge_level_lastfull(0)
	, charge_level_percentage(0)
	, design_capacity(0)
	, present_rate(0)
	, remaining_minutes(0)
	, warn_level(12)
	, low_level(7)
	, crit_level(2)
{
	if (trace) kdDebug() << funcinfo << "IN , udi: " << udi << endl;

	std::optional<std::string> serial__ = kps::upower_dev_serial(props);
	if (serial__)
		serial = *serial__;

	std::optional<uint32_t> type__ = kps::upower_dev_type(props);
	if (type__)
		type = get_type(*type__);

	std::optional<std::string> technology__ =
		kps::upower_dev_technology(props);
	if (technology__)
		technology = *technology__;

	std::optional<double> energy_rate__ =
		kps::upower_dev_energy_rate(props);
	if (energy_rate__)
		present_rate = *energy_rate__ < 0 ? 0.0 : *energy_rate__;

	std::optional<double> energy_full_design__ =
		kps::upower_dev_energy_full_design(props);
	if (energy_full_design__)
		design_capacity = *energy_full_design__ < 0 ?
			0.0 : *energy_full_design__;

	std::optional<double> percentage__ = kps::upower_dev_percentage(props);
	if (percentage__) {
		charge_level_percentage = *percentage__ < 0 ?
			0.0 : (*percentage__ > 100.0 ? 100.0 : *percentage__);
		state = get_state(charge_level_percentage, crit_level,
				  low_level, warn_level);
	}

	std::optional<uint32_t> state__ = kps::upower_dev_state(props);
	if (state__)
		charging_state = get_charging_state(*state__);

	std::optional<int64_t> remaining_time__ =
		kps::upower_dev_remaining_time(
			props, charging_state == CHARGING ? 
			"TimeToFull" : "TimeToEmpty");
	if (remaining_time__)
		remaining_minutes = *remaining_time__ >= 0 ?
			*remaining_time__ / 60 : 0ll;

	kdDebugFuncOut(trace);
}

enum BAT_STATE
Battery::get_state(double perc, int crit, int low, int warn) {
	if (perc <= crit)
		return BAT_CRIT;
	if (perc <= low)
		return BAT_LOW;
	if (perc <= warn)
		return BAT_WARN;
	return BAT_NORM;
}

enum BAT_CHARG_STATE
Battery::get_charging_state(uint32_t s) {
	switch (s) {
	default:
	case 0: // unknown
		 return UNKNOWN_STATE;
	case 1: // charging
	case 4: // fully charged
		return CHARGING;
	case 2: // discharging
	case 3: // empty
	case 5: // pending charge
	case 6: // pending discharge
		return DISCHARGING;
	}
}

enum BAT_TYPE
Battery::get_type(uint32_t t) {
	switch (t) {
	case 2:
		return BAT_PRIMARY;
	case 3:
		return BAT_UPS;
	case 5:
		return BAT_MOUSE;
	case 6:
		return BAT_KEYBOARD;
	case 8: // phone
		return BAT_CAMERA;
	case 1: // line power, not a battery
		return LINE_POWER;
	case 4: // monitor
	case 7: // pda
	case 0: // unknown
	default:
		return BAT_UNKNOWN;
	}
}

/*! The default constructor of the class Battery. */
/*
Battery::Battery() {
	kdDebugFuncIn(trace);
	initialized = false;

	initDefault();
	udi = QString();

	kdDebugFuncOut(trace);
}
*/
/*! The default destructor of the class Battery. */
/*
Battery::~Battery() {
	kdDebugFuncIn(trace);
	kdDebugFuncOut(trace);
}
*/
//! init a battery with default values
/*
void Battery::initDefault() {
	kdDebugFuncIn(trace);

	present = true;
	type = BAT_UNKNOWN;
	state = BAT_NORM;
	capacity_state = "ok";
	charging_state = UNKNOWN_STATE;
	charge_level_unit = "Wh";
	charge_level_current = 0;
	charge_level_lastfull = 0;
	charge_level_percentage = 0;
	design_capacity = 0;
	present_rate = 0;
	remaining_minutes = 0;
	serial = "";
	
	warn_level = 12;
	low_level = 7;
	crit_level = 2;
	
	kdDebugFuncOut(trace);
}
*/
//! initialize this battery object with values from HAL
/*
void Battery::init() {
	kdDebugFuncIn(trace);

	if (_dbus_HAL != NULL)
		dbus_HAL = _dbus_HAL;
	// read battery information from HAL
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		//couldnt connect to HAL
		state=BAT_HAL_ERROR;
	} else {
		if (resetUdi(udi)) {
			//udi is valid
			recheck(); //fills all values
			//ready here for now
		} else {
			//udi is invalid or is no battery
			state=BAT_HAL_ERROR;
			kdWarning() << "Warning: Battery::init cannot make use of udi " << udi << endl;
		}
	}

	initialized = true;
	kdDebugFuncOut(trace);
}
*/
//! rechecks only minimalistic set properties
/*!
* Will only recheck properties regarding current battery states/levels.
*/
/*
void Battery::minRecheck() {
	kdDebugFuncIn(trace);

	//first check whether HAL is available
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		//grr... no luck
		kdError() << "Battery::recheck couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return;
	}

	checkBatteryPresent();
	if (!present) {
		kdDebug() << "No need to update other properties, battery not present." << endl;
		kdDebugFuncOut(trace);
		return;
	} else {
		checkCapacityState();
		checkChargeLevelCurrent();
		checkRemainingPercentage();	
		checkChargingState();
		checkChargeLevelRate();
		checkRemainingTime();
	}

	kdDebugFuncOut(trace);
}
*/
//! recheck all properties of the battery
/*!
 * Check and set the properties of a battery or collect the information for all
 * primary batteries in one object.
 */
/*
void Battery::recheck() {
	kdDebugFuncIn(trace);

	//first check whether HAL is available
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		//grr... no luck
		kdError() << "Battery::recheck couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return;
	}

	checkBatteryPresent();
	checkBatteryType();

	if (!present) {
		kdDebug() << "Warning: No need to update properties, battery not present." << endl;
		kdDebugFuncOut(trace);
		return;
	} else {

		checkBatteryTechnology();
		checkCapacityState();
		checkChargeLevelCurrent();
		checkChargeLevelLastfull();
		checkRemainingPercentage();	
		checkChargingState();
		checkChargeLevelUnit();
		checkChargeLevelDesign();
		checkChargeLevelRate();
		checkRemainingTime();
	}

	kdDebugFuncOut(trace);
}
*/

void
Battery::update(const dict_type& modified) {
	bool charging_state_changed =
		update_charging_state(kps::upower_dev_state(modified));
	bool percentage_changed =
		update_percentage(kps::upower_dev_percentage(modified));
	bool state_changed = percentage_changed && update_state();
	bool rate_changed =
		update_present_rate(kps::upower_dev_energy_rate(modified));
	bool remaining_time_changed = update_remaining_time(
		kps::upower_dev_remaining_time(modified, CHARGING == state ?
					       "TimeToFull" : "TimeToEmpty"));
	if (state_changed)
		emit changedBatteryWarnState(state);
	if (remaining_time_changed)
		emit changedBatteryTime();
	if (charging_state_changed)
		emit changedBatteryChargingState();
	if (percentage_changed)
		emit changedBatteryPercentage();
	if (state_changed)
		emit changedBatteryWarnState(state);
	if (remaining_time_changed || charging_state_changed ||
	    percentage_changed || state_changed || rate_changed)
		emit changedBattery();
}

bool
Battery::update_charging_state(const std::optional<uint32_t>& s) {
	if (!s)
		return false;
	enum BAT_CHARG_STATE new_state = get_charging_state(*s);
	if (new_state == charging_state)
		return false;
	charging_state = new_state;
	return true;
}

bool
Battery::update_percentage(const std::optional<double>& p) {
	if (!p)
		return false;
	if (charge_level_percentage == static_cast<int>(*p))
		return false;
	charge_level_percentage = static_cast<int>(*p);
	return true;
}

bool
Battery::update_present_rate(const std::optional<double>& r) {
	if (!r)
		return false;
	if (present_rate == static_cast<int>(*r))
		return false;
	present_rate = static_cast<int>(*r);
	return true;
}

bool
Battery::update_state() {
	enum BAT_STATE new_state = BAT_NONE;
	if (charge_level_percentage <= crit_level)
		new_state = BAT_CRIT;
	else if (charge_level_percentage <= low_level)
		new_state = BAT_LOW;
	else if (charge_level_percentage <= warn_level)
		new_state = BAT_WARN;
	else if (BAT_NONE != state)
		new_state = BAT_NORM;

	if (state == new_state)
		return false;
	state = new_state;
	return true;
}

bool
Battery::update_remaining_time(const std::optional<int64_t>& r) {
	if (!r)
		return false;
	int64_t m = *r >= 0 ? *r / 60 : 0ll;
	if (remaining_minutes == m)
		return false;
	remaining_minutes = m;
	return true;
}

// ---> query HAL for properties SECTION : START <----

//! to check battery.present
/*!
 * function to update \ref present from property battery.present
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkBatteryPresent () {
	kdDebugFuncIn(trace);

	bool _present = false;
	
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (dbus_HAL->halGetPropertyBool(udi,"battery.present", &_present)) {
		if (_present != present) {
			present = _present;
			
			// force depending changes
			if (_present) {
				// should be now present
				recheck();
			}
			if (!_present) {
				//Uhhh, battery is not present, so we need to reset battery to default.
				initDefault();
				checkBatteryType();
				state = BAT_NONE;
			}

			if (initialized) {
				emit changedBatteryPresent();
				emit changedBattery();
			}
		}

		// also query the serial ...  no need to do this in a extra place
		dbus_HAL->halGetPropertyString(udi, "battery.serial", &serial);

	} else {
		//query was not successfull
		kdDebug() << "Query of battery.present of " << udi << " wasn't successfull." << endl;
		//..but we assume its there try on
		present = true;
	}
	
	kdDebugFuncOut(trace);
	return true;
}
*/

//! to check battery.type
/*!
 * function to update \ref type from property battery.type
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkBatteryType () {
	kdDebugFuncIn(trace);
	
	QString tmp_qstring;

	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (dbus_HAL->halGetPropertyString(udi,"battery.type", &tmp_qstring)) {
		if (tmp_qstring.compare("primary") == 0) {
			type = BAT_PRIMARY;
		} else if (tmp_qstring.compare("mouse") == 0) {
			type = BAT_MOUSE;
		} else if (tmp_qstring.compare("keyboard") == 0) {
			type = BAT_KEYBOARD;
		} else if (tmp_qstring.compare("keyboard_mouse") == 0) {
			type = BAT_KEY_MOUSE;
		} else if (tmp_qstring.compare("camera") == 0) {
			type = BAT_CAMERA;
		} else if (tmp_qstring.compare("ups") == 0) {
			type = BAT_UPS;
		} else {
			//anything else will currently be "UNKNOWN"
			type = BAT_UNKNOWN;
		}
		kdDebugFuncOut(trace);
		return true;
	} else {
		//query was not successfull
		kdWarning() << "Query of battery.type of " << udi << " was not successfull." << endl;
		type = BAT_UNKNOWN;
		kdDebugFuncOut(trace);
		return false;
	}
}
*/

//! to check battery.technology
/*!
 * function to update \ref technology from property battery.technology
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkBatteryTechnology () {
	kdDebugFuncIn(trace);
	
	QString tmp_qstring;
	
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!present) {
		kdWarning() << "No need to update property, battery not present." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (dbus_HAL->halGetPropertyString(udi,"battery.technology", &tmp_qstring)) {
		if (!tmp_qstring.isEmpty()) {
			technology = QString(tmp_qstring);
		} else {
			technology = QString("UNKNOWN");
		}
		kdDebugFuncOut(trace);
		return true;
	} else {
		//query was not successfull, but this property is optional
		technology = QString("UNKNOWN");
		kdDebugFuncOut(trace);
		return false;
	}
}
*/


//! to check battery.charge_level.capacity_state
/*!
 * function to update \ref capacity_state from battery.charge_level.capacity_state
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkCapacityState () {
	kdDebugFuncIn(trace);

	QString tmp_qstring;
	
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!present) {
		kdWarning() << "No need to update property, battery not present." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (dbus_HAL->halGetPropertyString(udi, "battery.charge_level.capacity_state", &tmp_qstring)) {
		capacity_state = QString(tmp_qstring);
		kdDebugFuncOut(trace);
		return true;
	} else {
		//query was not successfull, but this property is optional
		capacity_state = QString();
		kdDebugFuncOut(trace);
		return false;
	}
}
*/


//! to check battery.charge_level.current
/*!
 * function to update \ref charge_level_current from property battery.charge_level.current
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkChargeLevelCurrent () {
	kdDebugFuncIn(trace);
	
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!present) {
		kdWarning() << "No need to update property, battery not present." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (dbus_HAL->halGetPropertyInt(udi, "battery.charge_level.current", &charge_level_current)) {
		if (charge_level_current < 0) {
			//overflow?
			charge_level_current = 0;
		}
		kdDebugFuncOut(trace);
		return true;
	} else {
		//query was not successfull (is mandatory)
		kdError() << "Couldn't request charge_level.current for udi: " << udi << endl;
		state = BAT_NONE;
		kdDebugFuncOut(trace);
		return false;
	}
}
*/


//! to check battery.charge_level.last_full
/*!
 * function to update \ref charge_level_lastfull from property battery.charge_level.last_full
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkChargeLevelLastfull () {
	kdDebugFuncIn(trace);
	
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!present) {
		kdWarning() << "No need to update property, battery not present." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (dbus_HAL->halGetPropertyInt(udi, "battery.charge_level.last_full", &charge_level_lastfull)) {
		if (charge_level_lastfull < charge_level_current ) {
			//possible overflow?
			charge_level_lastfull = charge_level_current;
		}
		kdDebugFuncOut(trace);
		return true;
	} else {
		//query was not successfull (is mandatory)
		kdError() << "couldn't query last_full of udi: " << udi << endl;
		charge_level_lastfull = 0; // set back to 0
		kdDebugFuncOut(trace);
		return false;
	}
}
*/

//! to check battery.charge_level.rate
/*!
 * function to update \ref present_rate from property battery.charge_level.rate
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkChargeLevelRate () {
	kdDebugFuncIn(trace);
	
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError()  << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!present) {
		kdWarning() << "No need to update property, battery not present." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	int _rate = present_rate;

	if (dbus_HAL->halGetPropertyInt(udi, "battery.charge_level.rate", &present_rate)) {
		if (present_rate < 0 )
			present_rate = 0;

		if (present_rate != _rate)
			emit changedBattery();

		kdDebugFuncOut(trace);
		return true;
	} else {
		//query was not successfull but this is optional
		kdError() << "Couldn't request charge_level.rate for udi: " <<  udi << endl;
		present_rate = 0;
		kdDebugFuncOut(trace);
		return false;
	}
}
*/


//! to check battery.charge_level.unit
/*!
 * function to update \ref charge_level_unit from property battery.charge_level.unit
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkChargeLevelUnit () {
	kdDebugFuncOut(trace);
	
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!present) {
		kdWarning() << "No need to update property, battery not present." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!dbus_HAL->halGetPropertyString(udi, "battery.charge_level.unit", &charge_level_unit)) {
		//query was not successfull but this is optional
		kdWarning() << "Couldn't request charge_level.unit for udi: " << udi << endl;
		kdDebugFuncOut(trace);
		return false;
	} else {
		kdDebugFuncOut(trace);
		return true;
	}
}
*/

//! to check battery.charge_level.design
/*!
 * function to update \ref design_capacity from property battery.charge_level.design
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkChargeLevelDesign () {
	kdDebugFuncIn(trace);
	
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!present) {
		kdWarning() << "No need to update property, battery not present." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (dbus_HAL->halGetPropertyInt(udi, "battery.charge_level.design", &design_capacity)) {
		if (design_capacity < 0)
			design_capacity = 0;

		kdDebugFuncOut(trace);
		return true;
	} else {
		//query was not successfull (is mandatory but not critical)
		kdWarning() << "Couldn't request charge_level.design for udi: " << udi << endl;
		kdDebugFuncOut(trace);
		return false;
	}
}
*/


//! to check battery.charge_level.percentage
/*!
 * function to update \ref charge_level_percentage from property battery.charge_level.percentage
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkRemainingPercentage () {
	kdDebugFuncIn(trace);
	
	bool ret = false;
	int _val = 0;
	int _state = -1;

	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!present) {
		kdWarning() << "No need to update property, battery not present." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (dbus_HAL->halGetPropertyInt(udi, "battery.charge_level.percentage", &_val)) {
		if (_val > 100) {
			_val = 100;
		} else if (_val < 0) {
			_val = 0;
		}
		ret = true;
	} else {
		//query was not successfull, but this property is optional
		//so we calculate it from charge_level
		if (charge_level_current > 0) {
			_val = (int)((float)(charge_level_current * 100) / (float)charge_level_lastfull);
			ret = true;
		} else {
			kdError() << "Couldn't query percentage of udi: " << udi 
				  << ". and charge_level_current >= 0" << endl;
		}
	}

	if (charge_level_percentage != _val) {
		if (initialized) {
			emit changedBatteryPercentage();
			emit changedBattery();
		}
		charge_level_percentage = _val;
	}

	//battery state
	if (charge_level_percentage <= crit_level) {
		//we are now in critical level
		_state = BAT_CRIT;
	} else if (charge_level_percentage <= low_level) {
		//we are now in a low level
		_state = BAT_LOW;
	} else if (charge_level_percentage <= warn_level) {
		//we are now in warning state
		_state = BAT_WARN;
	} else if (state != BAT_NONE) {
		_state = BAT_NORM;
	} else {
		_state = BAT_NONE;
	}
	
	if (state != _state) {
		if (initialized) {
			if (_state == (BAT_CRIT || BAT_LOW || BAT_WARN))
				emit changedBatteryWarnState(_state);
			else if (state == (BAT_CRIT || BAT_LOW || BAT_WARN))
				emit changedBatteryWarnState(_state);
			else 
				emit changedBatteryState();

			emit changedBattery();
		}
		state = _state;
	}

	kdDebugFuncOut(trace);
	return ret;
}
*/

//! to check battery.remaining_time
/*!
 * function to update \ref remaining_minutes from property battery.remaining_time
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkRemainingTime () {
	kdDebugFuncIn(trace);
	
	int _min = 0;
	bool _ret = false;	

	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!present) {
		kdWarning() << "No need to update property, battery not present." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (dbus_HAL->halGetPropertyInt(udi, "battery.remaining_time", &_min)) {
		_min /= 60;
		_ret = true;
	} else {
		//query was not successfull, but this is optional 
		//and even very unlikely to be present

		//try to get it via charge_level_current and present_rate
		if (charge_level_current > 0 && present_rate > 0) {
			_min = ((charge_level_current * 60 )/ present_rate) ;
			_ret = true;
		} else  {
			//we don't know it better
			_min = 0;
			_ret = false;
		}
	}

	if (remaining_minutes != _min) {
		if (initialized) {
			emit changedBatteryTime();
			emit changedBattery();
		}

		remaining_minutes = _min;
	}

	kdDebugFuncOut(trace);
	return _ret;
}
*/

//! to check battery.rechargeable.is_*
/*!
 * function to update \ref charging_state from property battery.rechargeable.is_*
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::checkChargingState () {
	kdDebugFuncIn(trace);
	
	bool tmp_bool = false;
	bool tmp_bool2 = false;
	bool _ret = false;
	int _c_state = -1;

	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		kdError() << "Couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (!present) {
		kdWarning() << "No need to update property, battery not present." << endl;
		kdDebugFuncOut(trace);
		return false;
	}

	if (dbus_HAL->halGetPropertyBool(udi, "battery.rechargeable.is_charging", &tmp_bool) && 
	    dbus_HAL->halGetPropertyBool(udi, "battery.rechargeable.is_discharging", &tmp_bool2)) {
		if (tmp_bool && !tmp_bool2) {
			_c_state = CHARGING;
		} else if (tmp_bool2 && !tmp_bool) {
			_c_state = DISCHARGING;
		} else {
			_c_state = UNKNOWN_STATE;
		}
		
		_ret = true;
	} else {
		kdError() << "Couldn't get current charging state for udi: " << udi << endl;
		_c_state = UNKNOWN_STATE;
		_ret = false;
	}

	if (charging_state != _c_state) {
		if (initialized) {
			emit changedBatteryChargingState();
			emit changedBattery();
		}

		charging_state = _c_state;
	}

	kdDebugFuncOut(trace);
	return _ret;	
}
*/

// ---> query HAL for properties SECTION : END <----

//! to recheck a special value for a HAL event
/*!
 * Check for the given property new values from HAL and set them to 
 * the battery variables.
 * \param _udi		QString with the UDI of the device to recheck
 * \param _property	QString with the property which was changed
 * \return boolean with the result of the operation
 * \retval true 	if the update was successfull
 * \retval false 	if the update couldn't be applied
 */
/*
bool Battery::updateProperty(QString _udi, QString _property) {
	kdDebugFuncIn(trace);
	
	//first check whether HAL is available
	if (!dbus_HAL->isConnectedToHAL() && !dbus_HAL->reconnect()) {
		//grr... no luck
		kdError() << "Battery::updateProperty couldn't connect to HAL" << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	
	bool ret = true;

	if (udi.startsWith(_udi)) {
		// handle different keys and update only the needed
		if (_property.startsWith("battery.present")) {
			if (!checkBatteryPresent()) ret = false;
		} else if (_property.startsWith("battery.type")) {
			if (!checkBatteryType()) ret = false;
		} else if (_property.startsWith("battery.charge_level.capacity_state")) {
			if (!checkCapacityState()) ret=  false;
		} else if (_property.startsWith("battery.charge_level.current")) {
			if (!checkChargeLevelCurrent()) ret = false;
		} else if (_property.startsWith("battery.charge_level.rate")) {
			if (!checkChargeLevelRate()) ret = false;
		} else if (_property.startsWith("battery.charge_level.percentage")) {
			if (!checkRemainingPercentage()) ret = false;
		} else if (_property.startsWith("battery.remaining_time")) {
			if (!checkRemainingTime()) ret = false;
		} else if (_property.startsWith("battery.rechargeable.is_")) {
			if (!checkChargingState()) ret = false;
		} else if (_property.startsWith("battery.charge_level.last_full")) {
			if (!checkChargeLevelLastfull()) ret = false;
	// --> changes on this keys should normally not happen except if battery was added /removed
		} else if (_property.startsWith("battery.technology")) {
			if (!checkBatteryTechnology()) ret = false;
		} else if (_property.startsWith("battery.charge_level.unit")) {
			if (!checkChargeLevelUnit()) ret = false;
		} else if (_property.startsWith("battery.charge_level.design")) {
			if (!checkChargeLevelDesign()) ret = false;
		} else {
			kdDebug() << "Unknown or unhandled device property: " << _property 
				  << " for device with udi: " << _udi << endl;
			ret = false;
		}
	} else {
		kdError() << "Given UDI doesn't match the UDI of this battery object." << endl;
		ret = false;
	}

	kdDebugFuncOut(trace);
	return ret;
}
*/
//! Resets the current HAL udi used by the one given
/*!
* The given QString will be (checked and) used as new HAL udi for the battery.
* But don't forget to do a recheck of the battery afterwards.
* \param _udi 		QString with the UDI to reset
* \return boolean with the result of the operation
* \retval true 		if reset was successfull
* \retval false 	if reset couldn't be applied
*/
/*
bool Battery::resetUdi(QString _udi) {
	kdDebugFuncIn(trace);

	bool tmp_result=false;

	//trivial pre-check to eliminate totally dumb _udi strings
	if (!_udi.isNull() && !_udi.isEmpty() && _udi.startsWith("/org/freedesktop/Hal/devices/")) {
		
		//ok, now lets ask HAL if its really a battery
		if (dbus_HAL->isConnectedToHAL() || dbus_HAL->reconnect()) {

			dbus_HAL->halQueryCapability(_udi,"battery",&tmp_result);

		} else {
			kdError() << "Battery::resetUdi couldn't connect to HAL" << endl;
		}

	} else {
		kdError() << "Battery::resetUdi received empty or invalid udi" << endl;
	}

	kdDebugFuncOut(trace);
	return tmp_result;
}
*/
// ---> write private members SECTION : START <----

//! sets the chargelevel in percent when battery should go into state warning
void Battery::setWarnLevel(int _warn_level) {
	kdDebugFuncIn(trace);

	if (_warn_level < low_level) {
		kdError() << "Refuse requested level: " << _warn_level 
			  << " as it is smaller than the LowLevel: " << low_level << endl;
	} else {
		warn_level = _warn_level;
	}

	kdDebugFuncOut(trace);
}

//! sets the chargelevel in percent when battery should go into state low
void Battery::setLowLevel(int _low_level) {
	kdDebugFuncIn(trace);

	if (_low_level < crit_level || _low_level > warn_level) {
		kdError() << "Refuse requested level: " << _low_level 
			  << " as it is not between WarnLevel: " << warn_level
			  << " and CritLevel: " << crit_level << endl; 
	} else {
		low_level = _low_level;
	}

	kdDebugFuncOut(trace);
}

//! sets the chargelevel in percent when battery should go into state critical
void Battery::setCritLevel(int _crit_level) {
	kdDebugFuncIn(trace);

	if (_crit_level > low_level) {
		kdError() << "Refuse requested level: " << _crit_level
			  << " as it is bigger than LowLevel: " << low_level << endl;
	} else {
		crit_level = _crit_level;
	}

	kdDebugFuncOut(trace);
}

// ---> write private members SECTION : END <----
// ---> get private members SECTION : START <----

//! reports the HAL udi of this battery
QString Battery::getUdi() const {
	return QString(udi);
}

//! reports the battery type
int Battery::getType() const {
	return type;
}

//! gives the name of this battery technology
QString Battery::getTechnology() const {
	return QString(technology);
}

//! tells the current batterystate as enum BAT_STATE_
int Battery::getState() const {
	return state;
}

//! estimates the remaining minutes until fully charged/discharged
int Battery::getRemainingMinutes() const {
	return remaining_minutes;
}

//! current charging/discharging rate 
int Battery::getPresentRate() const {
	return present_rate;
}

//! get availability of this battery
bool Battery::isPresent() {
	return true;
}

//! maximum capacity of this battery by design
int Battery::getDesignCapacity() const {
	return design_capacity;
}

//! current charging state as enum BAT_CHARG_STATE
int Battery::getChargingState() const {
	return charging_state;
}

//! reports the physical unit of values like DesignCapacity, PresentRate and Lastfull
QString Battery::getChargelevelUnit() const {
	return QString(charge_level_unit);
}

//! reports current chargelevel in percentage
int Battery::getPercentage() const {
	return charge_level_percentage;
}

//! reports last full capacity of this battery when fully charged
int Battery::getLastfull() const {
	return charge_level_lastfull;
}

//! reports current chargelevel in units reported by getChargelevelUnit()
int Battery::getCurrentLevel() const {
	return charge_level_current;
}

//! reports HAL capacity_state value
QString Battery::getCapacityState() const {
	return QString(capacity_state);
}

//! reports the chargelevel in percent when battery goes to state warning
int Battery::getWarnLevel() const {
	return warn_level;
}

//! reports the chargelevel in percent when battery goes to state low
int Battery::getLowLevel() const {
	return low_level;
}

//! reports the chargelevel in percent when battery goes to state critical
int Battery::getCritLevel() const {
	return crit_level;
}

//some convenience methods

//! check if the battery is currently charged
bool Battery::isCharging() {
	return (charging_state == CHARGING);
}

//! check if the battery gets currently discharged
bool Battery::isDischarging() {
	return (charging_state == DISCHARGING);
}

//! check it this is a primary battery
bool Battery::isPrimary() const {
	return (type == BAT_PRIMARY);
}

//! check if the battery state is ok/normal
bool Battery::isOk() {
	return (state == BAT_NORM);
}

//! check if the battery is in warning level/state
bool Battery::isWarning() {
	return (state == BAT_WARN);
}

//! check if the battery is in a low chargingstate
bool Battery::isLow() {
	return (state == BAT_LOW);
}

//! check if the battery level is critical
bool Battery::isCritical() {
	return (state == BAT_CRIT);
}
 
// ---> get private members SECTION : START <----

#include "hardware_battery.moc"
