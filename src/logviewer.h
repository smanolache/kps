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

#ifndef LOGVIEWER_H
#define LOGVIEWER_H

/*! 
*  \file 	logviewer.h
*  \brief 	Headerfile for logviewer.cpp and the class \ref LogViewer.
*/
/*! 
*  \class 	LogViewer
*  \brief 	class for view logfiles in KPowersave related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \version 	0.0.1
*  \date    	2007
*/

// header of the UI
#include "log_viewer.h"

class LogViewer: public log_viewer {
	
	Q_OBJECT

public:
	//! default constructor
	LogViewer( QString filename, QWidget *parent = 0, const char *name = 0);
	//! default destructor
	~LogViewer();

private:
	//! name of the log file
	QString log_file;

private slots:

	//! called if the user click on 'Close' Button
	void pB_close_clicked();
	//! called if the user click on 'Save As ...' Button
	void pB_save_clicked();
};

#endif
