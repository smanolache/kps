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

/*! \file countdowndialog.cpp
 * \brief 	In this file can be found the countdown dialog related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de, danny.kukawka@web.de>
 * \date    	2007
 */

// own header
#include "countdowndialog.h"
#include "kpowersave_debug.h"

// KDE headers:
#include <klocale.h>
#include <kiconloader.h>
#include <kprogress.h>

// QT headers:
#include <qdialog.h>
#include <qlabel.h> 
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qtimer.h>

/*! This is the default constructor of class countDownDialog . */
countDownDialog::countDownDialog( int timeout, QWidget *parent, const char *name)
		:countdown_Dialog(parent, name, false, Qt::WStyle_StaysOnTop | Qt::WDestructiveClose )
{
	kdDebugFuncIn(trace);
	chancel = false;
	remaining = timeout;
	timeOut = timeout;

	PROGRESS = new QTimer(this);
	connect(PROGRESS, SIGNAL(timeout()), this, SLOT(updateProgress()));

	this->setCaption(i18n("KPowersave"));

	kdDebugFuncOut(trace);
}

/*! This is the default destructor of class countDownDialog . */
countDownDialog::~countDownDialog() {
	kdDebugFuncIn(trace);
	// no need to delete child widgets, Qt does it all for us
	emit dialogClosed(chancel);

	kdDebugFuncOut(trace);
}

/*! 
 * This used to set Icon/pixmap for the dialog.
 * \param type QString with the type of the current suspend
 *             to set the pixmap in the dialog
 */
void countDownDialog::setPixmap( QString type )
{
	QPixmap pixmap = 0;

	if(type.startsWith("suspend2disk")){
		pixmap = KGlobal::iconLoader()->loadIcon("suspend_to_disk", KIcon::NoGroup, KIcon::SizeLarge);
	} else if (type.startsWith("suspend2ram")) {
		pixmap = KGlobal::iconLoader()->loadIcon("suspend_to_ram", KIcon::NoGroup, KIcon::SizeLarge);
	} else if (type.startsWith("standby")) {
		pixmap = KGlobal::iconLoader()->loadIcon("stand_by", KIcon::NoGroup, KIcon::SizeLarge);
	} else {
		pixmap = KGlobal::iconLoader()->loadIcon("kpowersave", KIcon::NoGroup, KIcon::SizeLarge);
	}
	iconPixmap->setPixmap( pixmap );
}

/*!
 * To set the message to the dialog, which should be shown to the user.
 * \param text	QString with the message. 
 */
void countDownDialog::setMessageText(QString text) {
	kdDebugFuncIn(trace);

	if (!text.isEmpty()) {
		textLabel->setText(text);
	}

	kdDebugFuncOut(trace);
}

/*!
 * To show the dialog and start the countdown.
 * \return boolean with the result of the operation
 * \retval true		if the dialog could get displayed
 * \retval false	if there is any problem
 */
bool countDownDialog::showDialog() {
	kdDebugFuncIn(trace);

	bool _retval = false;

	if (!textLabel->text().isEmpty() && timeOut > 0) {
		// init the progressbar
		progressBar->setFormat(i18n("%1 seconds").arg(remaining));
		progressBar->setPercentageVisible(true);
		progressBar->setProgress(100);
		progressBar->setEnabled(true);

		this->adjustSize();
		this->show();

		PROGRESS->start(1000, true);
	}

	kdDebugFuncOut(trace);
	return _retval;
}

/*!
 * \b SLOT to get the event if the 'Cancel' button was pressed.
 */
void countDownDialog::pB_cancel_pressed() {
	kdDebugFuncIn(trace);

	if (PROGRESS->isActive())
		PROGRESS->stop();

	chancel = true;
	close();

	kdDebugFuncOut(trace);
}

/*!
 * \b SLOT to handle the change of the progressbar.
 */
void countDownDialog::updateProgress() {
	kdDebugFuncIn(trace);

	if (remaining == 0 ) {
		if (PROGRESS->isActive())
			PROGRESS->stop();
		
		chancel = false;
		close();
	} else if ( remaining > 0) {
		int setTo = (int)((100.0/(float)timeOut)*(float)remaining);
		
		// set the progressBar
		progressBar->setFormat(i18n("%1 seconds").arg(remaining));
		progressBar->setPercentageVisible(true);
		progressBar->setProgress(setTo);
		progressBar->setEnabled(true);

		// increase counter
		remaining--;
		// start needed timer
		PROGRESS->start(1000, true);
	}

	kdDebugFuncOut(trace);
}

#include "countdowndialog.moc"
