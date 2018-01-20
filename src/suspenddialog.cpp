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
 * \file        suspenddialog.cpp
 * \brief 	In this file can be found the suspend dialog related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2005
 */
 
 // KDE - Headers
 #include <klocale.h>
 #include <kiconloader.h>

 // QT - Headers
 #include <qdialog.h>
 #include <qlabel.h>
 #include <qstring.h>
 #include <qpixmap.h>
 #include <qprogressbar.h>

 #include "suspenddialog.h"

/*! This is the default constructor of the class. */
suspendDialog::suspendDialog(QWidget *parent, const char *name)
	:suspend_Dialog(parent, name, true, Qt::WStyle_StaysOnTop | Qt::WDestructiveClose )
{
	this->setIcon(SmallIcon("kpowersave", QIconSet::Automatic));
}

/*! This is the default destructor of the class. */
suspendDialog::~suspendDialog() 
{

}

/*! 
 * This used to set Icon/pixmap for the dialog.
 * \param type QString with the type of the current suspend
 *             to set the pixmap in the dialog
 */
void suspendDialog::setPixmap( QString type )
{
	QPixmap pixmap = 0;
	if(type.startsWith("suspend2disk")){// || type.startsWith("NULL")) {
		pixmap = KGlobal::iconLoader()->loadIcon("suspend_to_disk", KIcon::NoGroup, KIcon::SizeLarge);
	} else if (type.startsWith("suspend2ram")) {
		pixmap = KGlobal::iconLoader()->loadIcon("suspend_to_ram", KIcon::NoGroup, KIcon::SizeLarge);
	} else if (type.startsWith("standby")) {
		pixmap = KGlobal::iconLoader()->loadIcon("stand_by", KIcon::NoGroup, KIcon::SizeLarge);
	} else {
		pixmap = KGlobal::iconLoader()->loadIcon("kpowersave", KIcon::NoGroup, KIcon::SizeLarge);
	}
	setCaption(i18n("Preparing Suspend..."));
	iconPixmap->setPixmap( pixmap );
}

/*! 
 * This used to set the values of progressbar for the dialog.
 * \param percent integer value with current progress stauts of suspend
 */
void suspendDialog::setProgressbar( int percent )
{
	progressBar->setPercentageVisible(true);
	progressBar->setProgress(percent);
}

/*! 
 * This used to set the message of current suspend action to the the dialog.
 * \param messageText QString with the message of the current running suspend
 *                    prepare action
 */   
void suspendDialog::setTextLabel( QString messageText )
{
	message->show();
	message->setText(messageText);
}

#include "suspenddialog.moc"
