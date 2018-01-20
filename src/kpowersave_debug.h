/**************************************************************************
*   Copyright (C)                                                         *
*            2004-2007 by Danny Kukawka                                   *
*                            <dkukawka@suse.de>, <danny.kukawka@web.de>   *
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

#ifndef _KPOWERSAVE_DEBUG_H
#define _KPOWERSAVE_DEBUG_H

// Global Header
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Default Header
#include <stdio.h>
#include <stdlib.h>

// QT - Header
#include <qdatetime.h>

// KDE Header
#include <kdebug.h>

/*! 
*  \file 	kpowersave_debug.h
*  \brief 	Headerfile for debug releated defines/macros. Currently this file
*		contains the myDebug(...) macro.
*/

// to store the value over the different classes
extern bool trace;

/*
 * macro to collect time and k_funcinfo information for kdDebug()
 */
#define funcinfo "[" << QTime::currentTime().toString().ascii() << 			\
		 ":" << QTime::currentTime().msec() << "]" <<  k_funcinfo

/*
 * macros to trace function entry and leave points
 */
#define kdDebugFuncIn(traceinfo) do {							\
	if (traceinfo == true)								\
		kdDebug() << funcinfo << "IN " << endl;					\
} while (0)

#define kdDebugFuncOut(traceinfo) do {							\
	if (traceinfo == true) 								\
		kdDebug() << funcinfo << "OUT " << endl;				\
} while (0)


#endif //_KPOWERSAVE_DEBUG_H
