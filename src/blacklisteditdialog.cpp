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
 * \file 	blacklisteditdialog.cpp
 * \brief 	In this file can be found the "blacklist edit dialog" related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2005
 */

// own header
#include "blacklisteditdialog.h"

// KDE headers:
#include <klocale.h>
#include <kiconloader.h>

// QT headers:
#include <qbuttongroup.h>
#include <qdialog.h>
#include <qlabel.h> 
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtooltip.h>

/*! This is the default constructor of class blacklistEditDialog . */
blacklistEditDialog::blacklistEditDialog( QStringList blacklisted, QString captionName, 
					  bool initImport, QWidget *parent, const char *name)
		    :blacklistedit_Dialog(parent, name, false, WDestructiveClose )
{
	blacklist = blacklisted;
	
	changed = initImport;
	
	pB_add->setEnabled(false);
	pB_remove->setEnabled(false);
	
	lB_blacklist->insertStringList(blacklist);
	lB_blacklist->sort();
	
	if (captionName.startsWith(i18n("General Blacklist"))) 
		this->bG_scheme->setTitle(captionName);
	else 
		this->bG_scheme->setTitle( i18n("Scheme: ") + captionName);
	this->setIcon(SmallIcon("configure", QIconSet::Automatic));
	buttonCancel->setIconSet(SmallIconSet("cancel", QIconSet::Automatic));
	buttonOk->setIconSet(SmallIconSet("ok", QIconSet::Automatic));
	pB_add->setIconSet(SmallIconSet("forward", QIconSet::Automatic));
	pB_remove->setIconSet(SmallIconSet("back", QIconSet::Automatic));
	
}

/*! This is the default destructor of class blacklistEditDialog . */
blacklistEditDialog::~blacklistEditDialog()
{
	// no need to delete child widgets, Qt does it all for us
}

/*!
 * SLOT: called if the 'ok' button clicked. This SLOT emit
 * \ref config_finished() and close the dialog.
 */
void blacklistEditDialog::buttonOk_released() {

	if(changed == true) {
		changed = false;
		emit config_finished( blacklist );
	}
	close();
}


/*!
 * SLOT: called if the 'cancel' button clicked. This SLOT close
 * the dialog.
 */
void blacklistEditDialog::buttonCancel_released(){
	changed = false;
	close();
}


/*!
 * SLOT: called if the 'remove' button clicked. The SLOT try to remove
 * the selected item from the QListBox and the QStringList \ref blacklist .
 */
void blacklistEditDialog::pB_remove_released(){

	if(blacklist.remove(lB_blacklist->selectedItem()->text()) > 0) {
		lB_blacklist->removeItem(lB_blacklist->currentItem());
		lB_blacklist->sort();
		changed = true;
		
		pB_remove->setEnabled(false);
		tLabel_info->setText(i18n("Selected entry removed."));
	}
	else {
		tLabel_info->setText(i18n("Could not remove the selected entry."));
	}
}


/*!
 * SLOT: called if the 'add' button clicked. The SLOT try to add the string from 
 * the QLineEdit lE_blacklist to the QListBox and the QStringList \ref blacklist .
 */
void blacklistEditDialog::pB_add_released(){
	
	QString text = lE_blacklist->text();
	// remove the whitespaces and check if text is empty
	if(text.stripWhiteSpace() != "") {
		// check if the entry is already present
		if(!lB_blacklist->findItem(text, Qt::ExactMatch)) {
			lB_blacklist->insertItem(text);
			lB_blacklist->sort();
			blacklist.append(text);
			blacklist.sort();
			tLabel_info->setText(i18n("Inserted new entry."));
			changed = true;
		}
		else {
			tLabel_info->setText(i18n("Entry exists already. Did not insert new entry."));
		}
	}
	else tLabel_info->setText(i18n("Empty entry was not inserted."));
	
	// set the widgets back to default
	lE_blacklist->setText("");
	pB_remove->setEnabled(false);
	pB_add->setEnabled(false);
}


/*!
 * SLOT: called if a item in the QListBox lB_blacklist is selected.
 * Here we enable the remove button  pB_remove .
 */
void blacklistEditDialog::lB_blacklist_currentChanged(){
	// enable the remove button
	pB_remove->setEnabled(true);
}


/*!
 * SLOT: called if the input-line in the dialog is modified. Here we enable
 * the add button pB_add .
 */
void blacklistEditDialog::lE_blacklist_textChanged(){
	// enable the add button
	pB_add->setEnabled(true);
}

#include "blacklisteditdialog.moc"
