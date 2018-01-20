/***************************************************************************
 *   Copyright (C) 2007 by Danny Kukawka                                   *
 *                         <dkukawka@suse.de>, >danny.kukawka@web.de>      *
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
*  \file 	autodimm.h
*  \brief 	Headerfile for autodimm.cpp and the class \ref autodimm.
*/
/*! 
*  \class 	autodimm
*  \brief 	class for 'dimm the display on user inactivity' related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \version 	0.0.1
*  \date    	2007
*/

#ifndef AUTODIMM_H
#define AUTODIMM_H

// from project
#include "inactivity.h"

class autodimm : public inactivity
{
	Q_OBJECT

public:
	//! default constructor
	autodimm();
	//! default destructor
	~autodimm();

	//! to start check if the user is active again
	void startCheckForActivity();

signals:
	//! signal emited if the user is active again
	void UserIsActiveAgain();

private: 
	//! idle time from the last check
	unsigned long lastIdleTime;

	//! QTimer intervall for the Timer to recheck for user activity
	/*!
	 * The time intervall to recheck for the activity of the user.
	 * The timeslice is currently 1 sec.
	 */
	static const int RECHECK_INTERVALL = 1000;

	//! QTimer for check activity
	/*!
	 * This timer is used to check if the user get active again.
	 * The timerinterval is defined trough \ref RECHECK_INTERVALL .
	 */
	QTimer *checkActivity;
	
private slots:
	//! to poll X to get info if the user is active again
	void pollActivity();

};

#endif
