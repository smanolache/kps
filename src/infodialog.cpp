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
 * \file 	infodialog.cpp
 * \brief 	In this file can be found the "information dialog with checkbox" 
 *		related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2005
 */

// own headers
#include "infodialog.h"

// KDE headers:
#include <kiconloader.h>
#include <klocale.h>

// QT headers:
#include <qcheckbox.h>
#include <qdialog.h>
#include <qlabel.h> 
#include <qpushbutton.h>
#include <qstring.h>
#include <qtooltip.h>

/*! This is the default constructor of class infoDialog . */
infoDialog::infoDialog( KConfig *config, QString captionName, QString message, 
			QString dontShowAgainMsg, QString settingsEntryName, 
			QWidget *parent, const char *name)
			:info_Dialog( parent, name, false, Qt::WStyle_StaysOnTop | WDestructiveClose )
{
	if ( message.isEmpty() || (!dontShowAgainMsg.isEmpty() && settingsEntryName.isEmpty()) ||
	     (!dontShowAgainMsg.isEmpty() && (config == 0)))
		close();

	if (config != 0) {	
		settings = config;
		settings->reparseConfiguration();
		if (settings->hasGroup("infoDialog")){
			settings->setGroup("infoDialog");
			if (settings->readBoolEntry(settingsEntryName, false)) {
				dialogDisabled = true;
				//close();
			}
			else 
				dialogDisabled = false;
		}
	}

	buttonOK->setIconSet(SmallIconSet("ok", QIconSet::Automatic));	
	
	QPixmap pixmap = 0;
	pixmap = KGlobal::iconLoader()->loadIcon("messagebox_warning", KIcon::NoGroup, KIcon::SizeMedium);
	iconPixmap->setPixmap( pixmap );

	msgText->setText(message);
	
	if (!captionName.isEmpty()) 
		this->setCaption(i18n("KPowersave") + " - " + captionName);
	else
		this->setCaption(i18n("KPowersave"));

	if (dontShowAgainMsg.isEmpty()) {
		dontShowAgain->setHidden(true);
	} else {
		entryName = settingsEntryName;
		dontShowAgain->setText(dontShowAgainMsg);
		dontShowAgain->setHidden(false);
	}
	this->adjustSize();
}

/*! This is the default destructor of class infoDialog . */
infoDialog::~infoDialog()
{
	// no need to delete child widgets, Qt does it all for us
}

/*!
 * Use this function to get the value of \ref dialogDisabled.
 * \return boolean with value of \ref dialogDisabled
 * \retval true if disabled by user before
 * \retval false if not
 */
bool infoDialog::dialogIsDisabled() {
	return dialogDisabled;
}

/*!
 * SLOT: called if the 'ok' button clicked. This SLOT sync the settings
 * and close the dialog.
 */
void infoDialog::ButtonOK_clicked() {

	if (!entryName.isEmpty() && dontShowAgain->isVisible()) {
		settings->setGroup( "infoDialog" );
		settings->writeEntry( entryName, dontShowAgain->isChecked());
		settings->sync();
	}
	close();
}

#include "infodialog.moc"
