/**************************************************************************
*   Copyright (C) 2004-2006 by Danny Kukawka                              *
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
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
***************************************************************************/

#ifndef SCREEN_H
#define SCREEN_H

/* this is needed to avoid typedef clash with X11/Xmd.h (X11/Xproto.h)
 */
#ifndef QT_CLEAN_NAMESPACE
#define QT_CLEAN_NAMESPACE
#endif

// KDE Header
#include <dcopref.h>
#include <kprocess.h>
#include <klocale.h>

// QT Header
#include <qstring.h>
#include <qwidget.h>
//#include <qvector.h>
#include <qtimer.h>
#include <qevent.h>

// X11 Header
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

/*! 
*  \file 	screen.h
*  \brief 	Headerfile for screen.cpp and the class \ref screen.
*/
/*! 
*  \class 	screen
*  \brief 	class for all screensaver related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2004 - 2006
*/
class screen : public QWidget
{
	Q_OBJECT

private:

	//! reference to DCOP
	/*! 
	* This is the DCOP-reference to the KScreensaverIface. We use this to send
	* commandos to the active KScreensaver.
	*/
	DCOPRef screen_save_dcop_ref;
	//! KProcess to start xlock to lock the screen
	KProcess *xlock;
	//! KProcess to start xscreensaver with lock command
	KProcess *xscreensaver_lock;
	//! KProcess to start xscreensaver with restart command
	KProcess *xscreensaver_reset;
	//! KProcess to start gnome-screen-saver with lock command
	KProcess *gnomescreensaver_lock;
	//! KProcess to check if gnome-screensaver is running
	KProcess *gnomeScreensaverCheck;
	
	//! contains information about Xscreensaver
	/*!
	* This boolean represent information if Xscreensaver is active in X.
	* \li true:  if there is a Xscreensaver
	* \li false: if there isn't a Xscreensaver
	*/ 
	bool got_XScreensaver;
	//! cointains status information about the screensaver 
	/*!
	* The value of this integer represent statusinformation about the 
	* screensaver (Xscreensaver and KScreensaver):
	* \li (value == 0) if KScreensaver is disabled
	* \li (value == 1) if KScreensaver is activated
	* \li (value == 10) if Xscreensaver is not found or not running
	* \li (value == 11) if Xscreensaver is activated
	* \li (value == 20) if gnome-screen-saver is available
	* \li (value == -1) else
	*/
	int SCREENSAVER_STATUS;
	
	//! QTimer - interval
	/*! This is the interval to poll the Xscreensaver. The value is 58 sec */
	static const int xScreenSaver_timer_interval = 58000;
	//! QTimer to check/ping the Xscreensaver
	/*! This is the QTimer to ping the Xscreensaver. The ping-interval is defined 
	* through \ref xScreenSaver_timer_interval . */	
	QTimer *check_xscreensaver_timer;
	
	//! the active X-Display
	/*! Here we store a pointer to the active X-Display of KDE or GNOME */
	Display *mDisplay;
	//! Xscreensaver windowsid 
	/*! The windowid of the active Xscreensaver. */
	Window xs_windowid;
	//! XEvent for the Xscreensaver
	/*! This XEvent is used so send the commands to Xscreensaver. */
	XEvent ev;
	
private slots:

	//! to ping and deactivate the Xscreensaver
	void xscreensaver_ping();
	//! to get the return value of gnomescreensaver-command
	void getGSExited(KProcess *);

	void cleanProcess(KProcess *);

public slots:
	
	//! to fake a keyevent for the login dialog after suspend with lock screen
	void fakeShiftKeyEvent();

public:

	//! to check if \ref check_xscreensaver_timer is active
	/*!
	* This boolean contains information about QTimer \ref check_xscreensaver_timer .
	* \li true:  if the QTtimer is running
	* \li false: if the QTimer isn't active/ isn't in use
	*/
	bool check_xscreensaver_timer_runs;
	//! info about DPMS on the machine
	/*!
	* This variable represent the DPMS status of the machine.
	* \li true:  if the machine is DPMSable
	* \li false: if the machine isn't DPMSable or DPMS is basic deativated
	*/
	bool has_DPMS;
	
	//! blank only the screen
	void blankOnlyScreen( bool );
	//! reset KDE screensaver settings
	bool resetKDEScreensaver();
	//! reset XScreensaver settings
	bool resetXScreensaver();
	//! force DPMS off for display
	void forceDPMSOff();

	//! lock the screen
	bool lockScreen();
	//! lock the screen with a give alias for the lock method
	bool lockScreen( QString );
	//! to set DPMS on/off
	bool setDPMS( bool );
	//! to set the DPMS Timeouts
	bool setDPMSTimeouts( int, int, int );
	//! to activate/disable the Screensaver
	bool setScreenSaver( bool );
	
	//! for check the status of the screensaver
	int checkScreenSaverStatus();
	//! for check the actual status of DPMS on the machine
	int checkDPMSStatus();

	//! default constructor
	screen();
	//! default destructor
	virtual ~screen();
};
#endif
