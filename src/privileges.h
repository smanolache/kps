/***************************************************************************
 *   Copyright (C) 2007 by Danny Kukawka                                   *
 *                         <dkukawka@suse.de, danny.kukawka@web.de>        *
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

/*! 
*  \file 	privileges.h
*  \brief 	Headerfile containing defines for privileges
*  \author 	Danny Kukawka, <dkukawka@suse.de, danny.kukawka@web.de>
*  \date    	2007
*/

#ifndef _PRIVILEGES_H_
#define _PRIVILEGES_H_

#ifdef HAVE_HAL_0_5_10
 #define PRIV_SUSPEND		"org.freedesktop.hal.power-management.suspend"
 #define PRIV_HIBERNATE		"org.freedesktop.hal.power-management.hibernate"
 #define PRIV_STANDBY		"org.freedesktop.hal.power-management.standby"
 #define PRIV_CPUFREQ		"org.freedesktop.hal.power-management.cpufreq"
 #define PRIV_LAPTOP_PANEL	"org.freedesktop.hal.power-management.lcd-panel"
 #define PRIV_SETPOWERSAVE	"org.freedesktop.hal.power-management.set-powersave"
#else
 #define PRIV_SUSPEND		"hal-power-suspend"
 #define PRIV_HIBERNATE		"hal-power-hibernate"
 #define PRIV_STANDBY		"hal-power-standby"
 #define PRIV_CPUFREQ		"hal-power-cpufreq"
 #define PRIV_LAPTOP_PANEL	"hal-power-lcd-panel"
 #define PRIV_SETPOWERSAVE	"hal-power-set-powersave"
#endif

#endif
