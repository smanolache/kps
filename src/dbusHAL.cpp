 /**************************************************************************
 *   Copyright (C) 2006-2007 by Danny Kukawka                              *
 *                              <dkukawka@suse.de>, <danny.kukawka@web.de> *
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
 * \file 	dbusHAL.cpp
 * \brief 	In this file can be found the functionality to connect to 
 *		the HAL daemon via D-Bus, to handle D-Bus calls/events and to
 *		provide wrapper to HAL lib and functions
 * \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
 * \date    	2006-2007
 */
 
  // KDE Header
#include <klocale.h>

// DBUS - Header
#include "dbusHAL.h"

// system headers
#include <iostream>

#include "dbus_wrappers.hpp"
#include "UPowerProperties.hpp"
#include <unistd.h>

#include <sstream>

using kps::DBusErr;
using kps::DBusMsg;
using kps::DBusString;
using kps::dict_type;
using kps::devices_type;
using kps::modified_props_type;

static const char UPOWER_DEVICE_PATH[] = "org.freedesktop.UPower.Device";

/*! The default constructor of the class dbusHAL. */
dbusHAL::dbusHAL(){
	kdDebugFuncIn(trace);

	dbus_is_connected = false;
	aquiredPolicyPower = false;

	// init connection to dbus
	if (!initDBUS()) {
		kdError(debug_area) << "Can't connect to D-Bus" << endl;
		m_dBusQtConnection = NULL;
	}

	kdDebugFuncOut(trace);
}

/*! This is the default destructor of class dbusPowersaveConnection. */
dbusHAL::~dbusHAL(){
	kdDebugFuncIn(trace);

	close();

	kdDebugFuncOut(trace);
}

/*! 
 * This function return information about connection status to the DBUS daemon.
 * \return boolean with the state of the connection to D-Bus
 * \retval true if connected
 * \retval false if disconnected
 */
bool
dbusHAL::isConnectedToDBUS() const {
	return dbus_is_connected;
}

/*! 
 * This function return information if the org.freedesktop.Policy.Power 
 * interface was claimed.
 * \return boolean with the status of claim the interface
 * \retval true if aquired 
 * \retval false if not
 */
bool dbusHAL::aquiredPolicyPowerInterface() const {
	return aquiredPolicyPower;
}

/*! 
 * This function try a reconnect to D-Bus and HAL  daemon.
 * \return boolean with the result of the operation
 * \retval true if successful reconnected to D-Bus and HAL
 * \retval false if unsuccessful
 */
bool dbusHAL::reconnect() {
	// close D-Bus connection
	close();
	// init D-Bus conntection and HAL context
	return initDBUS();
}

/*! 
 * This function close the connection to powersave over the D-Bus daemon.
 * \return boolean with the result of the operation
 * \retval true if successful closed the connection
 * \retval false if any problems
 */
bool dbusHAL::close() {
	if (m_dBusQtConnection) {
		releasePolicyPowerIface();
		m_dBusQtConnection->close();
		m_dBusQtConnection = NULL;
	}
	dbus_is_connected = false;

	return true;
}

/* ----> D-Bus section :: START <---- */

/*! 
 * This function initialise the connection to the D-Bus daemon.
 * \return boolean with the result of the operation
 * \retval true if successful initialised D-Bus connection
 * \retval false if unsuccessful
 */
bool dbusHAL::initDBUS() {
	kdDebugFuncIn(trace);

	dbus_is_connected = false;

        DBusError error;
        dbus_error_init(&error);

	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

	if (nullptr == dbus_connection) {
		kdError(debug_area) << "Failed to open connection to system "
			"message bus: " << error.message << endl;
		dbus_error_free(&error);
		return false;
	}

	if (dbus_error_is_set(&error)) {
		kdError(debug_area) << "Failed to register connection with "
			"system message bus: " << error.message << endl;
		return false;
	}

	aquirePolicyPowerIface();

	dbus_connection_set_exit_on_disconnect(dbus_connection, false);

        /* add the filter function which should be executed on events on the
	   bus */
        if (!dbus_connection_add_filter(
		    dbus_connection,
		    reinterpret_cast<DBusHandleMessageFunction>(
			    &dbusHAL::filterFunction), this, NULL)) {
                kdFatal(debug_area) << "Error: Not enough memory to add filter"
			"to dbus connection" << endl;
                exit(EXIT_FAILURE);
        }

        /* add a match rule to catch all signals going through the bus with
	   D-Bus interface */
	dbus_bus_add_match(dbus_connection, "type='signal',"
			    "interface='org.freedesktop.DBus'," 
			    "member='NameOwnerChanged'", NULL);

	/* add a match rule to catch all signals going through the bus with
	   HAL interface */
	dbus_bus_add_match(dbus_connection, "type='signal',"
			   "interface='org.freedesktop.UPower',"
			   "member='DeviceAdded'", NULL);
	dbus_bus_add_match(dbus_connection, "type='signal',"
			   "interface='org.freedesktop.UPower',"
			   "member='DeviceRemoved'", NULL);
	dbus_bus_add_match(dbus_connection, "type='signal',"
			   "interface='org.freedesktop.DBus.Properties',"
			   "member='PropertiesChanged',"
			   "arg0='org.freedesktop.UPower.Device'", NULL);
	/* add a match rule to catch all signals going through the bus with
	   ConsoleKit Interface */
	dbus_bus_add_match(dbus_connection, "type='signal',"
			   "interface='org.freedesktop.ConsoleKit.Session'," 
			   "member='ActiveChanged'", NULL);
	
	m_dBusQtConnection = new DBusQt::Connection(this);
        m_dBusQtConnection->dbus_connection_setup_with_qt_main(dbus_connection);
	
	dbus_is_connected = true;

	kdDebugFuncOut(trace);
	return true;
}

/*! 
 * This function aquire the org.freedesktop.Policy.Power interface
 * \return boolean with the result of the operation
 * \retval true 	if successful aquired the interface
 * \retval false 	if unsuccessful
 */
bool dbusHAL::aquirePolicyPowerIface() {
	kdDebugFuncIn(trace);

	if (dbus_connection == NULL) {
		kdDebugFuncOut(trace);
		return false;
	}

	DBusError error;
        dbus_error_init(&error);
        DBusErr err(&error, &dbus_error_free);

	int rc = dbus_bus_request_name(dbus_connection,
				       "org.freedesktop.Policy.Power",
				       DBUS_NAME_FLAG_REPLACE_EXISTING, &error);
	switch (rc) {
	case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
		kdDebug(debug_area) << "Acquired org.freedesktop.Policy.Power "
			"interface" << endl;
		aquiredPolicyPower = true;
		break;
	case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
		kdWarning(debug_area) << "Queued to aquire org.freedesktop"
			".Policy.Power interface" << endl;
		aquiredPolicyPower = false;
		break;
	default:
		kdWarning(debug_area) << "Error while aquire org.freedesktop"
			".Policy.Power interface: " << rc << ": "
				      << error.message << endl;
		aquiredPolicyPower = false;
		break;
	}

	kdDebugFuncOut(trace);
	return aquiredPolicyPower;
}

/*! 
 * This function release the org.freedesktop.Policy.Power interface
 * \return boolean with the result of the operation
 * \retval true 	if successful aquired the interface
 * \retval false 	if unsuccessful
 */
bool dbusHAL::releasePolicyPowerIface() {
	kdDebugFuncIn(trace);

	if (nullptr == dbus_connection) {
		kdDebugFuncOut(trace);
		return false;
	}

	DBusError error;
        dbus_error_init(&error);

	int result = dbus_bus_release_name(
		dbus_connection, "org.freedesktop.Policy.Power", &error);

	if (dbus_error_is_set(&error)) {
		kdError(debug_area) << "Failed to release org.freedesktop"
			".Policy.Power: " << error.message << endl;
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return false;
	}

	switch (result) {
	case DBUS_RELEASE_NAME_REPLY_RELEASED:
		kdDebug(debug_area) << "Released org.freedesktop.Policy.Power "
			"interface" << endl;
		aquiredPolicyPower = false;
		kdDebugFuncOut(trace);
		return true;
	case DBUS_RELEASE_NAME_REPLY_NOT_OWNER:
		kdWarning(debug_area) << "Couldn't release org.freedesktop."
			"Policy.Power, not the owner" << endl;
		kdDebugFuncOut(trace);
		return false;
	case DBUS_RELEASE_NAME_REPLY_NON_EXISTENT:
		kdWarning(debug_area) << "Couldn't release org.freedesktop."
			"Policy.Power, Iface not existing" << endl;
		kdDebugFuncOut(trace);
		return false;
	default:
		kdWarning(debug_area) << "Couldn't release org.freedesktop."
			"Policy.Power, unknown error" << endl;
		kdDebugFuncOut(trace);
		return false;
	}
}

/*! 
 * This function check if the org.freedesktop.Policy.Power 
 * interface is owned by someone
 * \return boolean with the result of the operation
 * \retval true 	if the interface is owned by someone
 * \retval false 	if else
 */
bool dbusHAL::isPolicyPowerIfaceOwned(){
	kdDebugFuncIn(trace);

	if (nullptr == dbus_connection) {
		kdDebugFuncOut(trace);
		return false;
	}

	DBusError error;
        dbus_error_init(&error);

	bool has_owner = dbus_bus_name_has_owner(
		dbus_connection, "org.freedesktop.Policy.Power", &error);

	if (dbus_error_is_set(&error)) {
		kdError(debug_area) << "Failed to check if org.freedesktop"
			".Policy.Power has an owner: " << error.message << endl;
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return false;
	}

	kdDebugFuncOut(trace);
	return has_owner;
}

/* ----> DBUS section :: END   <---- */

/* ----> D-Bus methode calls functions :: START <---- */
/*! 
 * This function call a D-Bus method
 * \param interface	QString with te dbus interface
 * \param path		QString with the object path
 * \param object	QString with the object name
 * \param method	QString with the name of the methode
 * \param first_arg_type integer with the dbus type of the first argument
 * \param ... 		more arguments
 * \return 		If the query was successful or not
 */
bool
dbusHAL::dbusSystemMethodCall(QString interface, QString path, QString object,
			      QString method, int first_arg_type, ...) {
	kdDebugFuncIn(trace);
	
        va_list var_args;
        va_start(var_args, first_arg_type);
	bool _ret = dbusMethodCall(interface, path, object, method,
				   DBUS_BUS_SYSTEM, NULL, -1, first_arg_type,
				   var_args);
	va_end(var_args);
	
	kdDebugFuncOut(trace);
	return _ret;
}


/*! 
 * This overloaded function call a D-Bus method on the D-Bus system bus with a return value
 * \param interface	 QString with the dbus interface
 * \param path		 QString with the object path
 * \param object	 QString with the object name
 * \param method	 QString with the name of the method
 * \param retvalue	 void pointer to arguments, if NULL we make a simple call
 * \param retval_type    Integer with the dbus type of the return value, set to -1 if retvalue is NULL
 * \param first_arg_type Integer with the dbus type of the first argument followed by the value
 * \return 		 If the query was successful or not
 */
bool
dbusHAL::dbusSystemMethodCall(QString interface, QString path, QString object,
			      QString method, void *retvalue, int retval_type,
			      int first_arg_type, ...) {
	kdDebugFuncIn(trace);
	
        va_list var_args;

        va_start(var_args, first_arg_type);
	bool _ret = dbusMethodCall(interface, path, object, method,
				   DBUS_BUS_SYSTEM, retvalue, retval_type,
				   first_arg_type, var_args);
	va_end(var_args);
	
	kdDebugFuncOut(trace);
	return _ret;
}


/*! 
 * This function call a D-Bus method with a return value
 * \param interface	 QString with the dbus interface
 * \param path		 QString with the object path
 * \param object	 QString with the object name
 * \param method	 QString with the name of the method
 * \param dbus_type	 DBusBusType with the D-Bus BUS Type
 * \param retvalue	 void pointer to arguments, if NULL we make a simple call
 * \param retval_type    Integer with the dbus type of the return value, set to -1 if retvalue is NULL
 * \param first_arg_type Integer with the dbus type of the first argument followed by the value
 * \param var_args	 va_list with more arguments
 * \return 		 If the query was successful or not
 */
bool
dbusHAL::dbusMethodCall(QString interface, QString path, QString object,
			QString method, DBusBusType dbus_type, void *retvalue,
			int retval_type, int first_arg_type,
			va_list var_args ) {
	kdDebugFuncIn(trace);

	DBusError error;
	dbus_error_init(&error); 

	dbus_connection = dbus_bus_get(dbus_type, &error);
	if (dbus_error_is_set(&error)) {
		kdError(debug_area) << "Could not get dbus connection: "
				    << error.message << endl;
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return false;
	}

	DBusMessage *message;
	message = dbus_message_new_method_call(interface, path, object, method);
	dbus_message_append_args_valist(message, first_arg_type, var_args);

	if (nullptr == retvalue) {
		if (!dbus_connection_send(dbus_connection, message, NULL)) {
			kdError(debug_area) << "Could not send method call."
					    << endl;
			dbus_message_unref(message);
			kdDebugFuncOut(trace);
			return false;
		}
		dbus_message_unref(message);
		dbus_connection_flush(dbus_connection);
		kdDebugFuncOut(trace);
		return true;
	}

	DBusMessage *reply;
	reply = dbus_connection_send_with_reply_and_block(
		dbus_connection, message, -1, &error);

	if (dbus_error_is_set(&error)) {
		kdError(debug_area) << "Could not send dbus message: "
				    << error.message << endl;
		dbus_message_unref(message);
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return false;
	}

	int type = dbus_message_get_type(reply);
	if (type != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		kdError(debug_area) << "Revieved invalid DBUS_MESSAGE_TYPE: "
				    << type << ". Expected: "
				    << DBUS_MESSAGE_TYPE_METHOD_RETURN << endl;
		dbus_message_unref(reply);
		dbus_message_unref(message);
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return false;
	}

	if (!dbus_message_get_args(reply, &error, retval_type, retvalue,
				   DBUS_TYPE_INVALID)) {
		if (dbus_error_is_set(&error)) {
			kdError(debug_area) << "Could not get argument from "
				"reply: "  << error.message << endl;
			dbus_error_free(&error);
		}
		dbus_message_unref(reply);
		dbus_message_unref(message);
		kdDebugFuncOut(trace);
		return false;
	}

	dbus_message_unref(message);
	dbus_connection_flush(dbus_connection);
	kdDebugFuncOut(trace);
	return true;
}

/*! 
 * Function to call a suspend and call if resumed \ref callBackSuspend()
 * to emit a resume signal.
 * \param suspend 	a char pointer with the name of the suspend interface
 * \return 		If the query was successful or not
 */
bool
dbusHAL::dbusMethodCallSuspend(const char *suspend) {
	kdDebugFuncIn(trace);
	DBusError error;
	dbus_error_init(&error);

	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	
	if (dbus_error_is_set(&error)) {
		kdError(debug_area) << "Could not get dbus connection: "
				    << error.message << endl;
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return false;
	}

	kdDebug(debug_area) << "Calling org.freedesktop.login1.Manager"
			    << suspend << endl;
	DBusMessage *message =
		dbus_message_new_method_call(
			"org.freedesktop.login1", "/org/freedesktop/login1",
			"org.freedesktop.login1.Manager", suspend);
	if (nullptr == message) {
		kdError(debug_area) << "Error building org.freedesktop.login1."
			"Manager." << suspend << " call." << endl;
		kdDebugFuncOut(trace);
		return false;
	}
	dbus_bool_t interactive = false;
	if (!dbus_message_append_args(message, DBUS_TYPE_BOOLEAN, &interactive,
				      DBUS_TYPE_INVALID)) {
		kdError(debug_area) << "Error appending 'false' to org"
			".freedesktop.login1.Manager." << suspend << "."
				    << endl;
		dbus_message_unref(message);
		kdDebugFuncOut(trace);
		return false;
	}
	// need to set INT_MAX as default and not -1
	DBusPendingCall *pcall = nullptr;
	dbus_connection_send_with_reply(
		dbus_connection, message, &pcall, INT_MAX);
	if (pcall) {
		kdDebug(debug_area) << "org.freedesktop.login1.Manager."
				    << suspend << ": pcall is not null."
				    << endl;
		dbus_pending_call_ref(pcall); // really needed?
		dbus_pending_call_set_notify(
			pcall,
			reinterpret_cast<DBusPendingCallNotifyFunction>(
				&dbusHAL::callBackSuspend),
			this, NULL);
	} else {
		kdDebug(debug_area) << "org.freedesktop.login1.Manager."
				    << suspend << ": pcall is null." << endl;
	}
	dbus_message_unref(message);

	kdDebugFuncOut(trace);
	return true;
}

/*!
 * Slot called by D-Bus as set in \ref dbusMethodCallSuspend() 
 * Here we emit the resume signal.
 */
void
dbusHAL::callBackSuspend(DBusPendingCall* pcall, dbusHAL *instance) {
	kdDebugFuncIn(trace);

        if (nullptr == pcall) {
		kdError(debug_area) << "dbusHAL::callBackSuspend - "
			"DBusPendingCall not set, return" << endl;
		kdDebugFuncOut(trace);
		return;
        }

	DBusMessage *reply = dbus_pending_call_steal_reply(pcall);
	if (reply == NULL) {
		kdError(debug_area) << "dbusHAL::callBackSuspend - Got no "
			"reply, return" << endl;
	} else
		dbus_message_unref(reply);

	dbus_pending_call_unref(pcall);
	emit instance->backFromSuspend(0);
	kdDebugFuncOut(trace);
}


/* ----> D-Bus methode calls functions :: END  <---- */
/* ---> PolicyKit method call section :: START <--- */

/*!
 * Check if the user is privileged to a special privilege
 * \param privilege	QString with the name of the requested privilege
 * \param udi		QString with the UDI.
 * \param ressource	QString with the name of the ressource
 * \param user		QString with the name of the user. If empty the current user is used.
 * \return int with info if the user is allowed or not.
 * \retval 0		if not allowed
 * \retval 1		if allowed
 * \retval -1		if a error occurs or we could not query the interface
 */
int
dbusHAL::isUserPrivileged(QString privilege, QString udi, QString ressource,
			  QString user) {
	kdDebugFuncIn(trace);

	const char *_user = user.isEmpty() || user.isNull() ?
		getenv("USER") : user.latin1();
	
	if (nullptr == _user || privilege.isEmpty()) {
		kdDebugFuncOut(trace);
		return -1;
	}

	const char *_unique_name = dbus_bus_get_unique_name(dbus_connection);
	const char *_privilege = privilege.latin1();

	// not sure if we need this, but to avoid problems
	const char *_resource = ressource.latin1();

	dbus_bool_t _retval;
	if (!dbusSystemMethodCall("org.freedesktop.PolicyKit",
				  "/org/freedesktop/PolicyKit/Manager",
				  "org.freedesktop.PolicyKit.Manager",
				  "IsUserPrivileged",
				   &_retval, DBUS_TYPE_BOOLEAN,
				   DBUS_TYPE_STRING, &_unique_name,
				   DBUS_TYPE_STRING, &_user,
				   DBUS_TYPE_STRING, &_privilege,
				   DBUS_TYPE_STRING, &_resource,
				   DBUS_TYPE_INVALID)) {
		kdDebugFuncOut(trace);
		return -1;
	}

	kdDebugFuncOut(trace);
	return static_cast<int>(_retval);
}

/* ---> PolicyKit method call section :: END   <--- */

/*!
 * Use this SLOT to emit a reviced messages to the kpowersave.
 * NOTE: Because of the filter function this need to be a public function.
 *       Don't use this function in any other place than this class.
 * \param  type enum with the type of the message
 * \param  message String with the message
 * \param  string String with additional info
 */
void
dbusHAL::emitMsgReceived(msg_type type, QString message, QString string) {
	if (message.startsWith("dbus.terminate"))
		dbus_is_connected = false;

	if (type == POLICY_POWER_OWNER_CHANGED) {
		if (message.startsWith("NOW_OWNER"))
			aquiredPolicyPower = true;
		else 
			aquiredPolicyPower = false;
	}

	emit msgReceived_withStringString(type, message, string);
}

#include "dbusHAL.moc"
// --> functions which are not member of the class ...

/*! 
 * This function is needed filter function for the D-Bus connection to filter
 * all needed messages from the bus which are needful for KPowersave. 
 * \param connection	existing connection to the D-Bus daemon
 * \param message 	the recieved message from the D-Bus daemon
 * \param data		void pointer (see dbus bindings for more information)
 * \return DBusHandlerResult
 */
DBusHandlerResult 
dbusHAL::filterFunction(DBusConnection *connection, DBusMessage *message,
			dbusHAL *instance) {
 	kdDebugFuncIn(trace);
	
	if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL,
				   "Disconnected")) {
		instance->emitMsgReceived(DBUS_EVENT, "dbus.terminate", 0);
		dbus_connection_unref(connection);
		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

        if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL) {
                if (trace)
			kdDebug(debug_area) << "recieved message, but wasn't "
				"from type DBUS_MESSAGE_TYPE_SIGNAL" << endl;
		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

	QString ifaceType = dbus_message_get_interface(message);
        if (nullptr == ifaceType) {
                kdDebug(debug_area) << "Received message from invalid "
			"interface" << endl;
                kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

	if (0 == strcmp("org.freedesktop.DBus.Properties", ifaceType)) {
		const char *signal = dbus_message_get_member(message);
		if (0 == strcmp(signal, "PropertiesChanged")) {
			const char *udi = dbus_message_get_path(message);
			modified_props_type d;
			message >> d;
			emit instance->property_changed(UPOWER_PROPERTY_CHANGED,
							udi, d);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (ifaceType.startsWith(DBUS_INTERFACE_DBUS)) {
		if (trace)
			kdDebug(debug_area) << "Received from "
				"DBUS_INTERFACE_DBUS" << endl;
		/* get the name of the signal */
		const char *signal = dbus_message_get_member(message);
		
		char *value;
		DBusError error;
		dbus_error_init(&error);
		/* get the first argument. This must be a string at the moment */
		dbus_message_get_args(message, &error, DBUS_TYPE_STRING,
				      &value, DBUS_TYPE_INVALID);
	
		if (dbus_error_is_set(&error)) {
			kdWarning(debug_area) << "Received signal "
					      << error.message << " but no "
				"string argument" << endl;
			dbus_error_free(&error);
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
	
		if (trace)
			kdDebug(debug_area) << "filter_function::SIGNAL="
					    << signal << " VALUE=" << value
					    << endl;
	
		/* our name is... */
		if (0 == strcmp(signal, "NameAcquired")) {
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
		
		if (0 == strcmp(signal, "NameOwnerChanged")) {
			char *service;
			char *old_owner;
			char *new_owner;
                	if (!dbus_message_get_args(
				    message, NULL, DBUS_TYPE_STRING, &service,
				    DBUS_TYPE_STRING, &old_owner,
				    DBUS_TYPE_STRING, &new_owner,
				    DBUS_TYPE_INVALID)) {
				kdDebugFuncOut(trace);
				return DBUS_HANDLER_RESULT_HANDLED;
			}
			if (0 != strcmp(service,
					"org.freedesktop.Policy.Power")) {
				kdDebugFuncOut(trace);
				return DBUS_HANDLER_RESULT_HANDLED;
			}

			const char *own_name = dbus_bus_get_unique_name(
				instance->get_DBUS_connection());
			if (0 == strcmp(new_owner, own_name)) {
				kdDebug(debug_area) << "=== now owner of "
					"org.freedesktop.Policy.Power ==="
						    << endl;
				// we have now again the ower of the name!
				instance->emitMsgReceived(
					POLICY_POWER_OWNER_CHANGED, "NOW_OWNER",
					NULL);
			} else {
				// some other has now the interface
				kdDebug(debug_area) << "=== someone else owner"
					" of org.freedesktop.Policy.Power ==="
						    << endl;
				instance->emitMsgReceived(
					POLICY_POWER_OWNER_CHANGED,
					"OTHER_OWNER", NULL);
			}
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (0 == strcmp(ifaceType, "org.freedesktop.UPower")) {
		const char *signal = dbus_message_get_member(message);
		kdDebug(debug_area) << "Received from org.freedesktop.UPower: "
				    << (nullptr == signal ? "null" : signal)
				    << endl;
		if (0 != strcmp(signal, "DeviceRemoved") &&
		    0 != strcmp(signal, "DeviceAdded")) {
			kdDebug(debug_area) << "Neither DeviceAdded nor "
				"DeviceRemoved." << endl;
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
		DBusError error;
		dbus_error_init(&error);
		char *udi;
		if (!dbus_message_get_args(
			    message, &error, DBUS_TYPE_OBJECT_PATH, &udi,
			    DBUS_TYPE_INVALID)) {
			kdError(debug_area) << "No object path argument for "
					    << signal << ": " <<  error.message
					    << endl;
			dbus_error_free(&error);
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		instance->emitMsgReceived(UPOWER_DEVICE, signal, udi);

		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (ifaceType.startsWith("org.freedesktop.ConsoleKit.Session")) {
		kdDebug(debug_area) << "Received from org.freedesktop"
			".ConsoleKit.Session" << endl;

		const char *session = dbus_message_get_path(message);
		const char *signal = dbus_message_get_member(message);
		
		if (0 != strcmp(signal, "ActiveChanged")) {
			kdDebug(debug_area) << "Received unknown signal from "
				"org.freedesktop.ConsoleKit.Session: "
					    << signal << endl;
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		DBusError error;
		dbus_error_init(&error);
		dbus_bool_t active;
		if (!dbus_message_get_args(message, &error, DBUS_TYPE_BOOLEAN,
					  &active, DBUS_TYPE_INVALID)) {
			kdError(debug_area) << "Could not get args for signal "
				"ActiveChanged of org.freedesktop.ConsoleKit"
				".Session: " << error.message << endl;
			if (dbus_error_is_set(&error))
				dbus_error_free(&error);
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
		if (dbus_error_is_set(&error)) {
			kdError(debug_area) << "Could not get args for signal "
				"ActiveChanged of org.freedesktop.ConsoleKit"
				".Session: " << error.message << endl;
			dbus_error_free(&error);
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		instance->emitMsgReceived(CONSOLEKIT_SESSION_ACTIVE, 
					  session, QString("%1")
					  .arg(static_cast<int>(active)));

		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	kdDebugFuncOut(trace);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

// --> some functions to get private members

//! to get the current connection to D-Bus
DBusConnection *
dbusHAL::get_DBUS_connection() const {
	return dbus_connection;
}

int
dbusHAL::check_auth(const char *action) {
	kdDebugFuncIn(trace);

	DBusMessage *msg = dbus_message_new_method_call(
		"org.freedesktop.PolicyKit1",
		"/org/freedesktop/PolicyKit1/Authority",
		"org.freedesktop.PolicyKit1.Authority", "CheckAuthorization");
	if (nullptr == msg) {
		kdError(debug_area) << "Cannot build the org.freedesktop"
			".PolicyKit1.Authority.CheckAuthorization call."
				    << endl;
		kdDebugFuncIn(trace);
		return -1;
	}

	DBusMessageIter i;
	dbus_message_iter_init_append(msg, &i);

	// append subject
	DBusMessageIter j;
	if (!dbus_message_iter_open_container(
		    &i, DBUS_TYPE_STRUCT, nullptr, &j)) {
		kdError(debug_area) << "dbus_message_iter_open_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	const char *kind = "unix-process";
	if (!dbus_message_iter_append_basic(&j, DBUS_TYPE_STRING, &kind)) {
		kdError(debug_area) << "dbus_message_iter_append_basic returns"
			"false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// open dict
	DBusMessageIter k;
	if (!dbus_message_iter_open_container(
		    &j, DBUS_TYPE_ARRAY, "{sv}", &k)) {
		kdError(debug_area) << "dbus_message_iter_open_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// open dict-entry
	DBusMessageIter l;
	if (!dbus_message_iter_open_container(
		    &k, DBUS_TYPE_DICT_ENTRY, nullptr, &l)) {
		kdError(debug_area) << "dbus_message_iter_open_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// static const char *session_id_key = "session-id";
	static const char *pid_key = "pid";
	if (!dbus_message_iter_append_basic(&l, DBUS_TYPE_STRING,
					    &pid_key)) {
		kdError(debug_area) << "dbus_message_iter_append_basic returns"
			" false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	DBusMessageIter m;
	if (!dbus_message_iter_open_container(
		    &l, DBUS_TYPE_VARIANT, "u", &m)) {
		kdError(debug_area) << "dbus_message_iter_open_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	uint32_t pid = getpid();
	if (!dbus_message_iter_append_basic(&m, DBUS_TYPE_UINT32,
					    &pid)) {
		kdError(debug_area) << "dbus_message_iter_append_basic returns"
			" false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// close the variant
	if (!dbus_message_iter_close_container(&l, &m)) {
		kdError(debug_area) << "dbus_message_iter_close_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// close the dict entry
	if (!dbus_message_iter_close_container(&k, &l)) {
		kdError(debug_area) << "dbus_message_iter_close_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// open the dict entry
	if (!dbus_message_iter_open_container(
		    &k, DBUS_TYPE_DICT_ENTRY, nullptr, &l)) {
		kdError(debug_area) << "dbus_message_iter_open_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	static const char *start_time_key = "start-time";
	if (!dbus_message_iter_append_basic(&l, DBUS_TYPE_STRING,
					    &start_time_key)) {
		kdError(debug_area) << "dbus_message_iter_append_basic returns"
			" false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// open the variant
	if (!dbus_message_iter_open_container(
		    &l, DBUS_TYPE_VARIANT, "t", &m)) {
		kdError(debug_area) << "dbus_message_iter_open_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	uint64_t ts = 0;
	if (!dbus_message_iter_append_basic(&m, DBUS_TYPE_UINT64, &ts)) {
		kdError(debug_area) << "dbus_message_iter_append_basic returns"
			" false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// close the variant
	if (!dbus_message_iter_close_container(&l, &m)) {
		kdError(debug_area) << "dbus_message_iter_close_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// close the dict entry
	if (!dbus_message_iter_close_container(&k, &l)) {
		kdError(debug_area) << "dbus_message_iter_close_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// close the dictionary
	if (!dbus_message_iter_close_container(&j, &k)) {
		kdError(debug_area) << "dbus_message_iter_close_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// close the subject struct
	if (!dbus_message_iter_close_container(&i, &j)) {
		kdError(debug_area) << "dbus_message_iter_close_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}

	// append action
	if (!dbus_message_iter_append_basic(&i, DBUS_TYPE_STRING, &action)) {
		kdError(debug_area) << "dbus_message_iter_append_basic returns"
			" false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}

	// append details dictionary
	if (!dbus_message_iter_open_container(
		    &i, DBUS_TYPE_ARRAY, "{ss}", &j)) {
		kdError(debug_area) << "dbus_message_iter_open_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	if (!dbus_message_iter_close_container(&i, &j)) {
		kdError(debug_area) << "dbus_message_iter_close_container "
			"returns false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}

	// append flags
	uint32_t flags = 0;
	if (!dbus_message_iter_append_basic(&i, DBUS_TYPE_UINT32, &flags)) {
		kdError(debug_area) << "dbus_message_iter_append_basic returns"
			" false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}
	// append cancellation_id
	static const char *cancellation_id = "";
	if (!dbus_message_iter_append_basic(&i, DBUS_TYPE_STRING,
					    &cancellation_id)) {
		kdError(debug_area) << "dbus_message_iter_append_basic returns"
			" false." << endl;
		dbus_message_unref(msg);
		kdDebugFuncIn(trace);
		return -1;
	}

	DBusError error;
	dbus_error_init(&error);
	DBusMessage *reply = dbus_connection_send_with_reply_and_block(
		dbus_connection, msg, -1, &error);
	if (dbus_error_is_set(&error)) {
		kdError(debug_area) << "Could not send dbus message: "
				    << error.message << endl;
		dbus_message_unref(msg);
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return -1;
	}

	int type = dbus_message_get_type(reply);
	if (type != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		kdError(debug_area) << "Received invalid DBUS_MESSAGE_TYPE: "
				    << type << ". Expected: "
				    << DBUS_MESSAGE_TYPE_METHOD_RETURN << endl;
		dbus_message_unref(reply);
		dbus_message_unref(msg);
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return -1;
	}

	if (!dbus_message_iter_init(reply, &i)) {
		kdError(debug_area) << "dbus_message_iter_init returns false."
				    << endl;
		dbus_message_unref(reply);
		dbus_message_unref(msg);
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return -1;
	}

	type = dbus_message_iter_get_arg_type(&i);
	if (DBUS_TYPE_STRUCT != type) {
		kdError(debug_area) << "org.freedesktop.PolicyKit1.Authority."
			"CheckAuthorization: expected a struct." << endl;
		dbus_message_unref(reply);
		dbus_message_unref(msg);
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return -1;
	}

	dbus_message_iter_recurse(&i, &j);

	type = dbus_message_iter_get_arg_type(&j);
	if (DBUS_TYPE_BOOLEAN != type) {
		kdError(debug_area) << "org.freedesktop.PolicyKit1.Authority."
			"CheckAuthorization: expected a boolean." << endl;
		dbus_message_unref(reply);
		dbus_message_unref(msg);
		dbus_error_free(&error);
		kdDebugFuncOut(trace);
		return -1;
	}

	dbus_bool_t is_authorised = false;
	dbus_message_iter_get_basic(&j, &is_authorised);

	dbus_message_unref(reply);
	dbus_message_unref(msg);
	dbus_error_free(&error);

	if (trace)
		kdDebug(debug_area) << action << " is "
				    << (is_authorised ? "authorised" :
					"not authorised")
				    << endl;

	kdDebugFuncOut(trace);

	return is_authorised ? 1 : 0;
}
