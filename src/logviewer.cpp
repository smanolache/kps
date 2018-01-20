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

/*! \file logviewer.cpp
 * \brief 	In this file can be found the LogViewer related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \version 	0.0.1
 * \date    	2007
 */
 
// QT header
#include <qfile.h>
#include <qtextstream.h>

// KDE header
#include <ktextedit.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>

// own header
#include "logviewer.h"

/*! This is the default constructor of the class LogViewer. */
LogViewer::LogViewer( QString filename, QWidget *parent, const char *name)
		      :log_viewer(parent, name, false, WDestructiveClose ) {

	this->setCaption(i18n("KPowersave Logfile Viewer: %1").arg(filename));
	
	if (!QFile::exists ( filename ))
		return;

	log_file = filename;

	QFile file (log_file);
	if (file.open(IO_ReadOnly)) {
		QTextStream stream ( &file );
		kTextEdit->setText (stream.read());
		kTextEdit->setReadOnly(true);
	}
	file.close();
}

/*! This is the default destructor of the class LogViewer. */
LogViewer::~LogViewer(){
	 // no need to delete child widgets, Qt does it all for us
}

/*!
 * SLOT: Called if the user click on 'Close' Button
 */
void LogViewer::pB_close_clicked() {

	close();
}

/*!
 * SLOT: Called if the user click on 'Save As ...' Button
 */
void LogViewer::pB_save_clicked() {

	QString sFileName;
	bool tryagain = true;

	while (tryagain == true) {
		int answer;
		QString msg;

		sFileName = KFileDialog::getSaveFileName( QDir::homeDirPath() );
		QFileInfo info (sFileName);

		if (QFile::exists(sFileName) && info.isWritable() && info.isReadable() && info.isFile()) {
			msg = i18n("File already exist. Overwrite the file?");
			answer = KMessageBox::questionYesNo(this, msg , i18n("Error while save logfile"));
			if (answer == KMessageBox::Yes) {
				tryagain = false;
			}
		} else if (QFile::exists(sFileName)) {
			msg = i18n("File already exist.");
			answer = KMessageBox::warningContinueCancel(this, msg , 
								    i18n("Error while save logfile"),
								    i18n("Try other filename ..."));
			if (answer == KMessageBox::Cancel) {
				tryagain = false;
				return;
			}
		} else {
			tryagain = false;
		}
	}

	QFile in(log_file);
	QFile out(sFileName);
	if (in.open(IO_ReadOnly)) { 
		if (out.open(IO_WriteOnly)) {
			QByteArray input(4096);
			long l = 0;
			while (!in.atEnd()) {
				l = in.readLine(input.data(), 4096);
				out.writeBlock(input, l);
			}
			out.close();
		}
		in.close();
	}
}

#include "logviewer.moc"
