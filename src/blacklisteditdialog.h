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
*  \file 	blacklisteditdialog.h
*  \brief 	Headerfile for blacklisteditdialog.cpp and the class \ref 
*	        blacklistEditDialog .
*/
/*! 
*  \class 	blacklistEditDialog
*  \brief 	class for dialog to edit the blacklist related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2005
*/

#ifndef BLACKLISTEDITDIALOG_H
#define BLACKLISTEDITDIALOG_H

#include "blacklistedit_Dialog.h"

class blacklistEditDialog: public blacklistedit_Dialog {

	Q_OBJECT

public:
	
	//! default constructor
	blacklistEditDialog(QStringList blacklisted, QString captionName, bool initImport = false, 
			    QWidget *parent = 0, const char *name = 0);
	//! default destructor
	~blacklistEditDialog();

private:

	//! QStringList with the blacklisted processes 
	/*!
	 * This QStringList contains the given blacklisted processes
	 * for edit. If the dialog changed, we change also this list.
	 */
	QStringList blacklist;
	
	//! to tell if the blacklist was changed
	/*! 
	 * This boolean value tells if the current blacklist was changed.
	 * \li true:  if the blacklist changed
	 * \li false: if the blacklist isn't changed
	 */
	bool changed;
	
	
private slots:

	//! called if the 'ok' button clicked
	void buttonOk_released();
	//! called if the 'cancel' button clicked
	void buttonCancel_released();
	//! called if the 'remove' button clicked
	void pB_remove_released();
	//! called if the 'add' button clicked
	void pB_add_released();
	//! called if a item of the listbox selected
	void lB_blacklist_currentChanged();
	//! called if something input in the QLineEdit
	void lE_blacklist_textChanged();

signals:

	//! signal emited if the configuration finished
	/*!
	 * This signal is emited if the configuration is finished 
	 * and the blacklist was modified-
	 * \return QStringList (the modified blacklist)
	 */
	void config_finished( QStringList );
	
};

#endif
