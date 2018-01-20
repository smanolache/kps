 /**************************************************************************
 *   Copyright (C) 2006 by  Danny Kukawka                                  *
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

#ifndef _INACTIVITY_H_
#define _INACTIVITY_H_

/* this is needed to avoid typedef clash with X11
 */
#ifndef QT_CLEAN_NAMESPACE
#define QT_CLEAN_NAMESPACE
#endif

// KDE Header
#include <kprocess.h>

// QT Header
#include <qregexp.h>
#include <qstring.h>
#include <qwidget.h>
#include <qtimer.h>
#include <qevent.h>

// X11 Header
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

// from project
#include "kpowersave_debug.h"

/*! 
*  \file 	inactivity.h
*  \brief 	Headerfile for inactivity.cpp and the class \ref inactivity.
*/
/*! 
*  \class 	inactivity
*  \brief 	class for detect inactivity related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2006
*/

class inactivity : public QWidget
{
	Q_OBJECT
	
public:
	//! default constructor
	inactivity();
	//! default destructor
	~inactivity();
	
	//! to start the monitoring of the X-Server
	void start(int, QStringList);
	//! to stop the monitoring of the X-Server
	void stop();
	
	//! to get the current user inactivity
	unsigned long getXInactivity();

signals:

	//! signal emited if the with \ref start() set time of inactivity is expired
	void inactivityTimeExpired();
	//! signal to emit error msg
	void displayErrorMsg( QString );
	
private:

	//! pointer to the process to call pidof
	KProcess *proc;
	
	//! about the call result of pidof 
	/*!
	 * This boolean tells if call of the command pidof failed.
	 * \li true:  if failed
	 * \li false: if not
	 */
	bool pidof_call_failed;
	//! about the call of command pidof
	/*!
	 * This boolean tells if the call of the command pidof was started or if
	 * the command is running
	 * \li true:  if is started/running
	 * \li false: if not
	 */
	bool pidof_call_started;
	//! if pidof return value is recieved
	/*!
	 * This boolean tells if the call of the command pidof returned and the 
	 * returnvalue was parsed to set \ref blacklisted_running
	 * \li true:  if returned and parsed
	 * \li false: if not
	 */
	bool pidof_call_returned;
	//! if a blacklisted program/process is running
	/*! 
	 * This boolean tells if a blacklisted program/process is currently running.
	 * \li true:  if a blacklisted program/process is running
	 * \li false: if not
	 */
	bool blacklisted_running;

	//! QStringList with blacklisted programs for autosuspend
	QStringList blacklist;
	
	//! time which must expire befor emit signal for autosuspend
	unsigned long timeToInactivity;
	//! time of inactivity from the last check 
	unsigned long idleTime;
	//! time of inactivity from the last check 
	unsigned long blacklisted_running_last;
	//! if the XServer-has XScreenSaverExtension
	int has_XSC_Extension;
	
	//! QTimer intervall for the \ref checkInactivity Timer
	/*!
	 * The time intervall to check for the current status and time of
	 * userinactivity. The timeslice is currently 30 sec.
	 */
	static const int CHECK_for_INACTIVITY = 30000;
	
	//! QTimer for check inactivity
	/*!
	 * This timer is used to check the currently status and time of
	 * userinactivity on the X-Server. The timerinterval is defined trough
	 * \ref CHECK_for_INACTIVITY .
	 */
	QTimer *checkInactivity;
	
	// -------- FUNCTIONS ------------
	
	//! to check the user-inactivity on the XServer
	void checkXInactivity();
	//! to check for running blacklisted programs
	void checkBlacklisted();
	//! to monitor the values
	void check( bool recheck ); 
	//! to workaround a strange behavior of the XScreenSaver extension
	unsigned long workaroundCreepyXServer( unsigned long );

private slots:

	//! to monitor the values
	void check();
	//! to monitor the values
	void recheck();
	//! to get the PIDs of blacklisted programs/processes
	void getPIDs(KProcess *, char *, int);
	//! to get the signal if the command call is exited
	void getPIDsExited(KProcess *);
};

#endif
