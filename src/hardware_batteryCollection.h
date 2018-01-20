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
*  \file 	hardware_batteryCollection.h
*  \brief 	Headerfile for hardware_batteryCollection.cpp and the class \ref BatteryCollection.
*/
/*! 
*  \class 	BatteryCollection
*  \brief 	class to collect batteryinformation for a special type of battries
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2006-2007
*/

#ifndef _BATTERYCOLLECTION_H_
#define _BATTERYCOLLECTION_H_

// Global Header
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// QT - Header
#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>

// own Header
#include "hardware_battery.h"

class BatteryCollection : public QObject {

	Q_OBJECT

private:

	//! contains the udis of the batteries of this collection
	QStringList udis;
	
	//! contains the rate unit 
	QString present_rate_unit;

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
	int type;
	//! Current charging state of the active battery of this collection
	/*!
	* This int/enum tells if the battery is charged or discharged.
	* \li CHARGING: battery gets charged
	* \li DISCHARGING: battery get discharged
	* \li UNKNOWN_STATE: battery is neither charged nor discharged
	*/
	int charging_state;
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
	int state;

	//! Current charge level of battery in percentage
	/*!
	* This int tells the current charge level of the battery in percent.
	* \li a value between 0 and 100
	*/
	int remaining_percent;
	//! Expected minutes unitl fully discharged/charged
	/*!
	* This int tells the current estimate until the battery is fully 
	* discharged/charged (with current discharging/charging-rate and last 
	* full capacity).
	* \li a value >= 0
	*/
	int remaining_minutes;

	//! number of present batteries
	/*!
	* This int tells how many batteries of this type are really present. 
	* This mean only batteries and not battery slots/bays.
	* \li a value >= 0
	*/
	int present_batteries;

	//! number of present batteries
	/*!
	* This int tells the current rate of the batteries
	* \li a value >= 0
	*/
	int present_rate;

	//! charge_level in percent that will put battery into warning state
	int warn_level;
	//! charge_level in percent that will put battery into low state
	int low_level;
	//! charge_level in percent that will put battery into critical state
	int crit_level;
	
	//! init the battery collection with a default value
	void initDefault();

signals:
	
	//! emitted if we switch to a warning state
	/*!
	* The first int tell the battery type and the second which warning state we reached:
	* \li BAT_NORM: batterylevel is ok ... only emitted if we return form BAT_WARN
	* \li BAT_WARN: battery is soon getting low
	* \li BAT_LOW: batterylevel is already low
	* \li BAT_CRIT: batterylevel has become really critical
	*/
	void batteryWarnState (int type, int state);
	//! emitted if the charging state changed
	void batteryChargingStateChanged (int changing_state);
	//! emitted if the remainig percentage changed
	void batteryPercentageChanged (int percent);
	//! emitted if the remainig minutes changed
	void batteryMinutesChanged (int minutes );
	//! emitted if the number of present batteries changed
	void batteryPresentChanged (int num );
	//! emitted if the present rate changed
	void batteryRateChanged ();
	//! emitted if any Battery state changed
	void batteryChanged();

public:
	//! default constructor
	BatteryCollection( int type );
	//! default destructor
	~BatteryCollection();

	// functions
	//! refresh the information of the collection from the given batterylist
	bool refreshInfo(QPtrList<Battery> BatteryList, bool force_level_recheck = false); 
	//! check if this collection already handle a special battery/udi
	bool isBatteryHandled(QString udi);

	// get internals
	//! get the unit for charge level stuff
	QString getChargeLevelUnit() const;

	//! get the cumulative remaining time
	int getRemainingMinutes() const;
	//! get the cumulative remaining percentage of the battery capacity
	int getRemainingPercent() const;
	//! get the current Charging state of the machine
	int getChargingState() const;
	//! get the current battery state
	int getBatteryState() const;
	//! get the number of available batteries
	int getNumBatteries() const;
	//! get the number of present batteries
	int getNumPresentBatteries() const;
	//! get the battery Type from enum \ref BAT_TYPE 
	int getBatteryType() const;
	//! get the current battery rate
	int getCurrentRate() const;

	//! reports the chargelevel in percent when battery goes to state warning
	int getWarnLevel() const;
	//! reports the chargelevel in percent when battery goes to state low
	int getLowLevel() const;
	//! reports the chargelevel in percent when battery goes to state critical
	int getCritLevel() const;
	
	//! sets the chargelevel in percent when battery should go into state warning
	bool setWarnLevel(int _warn_level);
	//! sets the chargelevel in percent when battery should go into state low
	bool setLowLevel(int _low_level);
	//! sets the chargelevel in percent when battery should go into state critical
	bool setCritLevel(int _crit_level);
	
};

#endif
