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
 * \file 	hardware_cpu.cpp
 * \brief 	In this file can be found the CPU information related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 *  \author 	Daniel Gollub, <dgollub@suse.de>
 * \author 	
 * \date    	2006
 */

// include own header
#include "hardware_cpu.h"
#include "hardware_cpu.moc"

// QT Header
#include <qdir.h>
#include <qtimer.h>

// system header
#include <fcntl.h>
#include <unistd.h>

/*! The default constructor of the class CPUInfo */
CPUInfo::CPUInfo() {
	kdDebugFuncIn(trace);

	update_info_cpufreq_speed_changed = true;
	numOfCPUs = -1;
	
	kdDebugFuncOut(trace);
}

/*! The default desctuctor of the class CPUInfo */
CPUInfo::~CPUInfo() {
	kdDebugFuncIn(trace);
}

/*!
 * This function counts all online/offline CPUS.
 * Returns the total count of CPUs - _not_ the last CPU ID!
 */
int CPUInfo::getCPUNum() {
	kdDebugFuncIn(trace);

	int cpu_id=0;
	QDir tmp_dir;
	QString cpu_path = "/sys/devices/system/cpu/cpu0/";
#ifdef FAKE_CPU
	cpu_path.prepend("/tmp/foo");
#endif

	// let check if we support cpufreq in general
	if (tmp_dir.exists(tmp_dir.absFilePath(cpu_path + "cpufreq/scaling_cur_freq", true))) {
		cpuFreqHW = true;
	} else {
		cpuFreqHW = false;
	}

	QString tmp_path = tmp_dir.absFilePath(cpu_path, true);

	while (tmp_dir.exists(tmp_path)) {
		int tmp = cpu_id;

		cpu_id++;
                cpu_path.replace(QString::number(tmp), QString::number(cpu_id));
		tmp_path = tmp_dir.absFilePath(cpu_path, true);
        }

	kdDebug() << "getCPUNum() return: '" << cpu_id << "'" << endl;
	kdDebugFuncOut(trace);
	return cpu_id;
}

/*!
 * The function checks the current CPU Speed. The current cpu speed needs to be read out from 
 * sysfs and currently not be obtained through the daemon. If the CPUFreg changed the new value
 * is set to \ref cpufreq_speed .
 * \return Information if something changed or if there are errors as an interger value
 * \retval 	-1 if there are error by reading from /sys/..
 * \retval	 0 if nothing changed
 * \retval 	 1 if something changed
 */
int CPUInfo::checkCPUSpeed(){
	kdDebugFuncOut(trace);

	bool speed_changed = false;
	int new_value = -1;
	int fd;
	char buf[15];
	QString cpu_device = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq";
#ifdef FAKE_CPU
	cpu_device.prepend("/tmp/foo");
#endif

	// first check path for the kernel on-demand-govenour then 
	// for the use userspace case
	update_info_cpufreq_speed_changed = false;
	cpufreq_speed.clear();

	if (numOfCPUs == -1)	
		numOfCPUs = getCPUNum();

	for (int cpu_id=0; cpu_id < numOfCPUs; cpu_id++) {

		new_value = -1;

		fd = open(cpu_device, O_RDONLY);
		if (read(fd, buf, 14) > 0){
			new_value = strtol(buf, NULL, 10)/1000;
			close(fd);
		}
		else{
			close(fd);
			speed_changed = true;
			// CPU disabeld -> set Freq to -1
			cpufreq_speed.append(-1);
		}

		if (new_value != cpufreq_speed[cpu_id]) {
			speed_changed = true;
			cpufreq_speed.append(new_value);
		}

		cpu_device.replace(QString::number(cpu_id), QString::number(cpu_id+1));
	}
	
	if (speed_changed) {
		update_info_cpufreq_speed_changed = true;
		kdDebugFuncOut(trace);
		return 1;
	}

	kdDebugFuncOut(trace);
	return 0;
}

/*!
 * The function checks the Speed of throttling CPU(s). The cpu speed needs to be read out from 
 * /proc/cpuinfo.
 * \return Success or error while reading /proc/cpuinfo 
 * \retval	  0 successful
 * \retval 	 -1 reading problem
 */
int CPUInfo::checkCPUSpeedThrottling() {
	kdDebugFuncOut(trace);

	QString cpu_file = "/proc/cpuinfo";
#ifdef FAKE_CPU
	cpu_file.prepend("/tmp/foo");
#endif	
	QFile cpu_info(cpu_file);

	// clear cpufreq list
	cpufreq_speed.clear();

	if ( !cpu_info.open(IO_ReadOnly) ) {
		cpu_info.close();
		kdDebugFuncOut(trace);
		return -1;
	}

	QTextStream stream( &cpu_info );
	QString line;

	while ( !stream.atEnd() ) {
		line = stream.readLine();
		
		if (line.startsWith("cpu MHz		: ")) {
			line.remove("cpu MHz		: ");
			line = line.remove(line.length() - 4, 4);
			cpufreq_speed.append(line.toInt());
		}
	}
	
	while ((int) cpufreq_speed.count() < numOfCPUs) {
		cpufreq_speed.append(-1);
	}

	cpu_info.close();
	kdDebugFuncOut(trace);
	return 0;
}

/*!
 * The function gets the current throttling state of the CPU(s). The throttling state needs to be 
 * read out from /proc/acpi/processor/CPUX/throttling.
 * \return boolean with info if throttling is supported
 * \retval true if throttling is supported
 * \retval false if not supported or on any other error
 */
bool CPUInfo::getCPUThrottlingState() {
	kdDebugFuncIn(trace);

	int id = 0;
	QFileInfo *fi;
	QString cpu_dirname;
	QString dir_acpi_processor = "/proc/acpi/processor/";
#ifdef FAKE_CPU
	dir_acpi_processor.prepend("/tmp/foo");
#endif

	QDir d_throttling(dir_acpi_processor);
	if (!d_throttling.exists()) {
		kdDebugFuncOut(trace);
		return false;
	}

	d_throttling.setFilter( QDir::Dirs );
	d_throttling.setNameFilter("CPU*");

	const QFileInfoList *list = d_throttling.entryInfoList();
	QFileInfoListIterator it( *list );

	// clear throttling value list
	cpu_throttling.clear();

	while ((fi = it.current()) != 0 ) {
		cpu_dirname = fi->fileName();

		QString throttling_device = d_throttling.absPath();
		throttling_device.append("/").append(cpu_dirname).append("/throttling");

		kdDebug() << "Throttling state file for CPU" << id << " will be: " << throttling_device << endl;

		QFile throttling(throttling_device);

		// open throttling state file
		if ( throttling.open(IO_ReadOnly) ) {
			QTextStream stream( &throttling );
			QString line;

			do {
				line = stream.readLine();
			} while (!line.startsWith("   *T") && !stream.atEnd() );

			if (line.startsWith("   *T")) {
				line = line.right(3);
				line.remove("%");
				cpu_throttling.append(line.toInt());
				kdDebug () << "CPU " << id 
					   << ": cpu_throttling is set to: " << cpu_throttling[id] << endl;
			} else {
				cpu_throttling.append(0);
			}
		}

		throttling.close();
		++it; // next entry
		id++; // count cpu id
	}

	kdDebugFuncOut(trace);
	return true;
}

/*!
 * The function gets the Max CPU Speed. The max cpu speed needs to be read out from 
 * sysfs and currently not be obtained through the daemon. 
 */
void CPUInfo::getCPUMaxSpeed() {
	kdDebugFuncIn(trace);

	int fd;
	int maxfreq;
	char buf[15];
	QString cpu_device_max = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";
#ifdef FAKE_CPU
	cpu_device_max.prepend("/tmp/foo");	
#endif

	cpufreq_max_speed.clear();

	if (numOfCPUs == -1)	
		numOfCPUs = getCPUNum();

//	while (!access(cpu_device_max, R_OK)) {
	for (int cpu_id=0; cpu_id < numOfCPUs; cpu_id++) {

		fd = open(cpu_device_max, O_RDONLY);
		if (read(fd, buf, 14) > 0){
			maxfreq = strtol(buf, NULL, 10)/1000;
			cpufreq_max_speed.append(maxfreq);
			close(fd);
		} else {
			cpufreq_max_speed.append(-1);
			close(fd);
		}

		cpu_device_max.replace(QString::number(cpu_id), QString::number(cpu_id+1));
	}

	kdDebugFuncOut(trace);
}
