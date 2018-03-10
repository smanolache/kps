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
	if (trace)
		kdDebug(debug_area) << funcinfo << "IN , udi: " << udi << endl;

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
		kps::upower_dev_remaining_time(
			modified, CHARGING == charging_state ?
			"TimeToFull" : "TimeToEmpty"));

	kdDebug(debug_area) << "Charging state changed: "
			    << charging_state_changed << ": "
			    << (charging_state == CHARGING ? "charging" :
				(charging_state == DISCHARGING ? "discharging"
				 : "unknown")) << endl;
	kdDebug(debug_area) << "Percentage changed: " << percentage_changed
			    << ": " << charge_level_percentage << endl;
	kdDebug(debug_area) << "State changed: " << state_changed << ": "
			    << (BAT_CRIT == state ? "critical" :
				(BAT_LOW == state ? "low" :
				 (BAT_WARN == state ? "warning" :
				  (BAT_NORM == state ? "normal" :
				   "none")))) << endl;
	kdDebug(debug_area) << "Rate changed: " << rate_changed << ": "
			    << present_rate << endl;
	kdDebug(debug_area) << "Remaining time changed: "
			    << remaining_time_changed << ": "
			    << remaining_minutes << endl;
	if (state_changed)
		emit changedBatteryWarnState(state);
	if (remaining_time_changed)
		emit changedBatteryTime();
	if (charging_state_changed)
		emit changedBatteryChargingState();
	if (percentage_changed)
		emit changedBatteryPercentage();
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

// ---> write private members SECTION : START <----

//! sets the chargelevel in percent when battery should go into state warning
void Battery::setWarnLevel(int _warn_level) {
	kdDebugFuncIn(trace);

	if (_warn_level < low_level) {
		kdError(debug_area) << "Refuse requested level: "
				    << _warn_level << " as it is smaller than "
			"the LowLevel: " << low_level << endl;
	} else {
		warn_level = _warn_level;
	}

	kdDebugFuncOut(trace);
}

//! sets the chargelevel in percent when battery should go into state low
void Battery::setLowLevel(int _low_level) {
	kdDebugFuncIn(trace);

	if (_low_level < crit_level || _low_level > warn_level) {
		kdWarning(debug_area) << "Refuse requested level: "
				      << _low_level << " as it is not between "
			"WarnLevel: " << warn_level << " and CritLevel: "
				      << crit_level << endl; 
	} else {
		low_level = _low_level;
	}

	kdDebugFuncOut(trace);
}

//! sets the chargelevel in percent when battery should go into state critical
void Battery::setCritLevel(int _crit_level) {
	kdDebugFuncIn(trace);

	if (_crit_level > low_level) {
		kdWarning(debug_area) << "Refuse requested level: "
				      << _crit_level << " as it is bigger than"
			" LowLevel: " << low_level << endl;
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
bool Battery::isCharging() const {
	return charging_state == CHARGING;
}

//! check if the battery gets currently discharged
bool Battery::isDischarging() const {
	return charging_state == DISCHARGING;
}

//! check it this is a primary battery
bool Battery::isPrimary() const {
	return (type == BAT_PRIMARY);
}

//! check if the battery state is ok/normal
bool Battery::isOk() const {
	return state == BAT_NORM;
}

//! check if the battery is in warning level/state
bool Battery::isWarning() const {
	return state == BAT_WARN;
}

//! check if the battery is in a low chargingstate
bool Battery::isLow() const {
	return state == BAT_LOW;
}

//! check if the battery level is critical
bool Battery::isCritical() const {
	return state == BAT_CRIT;
}
 
// ---> get private members SECTION : START <----

#include "hardware_battery.moc"
