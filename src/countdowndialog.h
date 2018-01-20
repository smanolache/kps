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

/*! 
*  \file 	countdowndialog.h
*  \brief 	Headerfile for countdowndialog.cpp and the class \ref countDownDialog.
*/
/*! 
*  \class 	countDownDialog
*  \brief 	class for countdown dialog related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2007
*/

#ifndef COUNTDOWNDIALOG_H
#define COUNTDOWNDIALOG_H

#include "countdown_Dialog.h"

class countDownDialog: public countdown_Dialog {

	Q_OBJECT

public:
	
	//! default constructor
	countDownDialog(int timeout, QWidget *parent = 0, const char *name = 0);
	//! default destructor
	~countDownDialog();

	//! set needed icons
	void setPixmap( QString );
	//! to set the message text for the user
	void setMessageText(QString text);
	//! to show up the dialog
	bool showDialog();

private: 
	//! Timer for the timeout
	/*!
	* This timer is used to change the progressbar and to close the dialog if
	* the \ref timeOut is over.
	*/
	QTimer *PROGRESS;

	//! time the dialog should be displayed
	int timeOut;
	//! remaining time to show the dialog
	int remaining;

	//! to store the info if the countdown was chanceled
	bool chancel;

private slots:

	//! to update the progressbar
	void updateProgress();
	//! to catch the event if the 'Chancel' button get pressed 
	void pB_cancel_pressed();

signals:
	//! emited if the dialog get closed
	void dialogClosed( bool result);

};

#endif
