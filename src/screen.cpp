/***************************************************************************
 *   Copyright (C) 2004-2006 by Danny Kukawka                              *
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
/*! \file screen.cpp
 * All here displayed file members of screen.cpp are related to operations with the 
 * XSreensaver/XServer. This functions are basic/low level operations. They are 
 * inspired, partly copied and modified from <a href="http://www.mplayerhq.hu/">MPlayer</a>
 * code (1.05pre). Thanks for the inspiration. \n \n
 * All 'higher level' class members of the class screen can be found here: \ref screen 
 * \brief 	In this file can be found all screensaver related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2004 - 2006
 */
 
 // own headers
#include "screen.h"
#include "kpowersave_debug.h"

/* needed for lXext C library linkage */
extern "C" {
	#include <X11/Xproto.h>
	#include <X11/extensions/dpms.h>
	#include <X11/extensions/XTest.h>
}

// KDE Headers
#include <kprocess.h>

/*! The default constructor of the class screen */
screen::screen() {
	kdDebugFuncIn(trace);

	xscreensaver_lock = NULL;
	xscreensaver_reset = NULL;
	gnomescreensaver_lock = NULL;
	gnomeScreensaverCheck = NULL;
	xlock = NULL;

	got_XScreensaver = false;
	checkDPMSStatus();
	check_xscreensaver_timer_runs = false;
	
	SCREENSAVER_STATUS = -1;
	screen_save_dcop_ref = DCOPRef( "kdesktop", "KScreensaverIface" );
	
	check_xscreensaver_timer =  new QTimer( this );
	connect( check_xscreensaver_timer, SIGNAL(timeout()), this, SLOT(xscreensaver_ping() ));

	SCREENSAVER_STATUS = checkScreenSaverStatus();
	kdDebugFuncOut(trace);
}

/*! The default destructor of the class screen */
screen::~screen() {
	kdDebugFuncIn(trace);
}

/*!
* To enable/disable the KScreensaver/Xscreensaver.
* \param enable true: activate/enable screensaver / false: deactivate/disable screensacer
* \return the result of try to set the screensaver
* \retval true if screensaver set correct
* \retval false if there is a error
*/
bool screen::setScreenSaver(bool enable){
	kdDebugFuncIn(trace);

	if(SCREENSAVER_STATUS == -1) SCREENSAVER_STATUS = checkScreenSaverStatus();
	if((SCREENSAVER_STATUS == 1) || (SCREENSAVER_STATUS == 0)){	
		screen_save_dcop_ref.send( "enable", enable);
		kdDebugFuncOut(trace);
		return true;
	}

	if(SCREENSAVER_STATUS == 11 || SCREENSAVER_STATUS == 10){
		if(enable) {
			check_xscreensaver_timer->stop();
			check_xscreensaver_timer_runs = false;
		}
		else {
			check_xscreensaver_timer->start( xScreenSaver_timer_interval );
			check_xscreensaver_timer_runs = true;
		}
		kdDebugFuncOut(trace);
		return true;	
	} else {
		kdDebugFuncOut(trace);
		return false;
	}
}


/* * * * *
 * this inspired, partly copied and modified from MPlayer Code (1.05pre)
 * START Section
 */
//! to handle information about wrong XWindows
/*!
* This integer represent information about XWindows.
* - got_badwindow == True:  if the searched Window is wrong or if there is a problem/error
* - got_badwindow == False: if there is no problem/error
*/
static int got_badwindow;
//! ErrorHandler 
/*! Errorhandler for all X-Server related operations */
static XErrorHandler defaultHandler;

//! to find/handle bad XWindows / XSetErrorHandler
/*!
* This function is involved by the search for the window of the Xscreensaver. By this 
* function we seperate the BadWindow-error and set \ref got_badwindow if we get a BadWindow.
* For more information take a look at the XServer Documentation!
* \param dpy pointer to the active X-Display
* \param error pointer to XErrorEvent
* \return a info about a window-error
* \retval 0 if the window is a BadWindow as a integer 
* \retval n the errorID/XErrorHandler if there is a error != BadWindow
*/
static int badwindow_handler(Display * dpy, XErrorEvent * error) {
	if (error->error_code != BadWindow)
		return (*defaultHandler) (dpy, error);
	got_badwindow = True;
	return 0;
}

//! to find the X-window of XScreensaver
/*!
* This function search for the X-Window of the active/activated XScreensaver.
* \param dpy pointer to the active X-Display
* \return The WindowID of the active XScreensaver.
* \retval 0 if no Windows/Xscreensaver was found
* \retval n ID of the founded XScreensaver window
*/
static Window find_xscreensaver_window(Display * dpy) {
	kdDebugFuncIn(trace);

	Window root = RootWindowOfScreen(DefaultScreenOfDisplay(dpy));
	Window root2, parent, *kids;
	Window retval = 0;
	Atom xs_version;
	unsigned int i;
	unsigned int nkids = 0;

	xs_version = XInternAtom(dpy, "_SCREENSAVER_VERSION", True);

	if (!(xs_version != None && XQueryTree(dpy, root, &root2, &parent, &kids, &nkids) 
		&& kids && nkids)) {
		kdDebugFuncOut(trace);
		return 0;
	}

	defaultHandler = XSetErrorHandler(badwindow_handler);

	for (i = 0; i < nkids; i++) {
		Atom type;
		int format, status;
		unsigned long nitems, bytesafter;
		unsigned char *v;

		got_badwindow = False;
		status = XGetWindowProperty(dpy, kids[i], xs_version, 0, 200, False, XA_STRING, &type, &format, 
					    &nitems, &bytesafter, &v);
		XSync(dpy, False);
		if (got_badwindow) status = BadWindow;

		if (status == Success && type != None){
			retval = kids[i];
			break;
		}
	}
	XFree(kids);
	XSetErrorHandler(defaultHandler);
	kdDebugFuncOut(trace);
	return retval;
}

/*!
* This function is used to ping the XScreensaver. There is no direct way to stop
* the XScreensaver without to kill/stop an restart or modify config-files of the 
* current user. \n \n
* We ping the xscreensaver as defined in the QTimer-interval \ref xScreenSaver_timer_interval .
* The value of \ref xScreenSaver_timer_interval is 58 sec at the moment. The intervall must be
* smaller then 1 minute (this is the smallest interval of Xscreensaver a.t.m.).
*/
void screen::xscreensaver_ping(){
	kdDebugFuncIn(trace);

	if(!got_XScreensaver) {
		mDisplay = qt_xdisplay();
		xs_windowid = find_xscreensaver_window(mDisplay);
		
		Atom deactivate = XInternAtom(mDisplay, "DEACTIVATE", False);
		Atom screensaver = XInternAtom(mDisplay, "SCREENSAVER", False);
		
		ev.xany.type = ClientMessage;
		ev.xclient.display = mDisplay;
		ev.xclient.window = xs_windowid;
		ev.xclient.message_type = screensaver;
		ev.xclient.format = 32;
		memset(&ev.xclient.data, 0, sizeof(ev.xclient.data));
		ev.xclient.data.l[0] = (long) deactivate;
		
		if(xs_windowid != 0) got_XScreensaver = true;
	}
	if(got_XScreensaver){
		if(XSendEvent(mDisplay, xs_windowid, False, 0L, &ev) == 0){
			if(check_xscreensaver_timer->isActive()) {
				check_xscreensaver_timer->stop();
				this->got_XScreensaver = false;
			}
		}
		XSync(mDisplay, False);
	}

	kdDebugFuncOut(trace);
}
/*
 * END Section
 * * * * * * */ 

/*!
* Checks if KScreenSaver or XscreenSaver is activated
* \return The result of the check as an integer value.
* \retval 0 KScreensaver is disabled
* \retval 1 KScreensaver is activated
* \retval 10 Xscreensaver is not found or not running
* \retval 11 Xscreensaver is activated
* \retval 99 gnome-screensaver check is running
* \retval -1 else
*/
int screen::checkScreenSaverStatus() {
	kdDebugFuncIn(trace);
	
	bool get_reply = false;	
	int kScreenSaver_tmp_status = -1;
	int check = -1;

	// check for KScreenSaver
	DCOPReply reply = screen_save_dcop_ref.call("isEnabled()");
	if(reply.isValid()){
		if(reply.get(get_reply)){
			if(get_reply) return 1;
			/* don't return status her because we must also check if
			 * XScreensaver is activated !!!
			 */ 
			else kScreenSaver_tmp_status = 0;
		}
	}
	// check for XScreensaver
	if (got_XScreensaver) return 11;
	else if(!got_XScreensaver) {
		Display *dpy = qt_xdisplay();
		Window windowid = find_xscreensaver_window(dpy);
		if(windowid == 0) {
			//Xscreensaver not detected
			check_xscreensaver_timer->stop();
			// KScreensaver activ and no XScreensaver found
			if(kScreenSaver_tmp_status == 0) return 0;
			// no KScreensaver and no XScreensaver found
			else check = 10;
		}
		else return 11;
	}
	
	// check for gnome-screen-saver
	if (check == 10) {
		delete gnomeScreensaverCheck;

		gnomeScreensaverCheck = new KProcess;
		*gnomeScreensaverCheck << "gnome-screensaver-command" << "--query";

		connect( gnomeScreensaverCheck , SIGNAL(processExited(KProcess *)),SLOT(getGSExited(KProcess *)));
		
		if(!gnomeScreensaverCheck->start(KProcess::NotifyOnExit)) 
		{
			delete gnomeScreensaverCheck;
			gnomeScreensaverCheck = NULL;
			return 10;
		}
		else return 99;	
	}

	return -1;
}

/*!
 * \b SLOT which called if the call of gnomescreensaver-command exited
 * \param gnomecheckcommand the KPocess which called this SLOT
 */
void screen::getGSExited (KProcess *gnomecheckcommand) {
	kdDebugFuncIn(trace);

	if (gnomecheckcommand->normalExit()){
		if (gnomecheckcommand->exitStatus() == 1) SCREENSAVER_STATUS = 10;
		else if (gnomecheckcommand->exitStatus() == 0) SCREENSAVER_STATUS = 20;
	} else {
		SCREENSAVER_STATUS = 10;
	}

	delete gnomeScreensaverCheck;
	gnomeScreensaverCheck=NULL;	

	kdDebugFuncOut(trace);
	return;
} 
 
/*!
* This function check if DMPS is activated on the active X-Server.
* \return Integer value with the result of the check
* \retval 1 if DMPS is enabled
* \retval 0 if DMPS is disabled
* \retval -1 if there is a error
*/
int screen::checkDPMSStatus(){
	kdDebugFuncIn(trace);

	CARD16 state;
	BOOL onoff;
	int dummy;
	
	Display *dpy = qt_xdisplay();
	
	if (!DPMSQueryExtension(dpy, &dummy, &dummy) || !DPMSCapable(dpy)){
		has_DPMS = false;
		kdDebugFuncOut(trace);
		return -1;
	}
	else
		has_DPMS = true;
	
	DPMSInfo(dpy, &state, &onoff);
	if(onoff) {
		kdDebugFuncOut(trace);
		return 1;
	} else {
		kdDebugFuncOut(trace);
		return 0;
	}
}


/*!
* To set DPMS on X-Server on/off.
* \param enable true: activate/enable DMPS / false: deactivate/disable DPMS
* \return the result of try to set DPMS 
* \retval true if DMPS correct set false
* \retval false if fail (DPMS not supported)
* \todo \li check/evaluate for errormessages
*       \li check if DPMS-Settings correct set if DPMSEnable(dpy) used 
*/
bool screen::setDPMS( bool enable ){	
	kdDebugFuncIn(trace);

	defaultHandler = XSetErrorHandler(badwindow_handler);

	Display *dpy = qt_xdisplay();
	
	int dummy;
	if (!DPMSQueryExtension(dpy, &dummy, &dummy) || !DPMSCapable(dpy)){
		has_DPMS = false;
		XSetErrorHandler(defaultHandler);		
		kdDebugFuncOut(trace);
		return false;
	}
	
	if(enable) DPMSEnable(dpy);
	else DPMSDisable(dpy);
	
	XFlush(dpy);
	XSetErrorHandler(defaultHandler);
	
	kdDebugFuncOut(trace);
	return true;
}

/*!
* This function set the Timeouts for DPMS for the X-Server.
* \param standby_timeout time in seconds to stand-by the display as integer value 
* \param suspend_timeout time in seconds to suspend the display as integer value
* \param off_timeout time in seconds to switch off the display as integer value
* \return the result of try to set DPMS-Timeouts
* \retval true if DMPS correct set false
* \retval false if fail (DPMS not supported)
*/
bool screen::setDPMSTimeouts( int standby_timeout, int suspend_timeout, int off_timeout){
	kdDebugFuncIn(trace);

	//XErrFunc defaultHandler;
	defaultHandler = XSetErrorHandler(badwindow_handler);

	Display *dpy = qt_xdisplay();
	
	int dummy;
	if (!DPMSQueryExtension(dpy, &dummy, &dummy) || !DPMSCapable(dpy)){
		has_DPMS = false;
		XSetErrorHandler(defaultHandler);		
		kdDebugFuncOut(trace);
		return false;
	}
	
	DPMSSetTimeouts(dpy, 60 * standby_timeout, 60 * suspend_timeout, 60 * off_timeout);
	
	XFlush(dpy);
	XSetErrorHandler(defaultHandler);
	kdDebugFuncOut(trace);
	return true;
}

/*!
* Use this function to lock the screen. This function use automatically the right
* method for KDE or GNOME.
* \return boolean with the result of the operation
* \retval true if the requested method worked
* \retval false if there was a error
*/
bool screen::lockScreen(){
	kdDebugFuncIn(trace);

	// screensaver status known?
	if(SCREENSAVER_STATUS == -1) SCREENSAVER_STATUS = checkScreenSaverStatus();

	// set lock for KScreensaver
	if((SCREENSAVER_STATUS == 1) || (SCREENSAVER_STATUS == 0)){	
		DCOPReply reply = screen_save_dcop_ref.call("lock");
		if ( reply.isValid() ) {
			return true;
		} else {
			kdWarning(debug_area) << "Could not lock KScreensaver, try XScreensaver as fallback." << endl;
			goto xscreensaver;
		}
	}
	// set lock for XScreensaver
	else if(SCREENSAVER_STATUS == 11){
xscreensaver:
		delete xscreensaver_lock;		

		xscreensaver_lock = new KProcess;
		*xscreensaver_lock << "xscreensaver-command" << "-lock";
	        connect(xscreensaver_lock, SIGNAL(processExited(KProcess*)),
	                this, SLOT(cleanProcess(KProcess*)));

		bool status = xscreensaver_lock->start(KProcess::DontCare);
		if(!status)
		{
			delete xscreensaver_lock;
			xscreensaver_lock = NULL;
		}
		return status;
	}
	// lock with gnome-screen-saver
	else if(SCREENSAVER_STATUS == 20){
		delete gnomescreensaver_lock;

		gnomescreensaver_lock = new KProcess;
		*gnomescreensaver_lock << "gnome-screensaver-command" << "--lock";
	
	        connect(gnomescreensaver_lock, SIGNAL(processExited(KProcess*)),
	                this, SLOT(cleanProcess(KProcess*)));	
		bool status = gnomescreensaver_lock->start(KProcess::DontCare);
		if(!status)
		{
			delete gnomescreensaver_lock;
			gnomescreensaver_lock=NULL;
		}
		return status;
	}
	// set lock for xlock --> no kscreensaver, no xscreensaver present and
	// the check for gnome screensaver is not finished. This should normaly
	// not happen, but in this case we use xlock
	else if(SCREENSAVER_STATUS == 10 || SCREENSAVER_STATUS == 99){
		delete xlock;
		
		xlock = new KProcess;
		*xlock << "xlock"; //<< "-mode" << "blank";
                connect(xlock, SIGNAL(processExited(KProcess*)),
                           this, SLOT(cleanProcess(KProcess*)));
		bool status = xlock->start(KProcess::DontCare);
		if(!status)
		{
			delete xlock;
			xlock = NULL;
		}
		return status;
	}
	else return false;
}

/*!
* Use this function to lock the screen with a specified lock method.
* \param lock_withMethod a QString, which contain the alias for the lock
*			 command.
* \return boolean with the result of the operation
* \retval true if the requested method worked
* \retval false if there was a error
* \todo check if we should also set blank only if the user would like!!!
*/
bool screen::lockScreen( QString lock_withMethod ) {
	kdDebugFuncIn(trace);

	if (lock_withMethod == "automatic") {
		lockScreen();
		return true;
	}
	else if (lock_withMethod == "xlock") {
		delete xlock;
		
		xlock = new KProcess;
		*xlock << "xlock";
                connect(xlock, SIGNAL(processExited(KProcess*)),
                           this, SLOT(cleanProcess(KProcess*)));

                bool status = xlock->start(KProcess::DontCare);
		if(!status)
		{
			delete xlock;
			xlock=NULL;
		}

		return status;
	}
	else if (lock_withMethod == "gnomescreensaver") {
		gnomescreensaver_lock = new KProcess;
		*gnomescreensaver_lock << "gnome-screensaver-command" << "--lock";
                connect(gnomescreensaver_lock, SIGNAL(processExited(KProcess*)),
                                                this, SLOT(cleanProcess(KProcess*)));

		bool status = gnomescreensaver_lock->start(KProcess::DontCare);
		if(!status)
		{
			delete gnomescreensaver_lock;
			gnomescreensaver_lock = NULL;
		}
		return status;
	}
	else {
		// screensaver status known?
		SCREENSAVER_STATUS = checkScreenSaverStatus();
		
		if (lock_withMethod == "kscreensaver") {
			if((SCREENSAVER_STATUS == 1) || (SCREENSAVER_STATUS == 0)){	
				DCOPReply reply = screen_save_dcop_ref.call("lock");
				if ( reply.isValid() ) {
					return true;
				} else {
					kdWarning(debug_area) << "Could not call lock for KScreensaver, try XScreensaver "
						    << "as fallback." << endl;
					goto xscreensaver;
				}
			}
			else return false;
		}
		else if (lock_withMethod == "xscreensaver") {
			if(SCREENSAVER_STATUS == 11){
xscreensaver:
				delete xscreensaver_lock; 
				
				xscreensaver_lock = new KProcess;
				*xscreensaver_lock << "xscreensaver-command" << "-lock";
		                connect(xscreensaver_lock, SIGNAL(processExited(KProcess*)),
                                                this, SLOT(cleanProcess(KProcess*)));

				bool status = xscreensaver_lock->start(KProcess::DontCare);
				if(!status)
				{
					delete xscreensaver_lock;
					xscreensaver_lock = NULL;
				}
				return status;
			}
			else return false;
		}
		else return false;
	}
	return false;
}

/*!
* Use this function to set the screensaver to 'blank only' instead of 
* using the in KDE or GNOME set screensaver.
* \todo find a way to set the xscreensaver to 'blank only'
*/
void screen::blankOnlyScreen( bool blankonly ){
	kdDebugFuncIn(trace);

	if(SCREENSAVER_STATUS == -1) SCREENSAVER_STATUS = checkScreenSaverStatus();
	// set KScreensaver
	if((SCREENSAVER_STATUS == 1) || (SCREENSAVER_STATUS == 0)){	
		screen_save_dcop_ref.send("setBlankOnly", blankonly);
	}
	// set XScreensaver
	else if(SCREENSAVER_STATUS == 11){
		// implement something !!!
	}

	kdDebugFuncOut(trace);
}

/*!
* Use this function to reset the KDE screensaver/DPMS settings
* \return boolean with the result of the operation
* \retval true if the reset was called
* \retval false if there was a error
*/
bool screen::resetKDEScreensaver(){
	kdDebugFuncIn(trace);

	if(SCREENSAVER_STATUS == -1) 
		SCREENSAVER_STATUS = checkScreenSaverStatus();

	// do this only if the screensaver is not running
	if(SCREENSAVER_STATUS == 0){	
		DCOPReply reply = screen_save_dcop_ref.call("configure");
		if ( reply.isValid() ) {
			kdDebugFuncOut(trace);
			return true;
		} else {
			kdWarning(debug_area) << "Could not call configure() for the KDE screensaver." << endl;
			kdDebugFuncOut(trace);
			return false;
		}
	} else {
		kdDebugFuncOut(trace);
		return false;
	}
}

/*!
* Use this function to reset the xscreensaver settings
* \return boolean with the result of the operation
* \retval true if the reset was called
* \retval false if there was a error
*/
bool screen::resetXScreensaver(){
	kdDebugFuncIn(trace);

	if(checkScreenSaverStatus() == 11) {
		
		delete xscreensaver_reset;		

		xscreensaver_reset = new KProcess;
		*xscreensaver_reset << "xscreensaver-command" << "-restart";
	        connect(xscreensaver_reset, SIGNAL(processExited(KProcess*)),
	                this, SLOT(cleanProcess(KProcess*)));

		bool status = xscreensaver_reset->start(KProcess::DontCare);
		if(!status)
		{
			delete xscreensaver_reset;
			xscreensaver_reset = NULL;
		}
		kdDebugFuncOut(trace);
		return status;
	} else {
		kdDebugFuncOut(trace);
		return false;
	}
}

/*!
* function to call xset commandline tool to force shutdown the display with dpms
*/
void screen::forceDPMSOff() {
	kdDebugFuncIn(trace);
	
	KProcess *xset = new KProcess;
	*xset << "xset" << "dpms" << "force" << "off";
        connect(xset, SIGNAL(processExited(KProcess*)),
                this, SLOT(cleanProcess(KProcess*)));
	if(!xset->start())
	{
		delete xset;
	}

	kdDebugFuncOut(trace);
}

/*!
 * function to clean KProcess objects
 */
void screen::cleanProcess(KProcess* proc)
{
	delete proc;
	proc = NULL;
}

/*!
* function to fake a key event for the shift key. Use this to show the login
* passwd dialog after suspend if we locked the screen.
*/
void screen::fakeShiftKeyEvent() {
	kdDebugFuncIn(trace);

	Display *dpy = qt_xdisplay();

	if (dpy) {
		XTestFakeKeyEvent(dpy, 62, 1, 0);
		XTestFakeKeyEvent(dpy, 62, 0, 0);
		
		XFlush(dpy);
	}

	kdDebugFuncOut(trace);
	return;
}

#include "screen.moc"
