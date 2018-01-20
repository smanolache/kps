/***************************************************************************
 *   Copyright (C) 2007 by Danny Kukawka                                   *
 *                         <dkukawka@suse.de>, <danny.kukawka@web.de>      *
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

/*! \file 	dummy.cpp
 * \brief 	This file contains unused strings for translation
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \version 	0.0.1
 * \date    	2007
 */

#include <qstring.h>
#include <klocale.h>

class dummy {

	dummy() {
		QString dummy;
		
		// from configuredialog.cpp: 
		dummy = i18n("If the current desktop user is inactive, dim the display to:");
		dummy = i18n("Enable dim display on inactivity");
		dummy = i18n("Blacklist");
		dummy = i18n("Here you can add programs which should, if running, prevent the dimming "
			     "of the display.");
		dummy = i18n("Would you like to import a predefined blacklist?");
		dummy = i18n("Disable CPUs/Cores");
		dummy = i18n("Max. running CPUs:");
		dummy = i18n("Max. running CPUs/Cores:");
		dummy = i18n("Min. running CPUs:");
		dummy = i18n("Min. running CPUs/Cores:");
		dummy = i18n("Enable to switch off CPUs/cores");
		dummy = i18n("You have a multiprocessor/multicore machine.");
		dummy = i18n("You can disable CPUs/cores to reduce power consumption and save battery power.");
		dummy = i18n("Device");
		dummy = i18n("Devices");
		dummy = i18n("Device class");
		dummy = i18n("activate");
		dummy = i18n("Activate");
		dummy = i18n("deactivate");
		dummy = i18n("Deactivate");
		dummy = i18n("activated");
		dummy = i18n("deactivated");
		dummy = i18n("do nothing");
		dummy = i18n("Deactivate following devices:");
		dummy = i18n("Activate following devices");
		dummy = i18n("Reactivate following devices");
		dummy = i18n("Deactivate following device classes:");
		dummy = i18n("Activate following devices classes");
		dummy = i18n("Reactivate following device classes");
		dummy = i18n("If the scheme switched all devices are again activated.");
		dummy = i18n("This is a experimental feature.");
		dummy = i18n("If you have problems with this feature, please report them.");
		dummy = i18n("Select one of the available devices and click on ");
		dummy = i18n("Select one of the available device classes and click on ");
		dummy = i18n("Select one or more of the available devices and click on ");
		dummy = i18n("Select one or more of the available device classes and click on ");
		dummy = i18n("Please note: If you e.g. deactivate a network device you may lose your "
			     "internet connection.");
		dummy = i18n("<b>Note:</b> If you select this option, the computer will suspend or standby "
			     "if the current user is inactive for the defined time. <br><br> This feature "
			     "can also produce problems with some programs, such as video players or "
			     "cd burner. These programs can be blacklisted by checking <b>Enable "
			     "scheme-specific blacklist</b> and click <b>Edit Blacklist...</b>. If this "
			     "does not help, report the problem or deactivate autosuspend.<br><br> "
			     "Really use this option?");
		dummy = i18n("Try to use only one CPU/Core.");
		dummy = i18n("Reduce power consumption by try to use only one CPU/Core instead of spreading "
			     "the work over all/multiple CPUs.");
		
		// for settings.cpp:
		dummy = i18n("Could not load the global configuration.");
		dummy = i18n("Could not load the requested scheme configuration.");
		dummy = i18n("Configure the current scheme.");
		dummy = i18n("Try loading the default configuration.");
		dummy = i18n("Maybe the global configuration file is empty or missing.");

		// for kpowersave.cpp:
		dummy = i18n("Cannot connect to D-Bus. The D-Bus daemon may not be running.");
		dummy = i18n("Scheme switched to %1. \n Deactivate following devices: %2").arg("").arg("");
		dummy = i18n("Scheme switched to %1. \n Activate following devices: %2").arg("").arg("");
		dummy = i18n("Report ...");
		dummy = i18n("Suspend to RAM is not supported on your machine.");
		dummy = i18n("Additionally, please mail the output of %1 to %2 . Thanks!").arg("").arg("");
		dummy = i18n("Power consumption");

		// for screen.cpp
		dummy = i18n("KScreensaver not found.");
		dummy = i18n("Try locking with XScreensaver or xlock.");
		dummy = i18n("XScreensaver not found.");
		dummy = i18n("Try locking the screen with xlock.");
		dummy = i18n("XScreensaver and xlock not found. It is not possible to lock the screen. "
			     "Check your installation.");

		// for detaileddialog.cpp:
		dummy = i18n("D-Bus daemon:");
		dummy = i18n("ConsoleKit daemon:");
		dummy = i18n("Autosuspend activated:");
		dummy = i18n("Autodimm activated:");
		dummy = i18n("enabled");
		dummy = i18n("Session active:");

		// for countdowndialog.cpp
		dummy = i18n("The display get dimmed down to %1% in: ").arg(30);

		// other ConsoleKit related stuff
		dummy = i18n("Could not call %1. The current desktop session is not active.").arg("");
		dummy = i18n("Could not set %1. The current desktop session is not active.").arg("");
		dummy = i18n("Stopped %1. The current desktop session is now inactive.").arg("");
		dummy = i18n("Restarted %1. The current desktop session is now active again.").arg("");
	}
};
