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

/*! \file autodimm.cpp
 * \brief 	In this file can be found the autodimm class related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \version 	0.0.1
 * \date    	2007
 */

#include "autodimm.h"

/*! The default constructor of the class autodimm */
autodimm::autodimm() : inactivity() {
	kdDebugFuncIn(trace);	

	lastIdleTime = 0;
	
	checkActivity = new QTimer( this );
	connect( checkActivity, SIGNAL(timeout()), this, SLOT(pollActivity()));

	kdDebugFuncOut(trace);
}

/*! The default destructor of the class autodimm */
autodimm::~autodimm() {
	kdDebugFuncIn(trace);
}

/*!
 * Public function to start to check (poll) if the user get active again.
 */
void autodimm::startCheckForActivity() {
	kdDebugFuncIn(trace);

	lastIdleTime = 0;

	// stop the timer if running
	if (checkActivity->isActive()) 
		checkActivity->stop();
	
	checkActivity->start(RECHECK_INTERVALL, false);
	
	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to call check if the user is active again.
 */
void autodimm::pollActivity() {
	kdDebugFuncIn(trace);

	unsigned long idletime = 0;

	idletime = getXInactivity();
	if (idletime < lastIdleTime) {
		kdDebug(debug_area) << "Looks as if the user is active again (lastIdleTime:" << lastIdleTime 
			  << " , current idletime: " << idletime << ")" << endl;

		// The user is/was active ...
		if (idletime <= 1000) 
			kdDebug(debug_area) << "Looks as if the user was active within the last second" << endl;

		// stop the timer ... no need to let run ... start again if needed
		if (checkActivity->isActive()) checkActivity->stop();
		// emit the signal that the user is active again.
		emit UserIsActiveAgain();
	} else {
		lastIdleTime = idletime;
	}

	kdDebugFuncOut(trace);
}

#include "autodimm.moc"
