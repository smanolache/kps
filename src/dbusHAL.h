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

// HAL Library
#include <hal/libhal.h>

// kpowersave - Header
#include "kpowersave_debug.h"

#define HAL_SERVICE		"org.freedesktop.Hal"
#define HAL_PM_IFACE		"org.freedesktop.Hal.Device.SystemPowerManagement"
#define HAL_LPANEL_IFACE	"org.freedesktop.Hal.Device.LaptopPanel"
#define HAL_CPUFREQ_IFACE	"org.freedesktop.Hal.Device.CPUFreq"
#define HAL_COMPUTER_UDI	"/org/freedesktop/Hal/devices/computer"
#define CK_SERVICE		"org.freedesktop.ConsoleKit"
#define CK_MANAGER_IFACE	"org.freedesktop.ConsoleKit.Manager"
#define CK_MANAGER_OBJECT	"/org/freedesktop/ConsoleKit/Manager"
#define CK_SESSION_IFACE	"org.freedesktop.ConsoleKit.Session"

enum msg_type {
	ACPI_EVENT,
	DBUS_EVENT,
	HAL_DEVICE,
	HAL_PROPERTY_CHANGED,
	HAL_CONDITION,
	CONSOLEKIT_SESSION_ACTIVE,
	POLICY_POWER_OWNER_CHANGED
};

class dbusHAL : public QObject{
	Q_OBJECT

private: 

	//! QT connection to D-Bus
	DBusQt::Connection* m_dBusQtConnection;
	//! real connection to D-Bus
	DBusConnection *dbus_connection;
	//! HAL context
	LibHalContext *hal_ctx;
	
	//! to store information if KPowersave is connected to D-Bus
	/*!
	* This boolean represent information about the state of the connection to D-Bus
	* \li true:  if connected
	* \li false: if disconnected
	*/
	bool dbus_is_connected;
	//! to store information if KPowersave is connected to HAL
	/*!
	* This boolean represent information about the state of the connection to HAL
	* \li true:  if connected
	* \li false: if disconnected
	*/
	bool hal_is_connected;
	
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
	static void callBackSuspend (DBusPendingCall* pcall, void* /*data*/);

	/* HAL helper functions */
	//! to initialise the connection to HAL 
	bool initHAL();
	//! to free the HAL context 
	void freeHAL();

public:
	
	//! default constructor
	dbusHAL();
	//! default destructor
	~dbusHAL();

	//! to reconnect to D-Bus and HAL
	bool reconnect();
	//! to reconnect only HAL
	bool reconnectHAL();
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
	bool isConnectedToDBUS();
	//! to get information if KPowersave is connected to HAL
	bool isConnectedToHAL();
	//! to get info about claim org.freedesktop.Policy.Power interface
	bool aquiredPolicyPowerInterface();
	
	//! return the current HAL context
	LibHalContext *get_HAL_context();
	//! return the current DBus connection
	DBusConnection *get_DBUS_connection();

	/* HAL device information stuff */
	//! Query a integer device property from HAL
	bool halGetPropertyInt(QString udi, QString property, int *returnval);
	//! Query a bolean device property from HAL
	bool halGetPropertyBool(QString udi, QString property, bool *returnval);
	//! Query a string device property from HAL
	bool halGetPropertyString(QString udi, QString property, QString *returnval);
	//! Query a string list device property from HAL
	bool halGetPropertyStringList (QString udi, QString property, QStringList *devices); 
	//! Query a capability for a HAL device 
	bool halQueryCapability(QString udi, QString capability, bool *returnval);

	/* functions to find devices and check stuff */
	//! check if a property exist on a device
	bool halDevicePropertyExist(QString udi, QString property);
	//! to find a device by capability
	bool halFindDeviceByCapability (QString capability, QStringList *devices);
	//! to find a device by a string property
	bool halFindDeviceByString (QString property, QString keyval, QStringList *devices);
	
	/* D-Bus helper functions */
	
	/* functions to call methodes */
	//! to call a methode on a dbus system bus method without reply
	bool dbusSystemMethodCall( QString interface, QString path, QString object, QString method,
				   int first_arg_type, ... );
	//! to call a methode on a dbus system bus method with reply
	bool dbusSystemMethodCall( QString interface, QString path, QString object, QString method, 
				   void *retvalue, int retval_type, int first_arg_type, ... );

	//! to call a suspend method on HAL
	bool dbusMethodCallSuspend ( const char *suspend );

	/* PolicyKit call helper */
	//! check if the user has a requested privilege
	int isUserPrivileged( QString privilege, QString udi, QString ressource = "", QString user = QString());

	//! wrapper to emit a signal with a event from HAL
	void emitMsgReceived( msg_type type, QString message, QString string );
	
signals:
        //! signal with message to forward from D-Bus to HAL
	void msgReceived_withStringString( msg_type, QString, QString );
	//! signal if we resumed!
	void backFromSuspend( int result );
};

//! filter function to filter out needed information from D-Bus messages
DBusHandlerResult filterFunction (DBusConnection *connection, DBusMessage *message, void *data);

#endif
