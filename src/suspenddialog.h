/***************************************************************************
 *   Copyright (C) 2005 by Danny Kukawka                                   *
 *                         danny.kukawka@web.de, dkukawka@suse.de          *
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
 
/*! 
*  \file 	suspenddialog.h
*  \brief 	Headerfile for suspenddialog.cpp and the class \ref suspendDialog.
*/
/*! 
*  \class 	suspendDialog
*  \brief 	class for the suspend dialog related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2005
*/

#ifndef SUSPENDDIALOG_H
#define SUSPENDDIALOG_H

#include "suspend_Dialog.h"

class suspendDialog: public suspend_Dialog {

	Q_OBJECT
	
public:

	//! default constructor
	suspendDialog(QWidget *parent = 0, const char *name = 0);
	//! default destructor
	~suspendDialog();
	
	//! set all needed icons
	void setPixmap( QString );
	//! set the value for the progressbar
	void setProgressbar( int );
	//! set the message for the textlabel
	void setTextLabel( QString );

};

#endif
