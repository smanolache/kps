/***************************************************************************
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

/*! \file 	inactivity.cpp
 * \brief 	In this file can be found the inactivity related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2006-2007
 */

// own header
#include "inactivity.h"

/* needed for lXext C library linkage */
extern "C" {
	#include <X11/Xproto.h>
	#include <X11/extensions/dpms.h>
	#include <X11/extensions/scrnsaver.h>
}
 
// KDE Headers
#include <klocale.h>

/*! The default constructor of the class autosuspend */
inactivity::inactivity() {
	kdDebugFuncIn(trace);

	proc = NULL;

	timeToInactivity = 0;
	blacklisted_running_last = 0;
	
	pidof_call_failed = false;
	pidof_call_started = false;
	pidof_call_returned = false;
	blacklisted_running = false;
	
	int dummy = 0;
	has_XSC_Extension = XScreenSaverQueryExtension( qt_xdisplay(), &dummy, &dummy );
	
	checkInactivity =  new QTimer( this );
	connect( checkInactivity, SIGNAL(timeout()), this, SLOT(check()));

	kdDebugFuncOut(trace);
}

/*! The default destructor of the class autosuspend */
inactivity::~inactivity() {
	kdDebugFuncIn(trace);

	delete proc;
	proc = NULL;

	kdDebugFuncOut(trace);
}

/*!
 * This function start the monitoring of inactivity of user on the X-Server.
 * Here wee set the time for the signal \ref inactivityTimeExpired() and start
 * the needed QTimer.
 * \param timeToExpire  Integer value representing the time of inactivity which need 
 *			to elapse befor send signal. The time is in seconds.
 * \param blacked	QStringList with blacklisted programs which if detected with 
 *			pidof() as running prevent the autosuspend.
 */
void inactivity::start( int timeToExpire, QStringList blacked ) {
	kdDebugFuncIn(trace);

	blacklist = blacked;
 
	if(timeToExpire > 0 && has_XSC_Extension){
		stop();
		timeToInactivity = (unsigned long) (timeToExpire * 1000);
		checkInactivity->start(CHECK_for_INACTIVITY, true);
	}

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to call check as recheck inactivity if befor a running PID 
 * request was detected.
 */
void inactivity::recheck() {
	kdDebugFuncIn(trace);

	check(true);
	
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to call check without a recheck.
 */
void inactivity::check() {
	check(false);
}

/*!
 * \b SLOT to check the current idle-time of the X-Server and if there
 * are blacklisted programs are running. If the through \ref timeToInactivity 
 * defined time is expired, this function emit signal \ref inactivityTimeExpired() .
 * \param recheck   boolean which define if this is a recheck or not. 
 *                  \li true, if this is a recheck. In this case we didn't call
 *                      \ref checkBlacklisted() again
 *                  \li false, if this is normal check
 */
void inactivity::check( bool recheck ) {
	kdDebugFuncIn(trace);

	if (timeToInactivity > 0) {	
		checkXInactivity();
		if (!pidof_call_started && !recheck) checkBlacklisted();
		
		if( idleTime < blacklisted_running_last ) {
			blacklisted_running_last = idleTime;
		}
	
		if((idleTime - blacklisted_running_last ) >= timeToInactivity) {
			if (!pidof_call_started) {
				if (( pidof_call_returned && !blacklisted_running ) ||
				( pidof_call_returned && pidof_call_failed )) {
					emit inactivityTimeExpired();
				}
				else {
					checkInactivity->start(CHECK_for_INACTIVITY, true);
				}
			}
			else {
				//called if there is a getPIDs() is running
				QTimer::singleShot(500, this, SLOT(recheck()));
			}
		}
		else checkInactivity->start(CHECK_for_INACTIVITY, true);
	} else {
		kdWarning() <<  "timeToInactivity <= 0, stoped autosuspend checks!" << endl;
	}

	kdDebugFuncOut(trace);
}

/*!
 * This function stop the monitoring and reset all variables and Timer.
 */
void inactivity::stop() {
	kdDebugFuncIn(trace);

	if (checkInactivity->isActive()) checkInactivity->stop();
	timeToInactivity = 0;
	idleTime = 0;
	blacklisted_running_last = 0;
	
	pidof_call_failed = false;
	pidof_call_started = false;
	pidof_call_returned = false;
	blacklisted_running = false;

	kdDebugFuncOut(trace);
}

/*!
 * This function query the idle-time of user-imput from the X-Server and set
 * the return value to \ref idleTime.
 */
void inactivity::checkXInactivity(){
	kdDebugFuncIn(trace);

	idleTime = getXInactivity();
	kdDebug() << "autosuspend::checkXInactivity - idleTime: " << idleTime << endl;

	kdDebugFuncOut(trace);
}

/*!
 * This function query the idle-time of user-imput from the X-Server and
 * return the current idle-time. 
 */
unsigned long inactivity::getXInactivity(){
	kdDebugFuncIn(trace);

	if(has_XSC_Extension) {
		static XScreenSaverInfo* mitInfo = 0;
		if (!mitInfo) mitInfo = XScreenSaverAllocInfo ();
		XScreenSaverQueryInfo (qt_xdisplay(), DefaultRootWindow (qt_xdisplay()), mitInfo);
		kdDebugFuncOut(trace);
		return workaroundCreepyXServer(mitInfo->idle);
	}
	else {
		kdDebugFuncOut(trace);
		return 0;
	}
	
}

/*!
 * This function workaround a fucking XServer idleTime bug in the 
 * XScreenSaverExtension, if dpms is running. In this case always the
 * current dpms-state time is extracted from the current idletime.
 * This mean: XScreenSaverInfo->idle is not the time since the last
 * user activity, as descriped in the header file of the extension.
 * This result in SUSE bug # and sf.net bug #
 *
 * Workaround: check if if XServer is in a dpms state, check the 
 *             current timeout for this state and add this value to 
 * 	       the current idle time and return.
 *
 * \param _idleTime a unsigned long value with the current ideletime fromm
 *                  XScreenSaverInfo->idle
 * \return a unsigned long with the corrected idletime
 */
unsigned long inactivity::workaroundCreepyXServer( unsigned long _idleTime ){
	kdDebugFuncOut(trace);

	int dummy;
	CARD16 standby, suspend, off;
	CARD16 state;
	BOOL onoff;

	Display *dpy = qt_xdisplay();

	kdDebug() << "Current idleTime: " << _idleTime << endl;

	if (DPMSQueryExtension(dpy, &dummy, &dummy)) {
		if (DPMSCapable(dpy)) {
			DPMSGetTimeouts(dpy, &standby, &suspend, &off);
			DPMSInfo(dpy, &state, &onoff);

			if (onoff) {
				switch (state) {
					case DPMSModeStandby:
						kdDebug() << "DPMS enabled. Monitor in Standby. Standby: "
							  << standby << " sec" << endl;
						// this check is a littlebit paranoid, but be sure
						if (_idleTime < (unsigned) (standby * 1000))
							_idleTime += (standby * 1000);
						break;
					case DPMSModeSuspend:
						kdDebug() << "DPMS enabled. Monitor in Suspend. Suspend: "
							  << suspend << " sec" << endl;
						if (_idleTime < (unsigned) ((suspend + standby) * 1000))
							_idleTime += ((suspend + standby) * 1000);
						break;
					case DPMSModeOff:
						kdDebug() << "DPMS enabled. Monitor is Off. Off: "
							  << off << " sec" << endl;
						if (_idleTime < (unsigned) ((off + suspend + standby) * 1000))
							_idleTime += ((off + suspend + standby) * 1000);
						break;
					case DPMSModeOn:
					default:
						break;
				}
			}
		} 
	}

	kdDebug() << "Corrected idleTime: " << _idleTime << endl;
	kdDebugFuncOut(trace);
	return _idleTime;
}

/*!
 * This funtion starts the monitoring of blacklisted processes.
 */
void inactivity::checkBlacklisted(){
	kdDebugFuncIn(trace);

	if (proc != NULL) {
		delete proc;
		proc = NULL;
	}

	proc = new KProcess;
	*proc << "pidof" << blacklist;

	connect( proc, SIGNAL(receivedStdout(KProcess *, char *, int)),this,
		 SLOT(getPIDs(KProcess *, char *, int)));
	connect( proc, SIGNAL(processExited(KProcess *)),
		 SLOT(getPIDsExited(KProcess *)));
	
	if (!proc->start(KProcess::NotifyOnExit, KProcess::AllOutput))
	{
		emit displayErrorMsg(i18n("Could not start 'pidof'. "
					  "Could not autosuspend the machine.\n"
					  "Please check your installation."));
	}

	pidof_call_started = true;
	pidof_call_returned = false;
	pidof_call_failed = false;

	kdDebugFuncOut(trace);
}


/*!
 * \b SLOT to get the return of the command pidof and parse this to set
 * \ref blacklisted_running .
 * \param *proc     pointer to the sending KProcess
 * \param *buffer   the char pointer to the output of the process to stdout
 * \param *lenght   the length of the buffer
 */
void inactivity::getPIDs(KProcess */*proc*/, char *buffer, int /*lenght*/) {
	kdDebugFuncIn(trace);

	QString pids(buffer);
	pids.remove(" ");
	if(pids.isEmpty() || pids == "\n" ) {
		kdDebug() << "NO! BLACKLISTED IS RUNNING" << endl;
		blacklisted_running = false;
	} 
	else {
		if (pids.contains(QRegExp::QRegExp("[0-9]"))) {
			kdDebug() << "BLACKLISTED IS RUNNING" << endl;
			blacklisted_running = true;
			blacklisted_running_last = idleTime;
		}
		else {
			kdError() << "GET BLACKLISTED FAILED - WRONG RETURN" << endl;
			blacklisted_running = false;
			pidof_call_failed = true;
		}
	}

	kdDebugFuncOut(trace);
}


/*!
 * \b SLOT which called if the call of pidof is exited
 * \param proc the KPocess which called this SLOT
 */
void inactivity::getPIDsExited(KProcess *proc){
	kdDebugFuncIn(trace);

	pidof_call_returned = true;
	pidof_call_started = false;
	
	
	if (proc->normalExit()){
		// if returned some pids or if pid returned nothing
		if (proc->exitStatus() == 1 || proc->exitStatus() == 0){
			pidof_call_failed = false;
			kdDebugFuncOut(trace);
			return;
		}
	}	
	// if something crashed/failed
	pidof_call_failed = true;
	kdDebugFuncOut(trace);
}

#include "inactivity.moc"
