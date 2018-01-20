 /**************************************************************************
 *   Copyright (C) 2006 by Daniel Gollub                                   *
 *                            <dgollub@suse.de>                            *
 *                         Danny Kukawka                                   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
/*! 
*  \file 	hardware_cpu.h
*  \brief 	Headerfile for hardware_cpu.cpp and the class \ref CPUInfo.
*/
/*! 
*  \class 	CPUInfo
*  \brief 	class for CPU information related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2006
*/

#ifndef _HARDWARE_CPU_H_
#define _HARDWARE_CPU_H_

// Global Header
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// QT Headers
#include <qstring.h>
#include <qobject.h>
#include <qvaluelist.h>

// own headers
#include "kpowersave_debug.h"

class CPUInfo : public QObject{

	Q_OBJECT

private:

	//! Integer with the numbers of CPUs in the system
	/*! This contains the number of CPUs in the current system */
	int numOfCPUs;

public:

	//! represent the current Throttling of the CPU
	/*!
	* This interge value represent the current throttling step of the CPU in percent
	* \li a value >= 0 in %
	*/
	QValueList <int> cpu_throttling;
		
	//! represent the current CPU speeds
	/*!
	* This integer value represent the current speed/frequency of the CPUs in Mhz
	* \li a value > 0 in Mhz
	*/
	QValueList <int> cpufreq_speed;

	//! represent the max. CPU speeds
	/*!
	* This integer value represent the max speed/frequency of the CPUs in Mhz
	* \li a value > 0 in Mhz
	*/
	QValueList <int> cpufreq_max_speed;

	//! if the machine support change CPU Freq in general
	/*! 
	* This boolean represent information if the machine support change the 
	* CPU freqency in general vi sysfs
	* \li true: 	if supported
	* \li false: 	else
	*/
	bool cpuFreqHW;
	//! tells if the  CPUFreq Speed changed
	/*! 
	* This boolean represent information about CPUFreq Speed changes.
	* \li true: if something changed
	* \li false: if nothing changed (or this is reset to false if the message was consumed)
	*/
	bool update_info_cpufreq_speed_changed;

	// --> functions
	//! default constructor
	CPUInfo();
	//! default destructor
	~CPUInfo();

	//! checks the current CPU Speed from sysfs
	int checkCPUSpeed();
	//! checks the Speed of throttling CPUs from /proc/cpuinfo
	int checkCPUSpeedThrottling();
	//! read the current throttling state of the CPUs from /proc/acpi/processor/CPUX/throttling
	bool getCPUThrottlingState();
	//! read the max speed of the CPUs
	void getCPUMaxSpeed();
	//! counts the total number of CPUs
	int getCPUNum();
};
#endif
