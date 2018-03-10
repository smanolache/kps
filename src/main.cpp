/**************************************************************************
*   Copyright (C) 2004 by    Thomas Renninger                             *
*                              <trenn@suse.de>                            *
*                 2004-2007   Danny Kukawka                               *
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
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
***************************************************************************/

#include "kpowersave.h"
#include <kuniqueapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kiconloader.h>

#include "kpowersave_debug.h"

 /*! 
 *  \file 	main.cpp
 *  \brief 	The file with the \ref kdemain class to start kpowersave.
 */
/*!
 *  \class 	kdemain
 *  \brief 	The kpowersave kdemain class, which is the startpoint of KPowersave.
 *  \author     Thomas Renninger, <trenn@suse.de>
 *  \author     Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 *  \date       2004 - 2007
 */

static const char description[] = I18N_NOOP("KDE Frontend for Power Management, Battery Monitoring and Suspend");

KCmdLineOptions options[] = { { "force-acpi-check", I18N_NOOP("Force a new check for ACPI support"), 0 },
			      { "dbg-trace", I18N_NOOP("Trace function entry and leave points for debug\n"), 0 },	
			      { 0, 0, 0 }};

static const char version[] = "0.7.x (0.7.3)";
bool trace = false;
const int debug_area = 10137;

extern "C"
int main(int argc, char **argv)
{
	KAboutData about("kpowersave", I18N_NOOP("KPowersave"), version, description,
			 KAboutData::License_GPL, I18N_NOOP("(c) 2004-2006, Danny Kukawka\n"
							    "(c) 2004 Thomas Renninger"));
	
	about.addAuthor("Danny Kukawka", I18N_NOOP("Current maintainer"), "danny.kukawka@web.de" );
	about.addAuthor("Thomas Renninger", 0, "trenn@suse.de" );
	
	about.addCredit("Holger Macht", I18N_NOOP("Powersave developer and for D-Bus integration"), 
			"hmacht@suse.de");
	about.addCredit("Stefan Seyfried", I18N_NOOP("Powersave developer and tester"), 
			"seife@suse.de");
	about.addCredit("Daniel Gollub", I18N_NOOP("Added basic detailed dialog"), "dgollub@suse.de");
	about.addCredit("Michael Biebl", I18N_NOOP("Packaging Debian and Ubuntu"), "biebl@teco.edu");
	about.setBugAddress("powersave-users@forge.novell.com");
	about.setHomepage("http://sourceforge.net/projects/powersave");
	about.setTranslator("_: NAME OF TRANSLATORS\\nYour names","_: EMAIL OF TRANSLATORS\\nYour emails");

	KCmdLineArgs::init(argc, argv, &about);
	KCmdLineArgs::addCmdLineOptions (options);
	KUniqueApplication::addCmdLineOptions();

	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

	if (!KUniqueApplication::start()) {
		fprintf(stderr, "KPowersave is already running!\n");
		exit(0);
	}

	KUniqueApplication app;
	app.disableSessionManagement();

	kpowersave *mainWin = 0;

	mainWin = new kpowersave(args->isSet( "force-acpi-check" ), args->isSet( "dbg-trace" ));
	app.setMainWidget( mainWin );
	mainWin->show();

	// mainWin has WDestructiveClose flag by default, so it will delete itself.
	return app.exec();
}
