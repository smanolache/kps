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
 * \file 	hardware_batteryCollection.cpp
 * \brief 	In this file can be found the Battery Collection related code.
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2006-2007
 */

// own headers
#include "hardware_batteryCollection.h"

/*! The default constructor of the class BatteryCollection. */
BatteryCollection::BatteryCollection( int _type ) {
	kdDebugFuncIn(trace);

	initDefault();
	type = _type;

	kdDebugFuncOut(trace);
}


/*! The default destructor of the class BatteryCollection. */
BatteryCollection::~BatteryCollection( ) {
	kdDebugFuncIn(trace);

}

//! init a battery with default values
void BatteryCollection::initDefault() {
	kdDebugFuncIn(trace);

	udis.clear();
	
	present_rate_unit = "mWh";

	charging_state = UNKNOWN_STATE;
	state = BAT_NORM;
	remaining_percent = -1;
	remaining_minutes = -1;
	present_rate = 0;
	
	warn_level = 12;
	low_level = 7;
	crit_level = 2;

	kdDebugFuncOut(trace);
}

/*!
 * This function refresh the information of the collection from the given 
 * batterylist.
 * \param BatteryList		QPtrList with battery objects
 * \param force_level_recheck	boolean with info if the the check for the current 
 *				battery warning level should get forced
 */ 
bool BatteryCollection::refreshInfo(QPtrList<Battery> BatteryList, bool force_level_recheck) {
	kdDebugFuncIn(trace);

	int _charging_state = UNKNOWN_STATE;
	int _percent = 0;
	int _minutes = 0;
	int _present_batteries = 0;
	int _present_rate = 0;

	// for now: clean list before run update process!
	udis.clear();

	if (!BatteryList.isEmpty()) {
		Battery *bat;
		for (bat = BatteryList.first(); bat; bat = BatteryList.next() ) {
			if (type == bat->getType()) {
				udis.append(bat->getUdi());

				if (bat->isPresent()) {
					// count present batteries
					_present_batteries++;
				
					// handle charging state
					if (bat->getChargingState() != _charging_state) {
						if (_charging_state == UNKNOWN_STATE) {
							_charging_state = bat->getChargingState();
						} else if ( bat->getChargingState() == UNKNOWN_STATE) {
							kdWarning()  << "found battery with unknown state,"
								     << " do nothing" << endl;
						} else {
							
							if (_charging_state != bat->getChargingState()) {
								// This should only happen if one is in 
								// state CHARGING and the other in DISCHARGING
								kdWarning() << "Unexpected chargingstates" << endl;
								_charging_state = UNKNOWN_STATE;
							}
						}
					} 

					// handle remaining percentage
					if (bat->getPercentage() >= 0) {
						_percent = (_percent + bat->getPercentage()) / _present_batteries;
					}
					
					if (bat->getRemainingMinutes() >= 0) {
						_minutes += bat->getRemainingMinutes();
					}
					
					if (bat->getPresentRate() >= 0) {
						_present_rate += bat->getPresentRate();
					}
					
					if (!bat->getChargelevelUnit().isEmpty()) {
						present_rate_unit = bat->getChargelevelUnit();
					}
				}
			}
		}

		bool _changed = false;

		if (_charging_state != charging_state) {
			charging_state = _charging_state;
			_changed = true;
			emit batteryChargingStateChanged (charging_state);
		}
		if (_percent != remaining_percent || force_level_recheck) {
			remaining_percent = _percent;

			if (_present_batteries < 1) {
				/* there are no batteries present, we don't need to emit
				   a event, there is nothing ... */
				state = BAT_NONE;
			}else if (remaining_percent <= crit_level) {
				if (state != BAT_CRIT) {
					state = BAT_CRIT;
					emit batteryWarnState( type, BAT_CRIT );
				}
			} else if (remaining_percent <= low_level) {
				if (state != BAT_LOW) {
					state = BAT_LOW;
					emit batteryWarnState( type, BAT_LOW );
				}
			} else if (remaining_percent <= warn_level) {
				if (state != BAT_WARN) {
					state = BAT_WARN;
					emit batteryWarnState( type, BAT_WARN );
				}
			} else if (state != BAT_NONE) {
				if (state != BAT_NORM) {
					state = BAT_NORM;
					emit batteryWarnState( type, BAT_NORM );
				}
			} else {
				state = BAT_NONE;
			}

			_changed = true;
			emit batteryPercentageChanged (remaining_percent );
		}
		if (_minutes != remaining_minutes) {
			remaining_minutes = _minutes;
			_changed = true;
			emit batteryMinutesChanged( remaining_minutes );
		}
		if (_present_batteries != present_batteries) {
			present_batteries = _present_batteries;
			_changed = true;
			emit batteryPresentChanged ( present_batteries );
		}
		if (_present_rate != present_rate ) {
			present_rate = _present_rate;
			// don't set to changed, this avoid useless calls
			emit batteryRateChanged ();
		}

		if (_changed) 
			emit batteryChanged();

		kdDebugFuncOut(trace);
		return true;
	} else {
		kdError() << "Could not refresh battery information, BatteryList was empty" << endl;
		initDefault();
		kdDebugFuncOut(trace);
		return false;
	}
}

//! check if the given udi is already handled by this collection
bool BatteryCollection::isBatteryHandled( QString udi ) {
	return udis.contains( udi );
}

// ---> write private members SECTION : START <----
//! get the unit for charge level stuff
QString BatteryCollection::getChargeLevelUnit() const {
	return present_rate_unit;
}

//! get the current reported battery rate
int BatteryCollection::getCurrentRate() const {
	return present_rate;
}

//! get the cumulative remaining time
int BatteryCollection::getRemainingMinutes() const {
	return remaining_minutes;
}

//! get the cumulative remaining percentage of the battery capacity
int BatteryCollection::getRemainingPercent() const {
	return remaining_percent;
}

//! get the current Charging state of the machine
int BatteryCollection::getChargingState() const {
	return charging_state;
}

//! get the current battery state for this collection
int BatteryCollection::getBatteryState() const {
	return state;
}

//! get the number of available batteries 
int BatteryCollection::getNumBatteries() const {
	return udis.count();
}

//! get the number of present batteries, represent \ref present_batteries
int BatteryCollection::getNumPresentBatteries() const {
	return present_batteries;
}

//! get the battery Type from enum \ref BAT_TYPE 
int BatteryCollection::getBatteryType() const {
	return type;
}

//! sets the chargelevel in percent when battery should go into state warning
/*!
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool BatteryCollection::setWarnLevel(int _warn_level) {
	kdDebugFuncIn(trace);

	if (_warn_level < low_level) {
		kdError() << "Refuse: " << _warn_level 
			  << " as it is smaller than the LowLevel: " << low_level << endl;
		kdDebugFuncOut(trace);
		return false;
	} else {
		warn_level = _warn_level;
		kdDebugFuncOut(trace);
		return true;
	}
}

//! sets the chargelevel in percent when battery should go into state low
/*!
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool BatteryCollection::setLowLevel(int _low_level) {
	kdDebugFuncIn(trace);

	if (_low_level < crit_level || _low_level > warn_level) {
		kdError() << "Refuses: " << _low_level
 			  << " as it is not between WarnLevel: " << warn_level
			  << " and CritLevel: " << crit_level << endl; 
		kdDebugFuncOut(trace);
		return false;
	} else {
		low_level = _low_level;
		kdDebugFuncOut(trace);
		return true;
	}
}

//! sets the chargelevel in percent when battery should go into state critical
/*!
 * \return boolean with result of the operation
 * \retval true  	if successful
 * \retval false 	else, if a error occurs
 */
bool BatteryCollection::setCritLevel(int _crit_level) {
	kdDebugFuncIn(trace);

	if (_crit_level > low_level) {
		kdError() << "Refuses " << _crit_level 
			  << " as it is bigger than LowLevel: " << low_level << endl;
		kdDebugFuncOut(trace);
		return false;
	} else {
		crit_level = _crit_level;
		kdDebugFuncOut(trace);
		return true;
	}
}

// ---> write private members SECTION : END <----
// ---> get private members SECTION : START <----

//! reports the chargelevel in percent when battery goes to state warning
int BatteryCollection::getWarnLevel() const {
	return warn_level;
}

//! reports the chargelevel in percent when battery goes to state low
int BatteryCollection::getLowLevel() const {
	return low_level;
}

//! reports the chargelevel in percent when battery goes to state critical
int BatteryCollection::getCritLevel() const {
	return crit_level;
}

// ---> get private members SECTION : END <----

#include "hardware_batteryCollection.moc"
