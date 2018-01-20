/***************************************************************************
 *   Copyright (C) 2006 by   Daniel Gollub                                 *
 *                            <dgollub@suse.de>                            *
 *                 2006-2007 Danny Kukawka                                 *
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

/*! 
 * \file	detaileddialog.cpp
 * \brief	In this file can be found the detailed dialog related code.
 * \author	Daniel Gollub <dgollub@suse.de>
 * \author	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date	2006-2007
 */

// KDE headers:
#include <kled.h>
#include <kiconloader.h>
#include <klocale.h>

// QT headers:
#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qgroupbox.h>

// own headers
#include "detaileddialog.h"

/*! 
 * This is the default constructor of the class detaileddialog. 
 */
detaileddialog::detaileddialog( HardwareInfo *_hwinfo, QPixmap *_pixmap, Settings *_set, 
				QWidget* parent, const char* name )
    			      : detailed_Dialog( parent, name, false, WDestructiveClose ) {
	kdDebugFuncIn(trace);
	hwinfo = _hwinfo;
	config = _set;
	pixmap = _pixmap;
	primaryBatteries = hwinfo->getPrimaryBatteries();
	cpuInfo = new CPUInfo();

	int batteries = primaryBatteries->getNumBatteries();
	numOfCPUs = cpuInfo->getCPUNum();

	this->setCaption(i18n("KPowersave Information Dialog"));
	
	// use this as compromise with current translation process
	// TODO: remove them in the next translation round
	GeneralGroup->setTitle(i18n("Miscellaneous"));
	ProcessorGroup->setTitle(i18n("CPUs"));

	ProcessorGridLayout = new QGridLayout(ProcessorFrame, numOfCPUs, 2, 0, 5, "ProcessorGridLayout");
	
	if (batteries > 0) {
		if (batteries > 1) batteries++;

		BatteryGroup->setTitle(i18n("Battery state:").remove(":"));
		BatteryGridLayout = new QGridLayout(BatteryFrame, batteries, 2, 0, 5, "BatteryGridLayout");

		for (int i = 0;  i < batteries; i++) {
			QLabel *Label = new QLabel(BatteryFrame, "BatteryLabel");
			if ((primaryBatteries->getNumBatteries() > 1) && (i == 0))
				Label->setText( i18n( "Total:" ));
			else if ((primaryBatteries->getNumBatteries() > 1) && (i > 0))
				Label->setText( i18n( "Battery %1" ).arg(i));
			else 
				Label->setText( i18n( "Battery %1" ).arg(i + 1));
	
			BatteryGridLayout->addWidget( Label, i , 0);
	
			KProgress *PBar = new KProgress(BatteryFrame, "BatteryPBar");
			PBar->setTextEnabled(true);
	
			BatteryPBar.append( PBar );
			BatteryGridLayout->addWidget( PBar, i , 1);
		}
		BatteryFrame->adjustSize();
		tl_powerConsDesc->hide();
		tl_powerConsValue->hide();
		connect(hwinfo, SIGNAL(generalDataChanged()), this, SLOT(setBattery()));
		connect(primaryBatteries, SIGNAL(batteryChanged()), this, SLOT(setBattery()));
		connect(primaryBatteries, SIGNAL(batteryChargingStateChanged(int)), this,
			SLOT(setPowerConsumption()));
		connect(primaryBatteries, SIGNAL(batteryRateChanged()), this, 
			SLOT(setPowerConsumption()));
		setBattery();
		setPowerConsumption();
	} else {
		BatteryGroup->hide();
	}

	cpuInfo->checkCPUSpeed();

	ProcessorPictogram->setPixmap(SmallIcon("processor", 22));

	for (int i = 0; i < numOfCPUs; i++) {
		QLabel *Label = new QLabel(ProcessorFrame, "ProcessorLabel");
		Label->setText( i18n( "Processor %1" ).arg(i + 1));
		ProcessorGridLayout->addWidget( Label, i , 0);

		KProgress *CPUPBar = new KProgress(ProcessorFrame, "ProcessorPBar");
		CPUPBar->setTextEnabled(true);

		ProcessorPBar.append( CPUPBar );
		ProcessorGridLayout->addWidget( CPUPBar, i , 1);
	}
	ProcessorFrame->adjustSize();
	
	connect(OkButton, SIGNAL(clicked()), this, SLOT(closeDetailedDlg()));
	connect(hwinfo, SIGNAL(ACStatus(bool)), this, SLOT(setAC()));
	// TODO: replace event
	//connect(pd, SIGNAL(schemeDataChanged()), this, SLOT(setInfos()));
	connect(hwinfo, SIGNAL(generalDataChanged()), this, SLOT(setInfos()));

	if (hwinfo->supportCPUFreq() || cpuInfo->cpuFreqHW) {
		// Check if cpufreq is available
		cpuInfo->getCPUMaxSpeed();
		setProcessor();
		connect(hwinfo, SIGNAL(currentCPUFreqPolicyChanged()), this, SLOT(setInfos()));
	} else {
		// .. if not, use cpu throttling
		if (!cpuInfo->getCPUThrottlingState() || numOfCPUs <= 1) {
			connect(hwinfo, SIGNAL(generalDataChanged()), this, SLOT(setProcessorThrottling()));
		}
		setProcessorThrottling();
	}

	setAC();
	setInfos();

	kdDebugFuncOut(trace);
}

/*! This is the default destructor of class detaileddialog. */
detaileddialog::~detaileddialog() {	
	kdDebugFuncIn(trace);
	// no need to delete child widgets, Qt does it all for us
}

/*!
 * \b SLOT called if the dialog is closed by the user.
 * We do some cleanups here.
 */
void detaileddialog::closeDetailedDlg() {
	kdDebugFuncIn(trace);

	this->close();
	delete(this);
}

/*!
 * \b SLOT to set up the battery progress widgets.
 */
void detaileddialog::setBattery() {
	kdDebugFuncIn(trace);

	QString minutes;
	int batteries = 0;

	
	// refresh battery collection
	primaryBatteries = hwinfo->getPrimaryBatteries();
	QPtrList<Battery> allBatteries = hwinfo->getAllBatteries();

	batteries = primaryBatteries->getNumBatteries();

	if (batteries > 1) batteries++;

	for (int i=0; i < batteries ; i++) {
		int _r_min = 0;
		int _r_per = 0;
		int _c_state = UNKNOWN_STATE;
		bool _present = false;

		BatteryPBar[i]->setTextEnabled(true);
		BatteryPBar[i]->reset();

		if ( (primaryBatteries->getNumBatteries() > 1) && (i == 0) ) {
			// first progressbar with overall infos
			
			_r_min = primaryBatteries->getRemainingMinutes();
			_r_per = primaryBatteries->getRemainingPercent();
			_c_state = primaryBatteries->getChargingState();
			if (primaryBatteries->getNumPresentBatteries() > 0)
				_present = true;
		}
		else {
			// find the related primary battery
			int _current = 0;
			Battery *bat;
			for (bat = allBatteries.first(); bat; bat = allBatteries.next() ) {
				if (bat->getType() ==  primaryBatteries->getBatteryType()) {
					_current++;
					
					if (!bat->isPresent()) {
						_present = false;
					}
					else {
						_r_min = bat->getRemainingMinutes();
						_r_per = bat->getPercentage();
						_c_state = bat->getChargingState();
						_present = true;
					}

					if (_current == i) {
 						break;
					}
					
				}
			}
		}

		if (!_present) {
			BatteryPBar[i]->setFormat(i18n("not present"));
			BatteryPBar[i]->setProgress(0);
			BatteryPBar[i]->setEnabled(false);
		} else {
			int hours = _r_min / 60;
			minutes.setNum(_r_min % 60);
			minutes = minutes.rightJustify(2, '0');	
	
			// CHARG_STATE_CHARG_DISCHARG --> display only the percentage
			if (_c_state == UNKNOWN_STATE || _r_min < 0 ) {
				BatteryPBar[i]->setFormat("%p%");
			} else if (_c_state == CHARGING && hwinfo->hasAPM() ) {
				// this should fix apm, where we have no time info if charging
				BatteryPBar[i]->setFormat("%p% " + i18n("charged"));
			} else if (_c_state == CHARGING) {
				QString temp = i18n("%1:%2 h until charged").arg(hours).arg(minutes);
				BatteryPBar[i]->setFormat(temp);
			} else if (_c_state == DISCHARGING) {
				QString temp = i18n("%1:%2 h remaining").arg(hours).arg(minutes);
				BatteryPBar[i]->setFormat(temp);
			
			} else {
				//fallback 
				BatteryPBar[i]->setFormat(i18n("unknown"));
			}
	
			if (_r_per < 0)
				BatteryPBar[i]->setProgress(0);
			else	
				BatteryPBar[i]->setProgress(_r_per);
			BatteryPBar[i]->setEnabled(true);
		}
	}

	BatteryPictogram->setPixmap(*pixmap);
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to set up the Processor/CPU progress bar widgets.
 */
void detaileddialog::setPowerConsumption() {
	kdDebugFuncIn(trace);

	// refresh battery collection
	primaryBatteries = hwinfo->getPrimaryBatteries();
	int rate = primaryBatteries->getCurrentRate();

	if (rate > 0 && !primaryBatteries->getChargeLevelUnit().isEmpty()) {	

		QString _val;
		_val.setNum(rate);
		_val += " " + primaryBatteries->getChargeLevelUnit().remove('h');

		tl_powerConsValue->setText(_val);

		if (!tl_powerConsDesc->isShown()) {
			tl_powerConsDesc->show();
			tl_powerConsValue->show();
		}
	} else {
		if (tl_powerConsDesc->isShown()) {
			tl_powerConsDesc->hide();
			tl_powerConsValue->hide();
		}
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to set up the Processor/CPU progress bar widgets.
 */
void detaileddialog::setProcessor() {
	kdDebugFuncIn(trace);

	cpuInfo->checkCPUSpeed();

	for (int i=0; i < numOfCPUs; i++) {
		kdDebug() << "ID: " << i << "(" << cpuInfo->cpufreq_speed.count()
			  << ") cur_freq: " << cpuInfo->cpufreq_speed[i] << " max_freq: "
			  << cpuInfo->cpufreq_max_speed[i] << endl; 

		//ProcessorPBar[i]->setTextEnabled(true);
		if (cpuInfo->cpufreq_speed[i] > 0) {
			// CPU/Core is back from offline
			if(ProcessorPBar[i]->progress() == 0)
				cpuInfo->getCPUMaxSpeed();

			if(ProcessorPBar[i]->progress() != cpuInfo->cpufreq_speed[i]) {
				// get max cpu freq and set it to the max of the progressbar
				int maxfreq = cpuInfo->cpufreq_max_speed[i];
				ProcessorPBar[i]->setTotalSteps(maxfreq);
	
				// display  1400 MHz instead of 1400%
				ProcessorPBar[i]->setFormat(i18n("%v MHz"));
				ProcessorPBar[i]->setProgress(cpuInfo->cpufreq_speed[i]);
				ProcessorPBar[i]->setEnabled(true);
			}
		} else {
			ProcessorPBar[i]->setFormat(i18n("deactivated"));
			ProcessorPBar[i]->setProgress(0);
			ProcessorPBar[i]->setEnabled(false);
		}
	}
	QTimer::singleShot(333, this, SLOT(setProcessor()));	
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to set up the Processor/CPU bar widgets for cpu throttling machines.
 */
void detaileddialog::setProcessorThrottling() {
	kdDebugFuncOut(trace);

	bool throttling = cpuInfo->getCPUThrottlingState();
	cpuInfo->checkCPUSpeedThrottling();

	for (int i=0; i < numOfCPUs; i++) { 
		if (throttling)
			kdDebug() << "Throttling CPU : " << i << " - freq: " << cpuInfo->cpufreq_speed[i]
				  << " - throttling state: " << cpuInfo->cpu_throttling[i] << "%" << endl;
		else
			kdDebug() << "CPU - freq: " << cpuInfo->cpufreq_speed[i] << endl;
	
		if (throttling && cpuInfo->cpufreq_speed[i] > 0 && cpuInfo->cpu_throttling[i] >= 0) {
			// get max cpu freq and set it to the max of the progressbar
			ProcessorPBar[i]->setTotalSteps(100);
			QString ProgressString = QString("%1% (%2 MHz)").arg(100 - cpuInfo->cpu_throttling[i]).arg(cpuInfo->cpufreq_speed[i]);
			ProcessorPBar[i]->setFormat(i18n(ProgressString));
			ProcessorPBar[i]->setProgress(100 - cpuInfo->cpu_throttling[i]);
			ProcessorPBar[i]->setEnabled(true);
		} else if (cpuInfo->cpufreq_speed[i] < 0) {
			ProcessorPBar[i]->setFormat(i18n("deactivated"));
			ProcessorPBar[i]->setProgress(0);
			ProcessorPBar[i]->setEnabled(false);
		} else {
			ProcessorPBar[i]->setTotalSteps(cpuInfo->cpufreq_speed[i]);
			ProcessorPBar[i]->setFormat(i18n("%v MHz"));
			ProcessorPBar[i]->setProgress(cpuInfo->cpufreq_speed[i]);
			ProcessorPBar[i]->setEnabled(true);
		}
	}

	if (throttling || numOfCPUs > 1) {
		// currently there are no events we can use to get actual data
		// so we recheck data ever 2 secs to register changes in the 
		// throttling state and if a CPU/core online state change
		QTimer::singleShot(2000, this, SLOT(setProcessorThrottling()));
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to set up the AC status within the Led widget.
 */
void detaileddialog::setAC() {
	kdDebugFuncIn(trace);

	if (hwinfo->getAcAdapter()) {
		LabelACStatus->setText( i18n("plugged in") );
		LedAC->on();
	} else {
		LedAC->off();
		LabelACStatus->setText( i18n("unplugged") );
	}

	setInfos();
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to set all additional informtation as e.g. CPUFrequency policy
 * or current scheme
 */
void detaileddialog::setInfos() {
	kdDebugFuncOut(trace);

	QString display;
	QString displayValue;

	if(!config->currentScheme.isEmpty())
		display += i18n("Current Scheme: ") + "\n";
		displayValue += i18n(config->currentScheme) + "\n";
		if(config->currentScheme == config->ac_scheme)
			InfoPictogram->setPixmap(SmallIcon("scheme_power", 22));
		else if(config->currentScheme == config->battery_scheme)
			InfoPictogram->setPixmap(SmallIcon("scheme_powersave", 22));
		else if(config->currentScheme == "Acoustic")
			InfoPictogram->setPixmap(SmallIcon("scheme_acoustic", 22));
		else if(config->currentScheme == "Presentation")
			InfoPictogram->setPixmap(SmallIcon("scheme_presentation", 22));
		else if(config->currentScheme == "AdvancedPowersave")
			InfoPictogram->setPixmap(SmallIcon("scheme_advanced_powersave", 22));
		else
			InfoPictogram->setPixmap(SmallIcon("kpowersave", 22));

	if(hwinfo->isOnline()) {
		if (hwinfo->supportCPUFreq()) {
			display += i18n("Current CPU Frequency Policy:") + "\n";
			switch (hwinfo->getCurrentCPUFreqPolicy()){
				case PERFORMANCE:
					displayValue += i18n("Performance")  + "\n";
					break;
				case DYNAMIC:
					displayValue += i18n("Dynamic") + "\n";
					break;
				case POWERSAVE:
					displayValue += i18n("Powersave") + "\n";
					break;
				default:
					displayValue += i18n("unknown") + "\n";
					break;
			}
		}

		// refresh battery collection
		primaryBatteries = hwinfo->getPrimaryBatteries();
		int batteries = primaryBatteries->getNumBatteries();
		QPtrList<Battery> allBatteries = hwinfo->getAllBatteries();

		if (batteries > 0 && primaryBatteries->getNumPresentBatteries() > 0) {

			display += i18n("Battery state:") + "\n";
			switch (primaryBatteries->getBatteryState()){
				case BAT_CRIT:
					displayValue += i18n("Critical") + "\n";
					break;
				case BAT_LOW:
					displayValue += i18n("Low") + "\n";
					break;
				case BAT_WARN:
					displayValue += i18n("Warning") + "\n";
					break;
				case BAT_NORM:
					displayValue += i18n("ok") + "\n";
					break;
				default:
					displayValue += i18n("unknown") + "\n";
					break;
			}
		}
		
		if(hwinfo->supportBrightness()) {
			display += i18n("Set brightness supported:") + "\n";
			displayValue += i18n("yes") + "\n";
		} else {
			display += i18n("Set brightness supported:") + "\n";
			displayValue += i18n("no") + "\n";
		}

		display += i18n("HAL Daemon:");
		displayValue += i18n("running");
	}
	else {
		display += i18n("HAL Daemon:");
		displayValue += i18n("not running");
	}

	if(!display.isEmpty()) 
		InfoLabel->setText(display);
	
	InfoLabelValue->setText(displayValue);
	kdDebugFuncOut(trace);
}

#include "detaileddialog.moc"
