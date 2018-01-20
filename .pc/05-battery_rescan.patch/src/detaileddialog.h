/***************************************************************************
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#ifndef DETAILEDDIALOG_H
#define DETAILEDDIALOG_H

// own headers:
#include "detailed_Dialog.h"

// KDE headers:
#include <kprogress.h>

// other QT headers:
#include <qpixmap.h>

// own headers:
#include "hardware.h"
#include "hardware_cpu.h"
#include "settings.h"
#include "hardware_batteryCollection.h"

/*! 
*  \file 	detaileddialog.h
*  \brief 	Headerfile for detaileddialog.cpp and the class \ref detaileddialog.
*/
 /*! 
 *  \class 	detaileddialog
 *  \brief 	The class for the detailed information dialog
 *  \author 	Daniel Gollub, <dgollub@suse.de>
 *  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 *  \date    	2006
 */
class detaileddialog: public detailed_Dialog { 

	Q_OBJECT

public:
	//! default constructor
	detaileddialog(HardwareInfo *_hwinfo, QPixmap *_pixmap, Settings *_set, QWidget *parent = 0, const char *name = 0);
	//! default destructor
	~detaileddialog();

private slots:
	//! to close the dialog
	void closeDetailedDlg();
	//! to setup the battery progress widgets
	void setBattery();
	//! to setup the current power consumtion widgets
	void setPowerConsumption();
	//! to setup the CPU progress widgets
	void setProcessor();
	//! to setup the Throttling CPU progress widgets
	void setProcessorThrottling();
	//! to set the state AC KLed
	void setAC();
	//! to set all other information
	void setInfos();

private:
	//! pointer to class HardwareInfo to get cpu/battey/AC information
	HardwareInfo *hwinfo;
	//! pointer to class CPUInfo to get CPU information
	CPUInfo *cpuInfo;
	//! pointer to hardware information about the primary batteries.
	BatteryCollection *primaryBatteries;
	//! pointer to class settinfs to get the current settings
	Settings *config;

	//! pointer to the kpowersave class
	QPixmap *pixmap;

	//! the numbers of CPUs in the system
	int numOfCPUs;

	//! list of progressbars for battery information
	/*!
	 * This QValueList with type KProgress contains the list
	 * of battery progress widgets. Each element represent
	 * one battery or batteryslot
	 */
	QValueList<KProgress *> BatteryPBar;
	//! list of progressbars for CPU information
	/*!
	 * This QValueList with type KProgress contains the list
	 * of CPU progress widgets. Each element represent one CPU.
	 */
	QValueList<KProgress *> ProcessorPBar;

	//! QGridLayout for Battery progress widgets
	QGridLayout* BatteryGridLayout;
	//! QGridLayout for Processor progress widgets
	QGridLayout* ProcessorGridLayout;
};

#endif
