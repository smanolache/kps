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
*  \file 	hardware_battery.h
*  \brief 	Headerfile for hardware_battery.cpp and the class \ref Battery.
*/
/*! 
*  \class 	Battery
*  \brief 	class for Battery related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2006-2007
*/

#ifndef _BATTERY_H_
#define _BATTERY_H_

// Global Header
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// QT - Header
#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>

// #include "dbusHAL.h"

#include "dbus_properties.hpp"
#include <optional>

enum BAT_TYPE {
	BAT_PRIMARY,
	BAT_MOUSE,
	BAT_KEYBOARD,
	BAT_KEY_MOUSE,
	BAT_UPS,
	BAT_CAMERA,
	BAT_UNKNOWN,
	LINE_POWER
};
enum BAT_CHARG_STATE {
	CHARGING,
	DISCHARGING,
	UNKNOWN_STATE
};
enum BAT_STATE {
	BAT_NONE,
	BAT_WARN,
	BAT_LOW,
	BAT_CRIT,
	BAT_NORM,
	BAT_HAL_ERROR
};

class Battery : public QObject {

	Q_OBJECT

private:
	//! Internal reference to dbusHAL for communication with HAL daemon
//	dbusHAL* dbus_HAL;

	//! HAL udi of the battery to represent
	/*!
	* This QString holds the HAL udi adress of the battery.
	* \li empty: all primary batteries are represented
	* \li e.g. "/org/freedesktop/Hal/devices/acpi_BAT0": first acpi battery
	*/
	QString udi;

	//! Technologyname of the battery
	/*!
	* Tells the type of technologyname the battery is working with.
	* \li e.g. "LION"
	*/
	QString technology;
	//! Current charging state of this battery as reported by HAL
	/*!
	* This QString tells the current capacity_state HAL is reporting.
	* \li as of current HAL spec: "ok", "critical"
	*/
	QString	capacity_state;
	//! Unit of charge_level_unit and charge_level_lastfull.
	/*! This QString tells the physical unit the values of charge_level_unit. */
	QString charge_level_unit;
	//! Serialnumber of the the battery
	/*! Identifies the currently installed battery uniquely. */
	QString	serial;

	//! Boolean which tells if the battery is initalized first time
	/*!
	* When the battery (represented by the object) is intialised the first time
	* (via recheck() or init()) this value is true.
	* \li true: 	if battery object is now intialised
	* \li false: 	if not
	*/
	// bool 	initialized;
	//! Boolean which tells if the battery is present/connected
	/*!
	* When the battery (represented by the object) is available this is true.
	* \li true: battery is available
	* \li false: battery is disconnected/not available
	*/
	// bool 	present;

	//! Roletype of battery
	/*!
	* This int/enum tells what role this battery is used as.
	* \li BAT_PRIMARY: Std. battery for normal system operation
	* \li BAT_MOUSE: powersupply for wireless mouse
	* \li BAT_KEYBOARD: powersupply in wireless keyboards
	* \li BAT_KEY_MOUSE: powersupply in combined keyboard+mouse gadgets
	* \li BAT_UPS: Battery in UPS systems (step in on outage of mains)
	* \li BAT_CAMERA: battery is contained in a connected digital camera
	* \li UNKNOWN: Batterytype/role isn't known
	*/
	int 	type;
	//! Current general state this battery is in
	/*!
	* This int/enum tells what rough state the battery is currently in.
	* \li BAT_NORM: batterylevel is ok
	* \li BAT_WARN: battery is soon getting low
	* \li BAT_LOW: batterylevel is already low
	* \li BAT_CRIT: batterylevel has become really critical
	* \li BAT_NONE: battery state not available
	* \li BAT_HAL_ERROR: battery state couldn't be retrieved because of a HAL error
	*/
	int 	state;
	//! Current charging state of this battery
	/*!
	* This int/enum tells if the battery is charged or discharged.
	* \li CHARGING: battery gets charged
	* \li DISCHARGING: battery get discharged
	* \li UNKNOWN_STATE: battery is neither charged nor discharged
	*/
	int 	charging_state;	
	//! Current level the battery is charged to
	/*!
	* This int tells (in physical units of Battery::charge_level_unit) 
	* at what charging level the battery is currently at 
	* \li a value >= 0
	*/
	int 	charge_level_current;
	//! Charging level of battery it could hold when fully charged
	/*!
	* This int tells (in physical units of Battery::charge_level_unit) the 
	* maximum charginglevel of the battery on its last fullcharge. 
	* \li a value >=0
	*/
	int	charge_level_lastfull;
	//! Current charge level of battery in percentage
	/*!
	* This int tells the current charge level of the battery in percent.
	* \li a value between 0 and 100
	*/
	int 	charge_level_percentage;
	//! The maximum capacity by design of the battery.
	/*!
	* This int tells (in physical units of Battery::charge_level_unit) 
	* the maximum capacity this battery was designed for by its vendor.
	* \li a value > 0
	*/
	int	design_capacity;
	//! Current charging/discharging rate
	/*!
	* This int tells (in physical units of Battery::charge_level_unit per
	* second) the currently reported charging/discharging rate.
	* \li a value >= 0
	*/
	int 	present_rate;
	//! Expected minutes unitl fully discharged/charged
	/*!
	* This int tells the current estimate until the battery is fully 
	* discharged/charged (with current discharging/charging-rate and last 
	* full capacity).
	* \li a value >= 0
	*/
	int	remaining_minutes;

	//! charge_level in percent that will put battery into warning state
	int warn_level;
	//! charge_level in percent that will put battery into low state
	int low_level;
	//! charge_level in percent that will put battery into critical state
	int crit_level;

	// private functions
	//! function to set initial values for a battery
	// void initDefault();

	//! to check battery.present
	// bool checkBatteryPresent();
	//! to check battery.type
	// bool checkBatteryType();
	//! to check battery.technology
	// bool checkBatteryTechnology();
	//! to check battery.charge_level.capacity_state
	// bool checkCapacityState();
	//! to check battery.charge_level.current
	// bool checkChargeLevelCurrent();
	//! to check battery.charge_level.last_full
	// bool checkChargeLevelLastfull();
	//! to check battery.charge_level.rate
	// bool checkChargeLevelRate();
	//! to check battery.charge_level.unit
	// bool checkChargeLevelUnit();
	//! to check battery.charge_level.design
	// bool checkChargeLevelDesign();
	//! to check battery.charge_level.percentage
	// bool checkRemainingPercentage();
	//! to check battery.remaining_time
	// bool checkRemainingTime();
	//! to check battery.rechargeable.is_*
	// bool checkChargingState();

	static enum BAT_STATE get_state(double, int crit, int low, int warn);
	bool update_charging_state(const std::optional<uint32_t>&);
	bool update_percentage(const std::optional<double>&);
	bool update_state();
	bool update_present_rate(const std::optional<double>&);
	bool update_remaining_time(const std::optional<int64_t>&);

	kps::dict_type props;
signals:
	//! emitted if the remaining percentage changed
	void changedBatteryPercentage();
	//! emitted if the remaining time changed
	void changedBatteryTime();
	//! emitted if the the present state changed
	void changedBatteryPresent();
	//! emitted if the charging state changed
	void changedBatteryChargingState();
	//! emitted if the battery state changed
	void changedBatteryState();
	//! emitted if the Battery warning state changed
	void changedBatteryWarnState (int state);
	//! emitted if any Battery state changed
	void changedBattery();
	
public:
	static enum BAT_CHARG_STATE get_charging_state(uint32_t s);
	static enum BAT_TYPE get_type(uint32_t t);

	//! default constructor
/*
	Battery( dbusHAL* _dbus_HAL, QString _udi );
*/
	//! default constructor
	Battery(const std::string& udi__, const kps::dict_type&);
	//! default constructor
/*
	Battery( dbusHAL* _dbus_HAL );
	//! this constructor forces the use of init with dbuHAL pointer set!
	Battery();
	//! default destructor
*/
	// ~Battery();

	//! initialize this battery object with values from HAL
//	void init();
	//! recheck all properties of the battery
	// void recheck();
	//! rechecks only minimalistic set properties
/*
	void minRecheck();
*/
	//! update a property on HAL event
	// bool updateProperty(QString _udi, QString _property);
	//! merge new values
	void update(const kps::dict_type&);

	//ro-Interface to internal data
	//! reports the HAL udi of this battery
	QString getUdi() const;
	//! reports HAL capacity_state value
	QString getCapacityState() const;
	//! reports the physical unit of values like DesignCapacity, PresentRate and Lastfull
	QString getChargelevelUnit() const;
	//! gives the name of this battery technology
	QString getTechnology() const;	

	//! get availability of this battery
	bool isPresent();

	//! reports the battery type
	int getType() const;
	//! tells the current batterystate as enum BAT_STATE_
	int getState() const;
	//! estimates the remaining minutes until fully charged/discharged
	int getRemainingMinutes() const;
	//! current charging/discharging rate 
	int getPresentRate() const;
	//! maximum capacity of this battery by design
	int getDesignCapacity() const;
	//! current charging state as enum BAT_CHARG_STATE
	int getChargingState() const;
	//! reports current chargelevel in percentage
	int getPercentage() const;
	//! reports last full capacity of this battery when fully charged
	int getLastfull() const;
	//! reports current chargelevel in units reported by getChargelevelUnit()
	int getCurrentLevel() const;
	
	//! reports the chargelevel in percent when battery goes to state warning
	int getWarnLevel() const;
	//! reports the chargelevel in percent when battery goes to state low
	int getLowLevel() const;
	//! reports the chargelevel in percent when battery goes to state critical
	int getCritLevel() const;

	//writeable access to internals
	//! Resets the current HAL udi used by the one given
	/*!
	* The given QString will be (checked and) used as new HAL udi for the battery.
	* But don't forget to do a recheck of the battery afterwards.
	* \li returns TRUE: if reset was successfull
	* \li returns FALSE: if reset couldn't be applied
	*/
//	bool resetUdi(QString);

	//! sets the chargelevel in percent when battery should go into state warning
	void setWarnLevel(int _warn_level);
	//! sets the chargelevel in percent when battery should go into state low
	void setLowLevel(int _low_level);
	//! sets the chargelevel in percent when battery should go into state critical
	void setCritLevel(int _crit_level);

	//some convenience methods
	//! check if the battery is currently charged
	bool isCharging();
	//! check if the battery gets currently discharged
	bool isDischarging();
	//! check it this is a primary battery
	bool isPrimary() const;
	//! check if the battery state is ok/normal
	bool isOk();
	//! check if the battery is in warning level/state
	bool isWarning();
	//! check if the battery is in a low chargingstate
	bool isLow();
	//! check if the battery level is critical
	bool isCritical();
};


#endif
