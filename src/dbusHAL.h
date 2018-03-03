 /**************************************************************************
 *   Copyright (C) 2006-2007 by Danny Kukawka                              *
 *                           <dkukawka@suse.de>, <danny.kukawka@web.de>    *
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
*  \file 	dbusHAL.h
*  \brief 	Headerfile for dbusHAL.cpp and the class \ref dbusHAL.
*/
/*! 
*  \class 	dbusHAL
*  \brief 	class for connection to HAL via D-Bus 
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2006-2007
*/

#ifndef _DBUSHAL_H_
#define _DBUSHAL_H_

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE
#endif

// Global Header
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// QT - Header
#include <qstring.h>

// D-Bus Header
#include <dbus/dbus.h> // needed for dbus_bool_t
#include <dbus/message.h>
#include <dbus/connection.h>

// kpowersave - Header
#include "kpowersave_debug.h"

#include <any>
#include <string>
#include <map>
#include "dbus_properties.hpp"

#define CK_SERVICE		"org.freedesktop.ConsoleKit"
#define CK_MANAGER_IFACE	"org.freedesktop.ConsoleKit.Manager"
#define CK_MANAGER_OBJECT	"/org/freedesktop/ConsoleKit/Manager"
#define CK_SESSION_IFACE	"org.freedesktop.ConsoleKit.Session"

enum msg_type {
	ACPI_EVENT,
	DBUS_EVENT,
	CONSOLEKIT_SESSION_ACTIVE,
	POLICY_POWER_OWNER_CHANGED,
	UPOWER_DEVICE,
	UPOWER_PROPERTY_CHANGED
};

class dbusHAL : public QObject{
	Q_OBJECT

private: 
	static DBusHandlerResult filterFunction (DBusConnection *connection, DBusMessage *message, dbusHAL *);

	//! wrapper to emit a signal with a event from HAL
	void emitMsgReceived( msg_type type, QString message, QString string );

	//! QT connection to D-Bus
	DBusQt::Connection* m_dBusQtConnection;
	//! real connection to D-Bus
	DBusConnection *dbus_connection;
	
	//! to store information if KPowersave is connected to D-Bus
	/*!
	* This boolean represent information about the state of the connection to D-Bus
	* \li true:  if connected
	* \li false: if disconnected
	*/
	bool dbus_is_connected;
	
	//! if we could claim the org.freedesktop.Policy.Power interface
	/*!
	* This boolean represent information if KPowersave could claim the 
	* org.freedesktop.Policy.Power interface from the D-Bus
	* \li true:  if aquired
	* \li false: if not
	*/
	bool aquiredPolicyPower;

	/* D-Bus helper functions */
	//! to initialise the connection to D-Bus 
	bool initDBUS();
	//! to call a methode on a dbus interface with reply
	bool dbusMethodCall( QString interface, QString path, QString object, QString method,
			     DBusBusType dbus_type, void *retvalue, int retval_type,
			     int first_arg_type, va_list var_args);

	//! function to be called back by DBusPendingCall::dbus_pending_call_set_notify()
	static void callBackSuspend (DBusPendingCall* pcall, dbusHAL */*data*/);

	/* typedef std::map<std::string, std::any> Dict; */
	/* Dict call_dbusP(const char *, const char *, const char *, */
	/* 		const char *, int, ...); */
	/* Dict call_dbusP(const char *, const char *, const char *, */
	/* 		const char *, int, va_list); */
	/* Dict get_all(DBusMessage *); */
	/* Dict get_props_array(DBusMessageIter *); */
	/* Dict::value_type get_dict_entry(DBusMessageIter *); */
	/* std::any get_variant(DBusMessageIter *, const char *); */

public:
	
	//! default constructor
	dbusHAL();
	//! default destructor
	~dbusHAL();

	//! to reconnect to D-Bus and HAL
	bool reconnect();
	//! to close the connection to D-Bus and HAL
	bool close();

	//! to aquire the org.freedesktop.Policy.Power interface
	bool aquirePolicyPowerIface();
	//! to release the org.freedesktop.Policy.Power interface
	bool releasePolicyPowerIface();
	//! to check if the org.freedesktop.Policy.Power interface has an owner
	bool isPolicyPowerIfaceOwned();

	// --- helper to get private members of the class --- //
	//! to get information if KPowersave is connected to D-Bus
	bool isConnectedToDBUS() const;
	//! to get info about claim org.freedesktop.Policy.Power interface
	bool aquiredPolicyPowerInterface() const;
	
	//! return the current DBus connection
	DBusConnection *get_DBUS_connection() const;
	
	/* D-Bus helper functions */
	
	/* functions to call methodes */
	//! to call a methode on a dbus system bus method without reply
	bool dbusSystemMethodCall(QString interface, QString path,
				  QString object, QString method,
				  int first_arg_type, ...);
	//! to call a methode on a dbus system bus method with reply
	bool dbusSystemMethodCall(QString interface, QString path,
				  QString object, QString method, 
				  void *retvalue, int retval_type,
				  int first_arg_type, ... );

	//! to call a suspend method on HAL
	bool dbusMethodCallSuspend(const char *suspend);

	/* PolicyKit call helper */
	//! check if the user has a requested privilege
	int isUserPrivileged(QString privilege, QString udi,
			     QString ressource = "", QString user = QString());

	int check_auth(const char *action);
	/* bool isBattery(const char *udi); */

signals:
        //! signal with message to forward from D-Bus to HAL
	void msgReceived_withStringString( msg_type, QString, QString );
	//! signal if we resumed!
	void backFromSuspend( int result );
	//! signal a property change
	void property_changed( msg_type, const char *, const kps::modified_props_type& );
};


#endif
