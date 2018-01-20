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
*  \file 	infodialog.h
*  \brief 	Headerfile for infoDialog.cpp and the class \ref infoDialog .
*/
/*! 
*  \class 	infoDialog
*  \brief 	class for information dialog with checkbox (e.g. for DontShowAgain) 
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2005
*/

#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <kconfig.h>

#include "info_Dialog.h"

class infoDialog: public info_Dialog {

	Q_OBJECT

public:
	
	//! default constructor
	infoDialog( KConfig *config = 0, QString captionName = QString(), QString message = QString(), 
		    QString dontShowAgainMsg = QString(), QString settingsEntryName = QString(),
		    QWidget *parent = 0, const char *name = 0);
	//! default destructor
	~infoDialog();

	//! return the value of dialogDisabled
	bool dialogIsDisabled();

private:

	//! the pointer to the settings of kpowersave, get from constructor
	KConfig *settings;

	//! name of the entry in the settings  
        /*! QString store the name of the settings entry to 
	 *  store settings of 'DontShowAgain' checkbox
	 */
	QString entryName;

	//! represent if the dialog is disabled by user
	/*!
	* This boolean tell if the dialog is already disabled by user settings
	* \li true:  if dialog is disabled
	* \li false: if not
	*/
	bool dialogDisabled;

private slots:

	//! called if the 'ok' button clicked
	void ButtonOK_clicked();
	
};

#endif
