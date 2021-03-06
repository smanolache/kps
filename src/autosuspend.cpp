/***************************************************************************
 *   Copyright (C) 2004 by Danny Kukawka                                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

/*! \file 	autosuspend.cpp
 * \brief 	In this file can be found the autosuspend related code. 
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2005
 */

#include "autosuspend.h"

/*! The default constructor of the class autosuspend */
autosuspend::autosuspend() : inactivity () {
	kdDebugFuncIn(trace);

}

/*! The default destructor of the class autosuspend */
autosuspend::~autosuspend() {
	kdDebugFuncIn(trace);
}

#include "autosuspend.moc"
