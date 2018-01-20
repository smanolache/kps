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

static void* myInstance = 0;

/*! The default constructor of the class dbusHAL. */
dbusHAL::dbusHAL(){
	kdDebugFuncIn(trace);

	dbus_is_connected = false;
	hal_is_connected = false;
	aquiredPolicyPower = false;
	hal_ctx = NULL;

	// add pointer to this for filter_function()
	myInstance=this;
	// init connection to dbus
	if(!initDBUS()) {
		kdError() << "Can't connect to D-Bus" << endl;
		m_dBusQtConnection = NULL;
	}
	if(!initHAL()) 
		kdError() << "Can't connect to HAL" << endl;

	kdDebugFuncOut(trace);
}

/*! This is the default destructor of class dbusPowersaveConnection. */
dbusHAL::~dbusHAL(){
	kdDebugFuncIn(trace);

	close();
	myInstance = NULL;

	kdDebugFuncOut(trace);
}

/*! 
 * This function return information about connection status to the DBUS daemon.
 * \return boolean with the state of the connection to D-Bus
 * \retval true if connected
 * \retval false if disconnected
 */
bool dbusHAL::isConnectedToDBUS() {
	return dbus_is_connected;
}

/*! 
 * This function return information about connection status to the HAL daemon.
 * \return boolean with the state of the connection to HAL
 * \retval true if connected
 * \retval false if disconnected
 */
bool dbusHAL::isConnectedToHAL() {
	return hal_is_connected;
}

/*! 
 * This function return information if the org.freedesktop.Policy.Power 
 * interface was claimed.
 * \return boolean with the status of claim the interface
 * \retval true if aquired 
 * \retval false if not
 */
bool dbusHAL::aquiredPolicyPowerInterface() {
	return aquiredPolicyPower;
}

/*! 
 * This function try a reconnect to D-Bus and HAL  daemon.
 * \return boolean with the result of the operation
 * \retval true if successful reconnected to D-Bus and HAL
 * \retval false if unsuccessful
 */
bool dbusHAL::reconnect() {
	// free HAL context
	freeHAL();
	// close D-Bus connection
	close();
	// init D-Bus conntection and HAL context
	return (initDBUS() && initHAL());
}

/*! 
 * This function close the connection to powersave over the D-Bus daemon.
 * \return boolean with the result of the operation
 * \retval true if successful closed the connection
 * \retval false if any problems
 */
bool dbusHAL::close() {
	if ( m_dBusQtConnection != NULL ) {
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
bool dbusHAL::initDBUS(){
	kdDebugFuncIn(trace);

	dbus_is_connected = false;

        DBusError error;
        dbus_error_init(&error);
	
	dbus_connection = dbus_bus_get( DBUS_BUS_SYSTEM, &error );

	if (dbus_connection == NULL){
		kdError() << "Failed to open connection to system message bus: " << error.message << endl;
		dbus_error_free (&error);
		return false;
	}

	if ( dbus_error_is_set( &error ) ) {
		kdError() << "Failed to register connection with system message bus: " << error.message << endl;
		return false;
	}

	aquirePolicyPowerIface();

	dbus_connection_set_exit_on_disconnect( dbus_connection, false );

        /* add the filter function which should be executed on events on the bus */
        if ( ! dbus_connection_add_filter( dbus_connection, filterFunction, this, NULL) ) {
                kdFatal() << "Error: Not enough memory to add filter to dbus connection" << endl;
                exit(EXIT_FAILURE);
        }

        /* add a match rule to catch all signals going through the bus with D-Bus interface */
	dbus_bus_add_match( dbus_connection, "type='signal',"
			    "interface='org.freedesktop.DBus'," 
			    "member='NameOwnerChanged'", NULL);

	/* add a match rule to catch all signals going through the bus with HAL interface */
	dbus_bus_add_match( dbus_connection, "type='signal',"
			    "interface='org.freedesktop.Hal.Manager'," 
			    "member='DeviceAdded'", NULL);
	dbus_bus_add_match( dbus_connection, "type='signal',"
			    "interface='org.freedesktop.Hal.Manager'," 
			    "member='DeviceRemoved'", NULL);
	dbus_bus_add_match( dbus_connection, "type='signal',"
			    "interface='org.freedesktop.Hal.Device'," 
			    "member='PropertyModified'", NULL);
	dbus_bus_add_match( dbus_connection, "type='signal',"
			    "interface='org.freedesktop.Hal.Device'," 
			    "member='Condition'", NULL);

	/* add a match rule to catch all signals going through the bus with ConsoleKit Interface */
	dbus_bus_add_match( dbus_connection, "type='signal',"
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
bool dbusHAL::aquirePolicyPowerIface(){
	kdDebugFuncIn(trace);

	if (dbus_connection == NULL) {
		kdDebugFuncOut(trace);
		return false;
	}

	switch (dbus_bus_request_name(dbus_connection, "org.freedesktop.Policy.Power",
				      DBUS_NAME_FLAG_REPLACE_EXISTING, NULL)) {
		case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
			kdDebug() << "Acquired org.freedesktop.Policy.Power interface" << endl;
			aquiredPolicyPower = true;
			break;
		case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
			kdWarning() << "Queued to aquire org.freedesktop.Policy.Power interface" << endl;
			aquiredPolicyPower = false;
			break;
		default:
			kdWarning() << "Unknown error while aquire org.freedesktop.Policy.Power interface" << endl;
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
bool dbusHAL::releasePolicyPowerIface(){
	kdDebugFuncIn(trace);

	int result;
	bool retval = false;
	DBusError error;

	if (dbus_connection == NULL) {
		kdDebugFuncOut(trace);
		return false;
	}

        dbus_error_init(&error);

	result = dbus_bus_release_name(dbus_connection, "org.freedesktop.Policy.Power", &error);

	if ( dbus_error_is_set( &error ) ) {
		kdError() << "Failed to release org.freedesktop.Policy.Power: " << error.message << endl;
		dbus_error_free(&error);
	} else {
		switch (result) {
			case DBUS_RELEASE_NAME_REPLY_RELEASED:
				kdDebug() << "Released org.freedesktop.Policy.Power interface" << endl;
				retval = true;
				aquiredPolicyPower = false;
				break;
			case DBUS_RELEASE_NAME_REPLY_NOT_OWNER:
				kdWarning() << "Couldn't release org.freedesktop.Policy.Power, not the owner" << endl;
				break;
			case DBUS_RELEASE_NAME_REPLY_NON_EXISTENT:
				kdWarning() << "Couldn't release org.freedesktop.Policy.Power, Iface not existing" << endl;
				break;
			default:
				kdWarning() << "Couldn't release org.freedesktop.Policy.Power, unknown error" << endl;
				break;
		}
	}
		
	return retval;
	kdDebugFuncOut(trace);
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

	bool retval = false;
	DBusError error;

	if (dbus_connection == NULL) {
		kdDebugFuncOut(trace);
		return false;
	}

        dbus_error_init(&error);

	retval = dbus_bus_name_has_owner(dbus_connection, "org.freedesktop.Policy.Power", &error);

	if ( dbus_error_is_set( &error ) ) {
		kdError() << "Failed to check if org.freedesktop.Policy.Power has an owner: " << error.message << endl;
		dbus_error_free(&error);
	}

	kdDebugFuncOut(trace);
	return retval;
}

/* ----> DBUS section :: END   <---- */
/* ----> HAL  section :: START <---- */

/*! 
 * This function initialise the connection to HAL over the D-Bus daemon.
 * \return boolean with the result of the operation
 * \retval true if successful initialised HAL connection and context
 * \retval false if unsuccessful
 */
bool dbusHAL::initHAL(){
	kdDebugFuncIn(trace);

	if ( !dbus_is_connected ) {
		freeHAL();
		return false;
	} else if ( hal_is_connected && (hal_ctx != NULL)) { 
		return true;
	}
	
	// could not connect to HAL, reset all and try again
	freeHAL();

	DBusError error;
	dbus_error_init(&error);

	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if (dbus_connection == NULL || dbus_error_is_set(&error)) {
		kdError() << "could not open connection to system bus: " << error.message << endl;
		dbus_error_free(&error);
		return false;
	}

	bool hal_is_ready = dbus_bus_name_has_owner(dbus_connection, "org.freedesktop.Hal", &error);

	if (!hal_is_ready) {
		kdWarning() << "HAL is not ready. We will try later... " << endl;

		if ( dbus_error_is_set( &error ) ) {
			kdError() << "Error checking if hal service exists: " << error.message << endl;
			dbus_error_free( &error );
		}

		freeHAL();
		return false;
	}

	if((hal_ctx = libhal_ctx_new()) == NULL) {
		kdError() << "Could not init HAL context" << endl;
		return false;
	}

	/* setup dbus connection for hal */
	if (!libhal_ctx_set_dbus_connection(hal_ctx, dbus_connection)) {
		kdError() << "Could not set up connection to dbus for hal" << endl;
		freeHAL();
		return false;
	}

	/* init the hal library */
	if (!libhal_ctx_init(hal_ctx, &error)) {
		kdError() << "Could not init hal library: " << error.message << endl;
		freeHAL();
		return false;
	}

	hal_is_connected = true;

	kdDebugFuncOut(trace);
	return hal_is_connected;
}

/*! 
 * This function free the hal connection/context.
 */
void dbusHAL::freeHAL(){

	if ( hal_ctx != NULL ) {
		libhal_ctx_free( hal_ctx );
		hal_ctx = NULL;
	}
	hal_is_connected = false;
}

/*! 
 * This function try a reconnect to the HAL daemon only.
 * \return boolean with the result of the operation
 * \retval true if successful reconnected to HAL
 * \retval false if unsuccessful
 */
bool dbusHAL::reconnectHAL() {
	// free HAL context
	freeHAL();
	// init HAL context
	return (initHAL());
}

/*! 
 * This function query a integer property from HAL for a given device
 * \param udi		QString with the UDI of the device
 * \param property 	QString with the property
 * \param returnval 	pointer to the return value
 * \return 		If the query was successful or not
 */
bool dbusHAL::halGetPropertyInt(QString udi, QString property, int *returnval){
	kdDebugFuncIn(trace);
	
	bool ret = false;

	if (!initHAL() || udi.isEmpty() || property.isEmpty())
		goto out;

	DBusError error;
	dbus_error_init(&error);

	if (!libhal_device_property_exists(hal_ctx, udi, property, &error)) {
		kdWarning() << "Property: " << property << " for: " << udi << " doesn't exist." << endl;
		goto out;
	}

	*returnval = libhal_device_get_property_int(hal_ctx, udi, property, &error);

	if (dbus_error_is_set(&error)) {
		kdError() << "Fetching property: " << property << " for: " << udi
			  << " failed with: " << error.message << endl;
		dbus_error_free(&error);
		goto out;
	} else {
		ret = true;
	}

out:
	kdDebugFuncOut(trace);
	return true;
}

/*! 
 * This function query a boolean property from HAL for a given device
 * \param udi		QString with the UDI of the device
 * \param property 	QString with the property
 * \param returnval 	pointer to the return value
 * \return 		If the query was successful or not
 */
bool dbusHAL::halGetPropertyBool(QString udi, QString property, bool *returnval){
	kdDebugFuncIn(trace);

	bool ret = false;

	if (!initHAL() || udi.isEmpty() || property.isEmpty()) 
		goto out;

	DBusError error;
	dbus_error_init(&error);

	if (!libhal_device_property_exists(hal_ctx, udi, property, &error)) {
		kdWarning() << "Property: " << property << " for: " << udi << " doesn't exist." << endl;
		goto out;
	}

	*returnval = libhal_device_get_property_bool(hal_ctx, udi, property, &error);

	if (dbus_error_is_set(&error)) {
		kdError() << "Fetching property: " << property << " for: " << udi 
			  << " failed with: " << error.message << endl;
		dbus_error_free(&error);
		goto out;
	} else {
		ret = true;
	}

out:
	kdDebugFuncOut(trace);
	return ret;
}


/*! 
 * This function query a Sting property from HAL for a given device
 * \param udi		QString with the UDI of the device
 * \param property 	QString with the property
 * \param returnval 	pointer to the return value
 * \return 		If the query was successful or not
 */
bool dbusHAL::halGetPropertyString(QString udi, QString property, QString *returnval){
	kdDebugFuncIn(trace);

	bool ret = false;

	if (!initHAL() || udi.isEmpty() || property.isEmpty()) 
		goto out;

	DBusError error;
	dbus_error_init(&error);

	if (!libhal_device_property_exists(hal_ctx, udi, property, &error)) {
		kdWarning() << "Property: " << property << " for: " << udi << " doesn't exist." << endl;
		goto out;
	}

	*returnval = libhal_device_get_property_string(hal_ctx, udi, property, &error);

	if (dbus_error_is_set(&error)) {
		kdError() << "Fetching property: " << property << " for: " << udi 
			  << " failed with: " << error.message << endl;
		dbus_error_free(&error);
		goto out;
	} else {
		ret = true;
	}

out:
	kdDebugFuncOut(trace);
	return ret;
}

/*!
 * This function query a String List property from HAL for a given device
 * \param udi		QString with the udi of the device
 * \param property	QString with the property to query 
 * \param devices 	QStringList to return the values
 * \return 		If the query was successful or not
 */
bool dbusHAL::halGetPropertyStringList (QString udi, QString property, QStringList *devices) {
	kdDebugFuncIn(trace);

	bool ret = false;

	if (!initHAL() || udi.isEmpty() || property.isEmpty()) 
		goto out;

	DBusError error;
	char ** found;

	dbus_error_init(&error);
	
	if (!libhal_device_property_exists(hal_ctx, udi, property, &error)) {
		kdWarning() << "Property: " << property << " for: " << udi << " doesn't exist." << endl;
		goto out;
	}

	found = libhal_device_get_property_strlist (hal_ctx, udi, property, &error);

	if (dbus_error_is_set(&error)) {
		kdWarning() << "Error while query existing strlist Property: " << property 
			    << " for: " << udi << " error: " << error.message << endl;
		dbus_error_free(&error);
		libhal_free_string_array(found);
		goto out;
        } else {
		for (int i = 0; found[i] != NULL ; ++i) {
			QString _to_add = found[i];
			if (!_to_add.isEmpty()) *devices += _to_add;
		}
		libhal_free_string_array(found);
		ret = true;
	}

out:
	kdDebugFuncOut(trace);
	return ret;
}


/*! 
 * This function query a capability from HAL for a given device
 * \param udi		QString with the UDI of the device
 * \param capability	QString with the capability to query
 * \param returnval 	pointer to the return value as boolean
 * \return 		If the query was successful or not
 */
bool dbusHAL::halQueryCapability(QString udi, QString capability, bool *returnval) {
	kdDebugFuncIn(trace);

	bool ret = false;

	if (!initHAL() || udi.isEmpty() || capability.isEmpty()) 
		goto out;

	DBusError error;
	dbus_error_init(&error);

	*returnval = libhal_device_query_capability(hal_ctx, udi, capability, &error);

	if (dbus_error_is_set(&error)) {
		kdError() << "Fetching capability: " << capability << " for: " << udi
			  << " failed with: " << error.message << endl;
		dbus_error_free(&error);
		goto out;
	} else {
		ret = true;
	}

out:
	kdDebugFuncOut(trace);
	return ret;
}

/*! 
 * Use this function to check if a device has a specia property/key.
 * \param udi		QString with the UDI of the device
 * \param property 	QString with the property
 * \return 		If the query was successful or not
 */
bool dbusHAL::halDevicePropertyExist(QString udi, QString property ) {
	kdDebugFuncIn(trace);

	bool ret = false;

	if (!initHAL() || udi.isEmpty() || property.isEmpty()) 
		goto out;

	DBusError error;
	dbus_error_init(&error);

	if (! libhal_device_property_exists (hal_ctx, udi, property, &error)) {
		if (dbus_error_is_set(&error)) {
			kdError() << "Fetching existing property: " << property << " for: " << udi 
				  << " failed with: " << error.message << endl;
			dbus_error_free(&error);
		}
		goto out;
	} else {
		ret = true;
	}

out:
	kdDebugFuncOut(trace);
	return ret;
}

/*!
 * Use this function to search find devices with a give capability
 * \param capability	QString with the capability to query
 * \param devices 	QStringList to return the found devices
 * \return 		If the query was successful or not
 */
bool dbusHAL::halFindDeviceByCapability (QString capability, QStringList *devices) {
	kdDebugFuncIn(trace);

	DBusError error;
	char ** found;
	int num = 0;
	bool ret = false;

	if (!initHAL() || capability.isEmpty()) 
		goto out;

	dbus_error_init(&error);
	
	found = libhal_find_device_by_capability (hal_ctx, capability, &num, &error);

	if (dbus_error_is_set(&error)) {
                kdError() << "Could not get list of devices with capability: " << capability	
			  << " error: " << error.message << endl;
		dbus_error_free(&error);
		libhal_free_string_array(found);
		goto out;
        } else {
 		for (int i = 0; i < num; ++i) {
			QString _to_add = found[i];
			if (!_to_add.isEmpty()) *devices += _to_add;
		}
		libhal_free_string_array(found);
		ret = true;
	}

out:
	kdDebugFuncOut(trace);
	return ret;
}

/*!
 * Use this function to search find devices with a special string property
 * \param property      QString with the name of the property 
 * \param keyval	QString with value of the string property
 * \param devices 	QStringList to return the found devices
 * \return 		If the query was successful or not
 */
bool dbusHAL::halFindDeviceByString (QString property, QString keyval, QStringList *devices) {
	kdDebugFuncIn(trace);

	DBusError error;
	char ** found;
	int num = 0;
	bool ret = false;

	if (!initHAL() || property.isEmpty() || keyval.isEmpty()) 
		goto out;
	
	dbus_error_init(&error);
	
	found = libhal_manager_find_device_string_match (hal_ctx, property, keyval, &num, &error);

	if (dbus_error_is_set(&error)) {
		kdError() << "Could not get list of devices with key: " << property
			  << "and string value: " <<  keyval << " error: " << error.message << endl;
		dbus_error_free(&error);
		libhal_free_string_array(found);
		goto out;
        } else {
 		for (int i = 0; i < num; ++i) {
			QString _to_add = found[i];
			if (!_to_add.isEmpty()) *devices += _to_add;
		}
		libhal_free_string_array(found);
		ret = true;
	}

out:
	kdDebugFuncOut(trace);
	return ret;
}

/* ----> HAL section :: END <---- */
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
bool dbusHAL::dbusSystemMethodCall( QString interface, QString path, QString object, QString method,
			   	    int first_arg_type, ... ) {
	kdDebugFuncIn(trace);
	
	bool _ret = false;
        va_list var_args;

        va_start(var_args, first_arg_type);
	_ret = dbusMethodCall( interface, path, object, method, DBUS_BUS_SYSTEM,
			       NULL, -1, first_arg_type, var_args);
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
bool dbusHAL::dbusSystemMethodCall( QString interface, QString path, QString object, QString method,
				    void *retvalue, int retval_type, int first_arg_type, ... ) {
	kdDebugFuncIn(trace);
	
	bool _ret = false;
        va_list var_args;

        va_start(var_args, first_arg_type);
	_ret = dbusMethodCall( interface, path, object, method, DBUS_BUS_SYSTEM, 
			       retvalue, retval_type, first_arg_type, var_args);
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
bool dbusHAL::dbusMethodCall( QString interface, QString path, QString object, QString method,
			      DBusBusType dbus_type, void *retvalue, int retval_type, int first_arg_type,
			      va_list var_args ) {
	kdDebugFuncIn(trace);

	DBusMessage *message;
	DBusMessage *reply;
	DBusError    error;
	bool ret = false;
	
	dbus_error_init(&error); 

	dbus_connection = dbus_bus_get(dbus_type, &error);
	
	if (dbus_error_is_set(&error)) {
		kdError() << "Could not get dbus connection: " << error.message << endl;
		dbus_error_free(&error);
		goto out;
	}

	message = dbus_message_new_method_call( interface, path, object, method );
	dbus_message_append_args_valist(message, first_arg_type, var_args);

	if (retvalue == NULL) {
		if (!dbus_connection_send(dbus_connection, message, NULL)) {
			kdError() << "Could not send method call." << endl;
			dbus_message_unref( message );
			goto out;
		}
	} else {
		reply = dbus_connection_send_with_reply_and_block(dbus_connection, message, -1, &error);

		if (dbus_error_is_set(&error)) {
			kdError() << "Could not send dbus message: " << error.message << endl;
			dbus_message_unref(message);
			dbus_error_free(&error);
			goto out;
		}

		int type = dbus_message_get_type(reply);
		if (type == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
			if (!dbus_message_get_args(reply, &error, retval_type, retvalue, DBUS_TYPE_INVALID)){
				if (dbus_error_is_set(&error)) {
					kdError() << "Could not get argument from reply: " 
						  << error.message << endl;
					dbus_error_free(&error);
				}
				dbus_message_unref(reply);
				dbus_message_unref(message);
				goto out;
			}
		} else {
			kdError() << "Revieved invalid DBUS_MESSAGE_TYPE: " << type 
				  << "expected: " << DBUS_MESSAGE_TYPE_METHOD_RETURN << endl;
			dbus_message_unref(reply);
			dbus_message_unref(message);
			goto out;
		}
	}

	ret = true;	// if we are here, everything should be okay
	dbus_message_unref(message);
	dbus_connection_flush(dbus_connection);

out:
	kdDebugFuncOut(trace);
        return ret;
}

/*! 
 * Function to call a suspend and call if resumed \ref callBackSuspend()
 * to emit a resume signal.
 * \param suspend 	a char pointer with the name of the suspend interface
 * \return 		If the query was successful or not
 */
bool dbusHAL::dbusMethodCallSuspend ( const char *suspend ) {
	kdDebugFuncIn(trace);

    	DBusMessage *message;
	DBusError    error;
	DBusPendingCall* pcall = NULL;
	bool ret = false;

	dbus_error_init(&error);
	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	
	if (dbus_error_is_set(&error)) {
		kdError() << "Could not get dbus connection: " << error.message << endl;
		dbus_error_free(&error);
		goto out;
	}
		
	message = dbus_message_new_method_call( HAL_SERVICE, HAL_COMPUTER_UDI, HAL_PM_IFACE, suspend);
	if (strcmp( suspend, "Suspend") == 0) {
		int wake_up = 0;
		dbus_message_append_args (message, DBUS_TYPE_INT32, &wake_up, DBUS_TYPE_INVALID);
	}
	
	if (message) {
		// need to set INT_MAX as default and not -1
		dbus_connection_send_with_reply (dbus_connection, message, &pcall, INT_MAX);
		if (pcall) {
			dbus_pending_call_ref (pcall); // really needed?
			dbus_pending_call_set_notify (pcall, dbusHAL::callBackSuspend, NULL, NULL);
		}
		dbus_message_unref (message);
		ret = true;
	}

out:
	kdDebugFuncOut(trace);
	return ret;
}

/*!
 * Slot called by D-Bus as set in \ref dbusMethodCallSuspend() 
 * Here we emit the resume signal.
 */
void dbusHAL::callBackSuspend (DBusPendingCall* pcall, void* /*data*/) {
	kdDebugFuncIn(trace);

	DBusMessage* reply = NULL;
	DBusError error;
	int result;
	bool failed = false;

        if (!pcall) {
		kdError() << "dbusHAL::callBackSuspend - DBusPendingCall not set, return" << endl;
		kdDebugFuncOut(trace);
		return;
        }

	reply = dbus_pending_call_steal_reply (pcall);
	if (reply == NULL) {
		kdError() << "dbusHAL::callBackSuspend - Got no reply, return" << endl;
		goto out;
	}

	dbus_error_init(&error);

        if (!dbus_message_get_args (reply, &error, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID)) {
		if (dbus_error_is_set(&error)) {
			kdError() << "Could not get argument from reply: " << error.message << endl;
			dbus_error_free(&error);
		}

		kdWarning() << "dbusHAL::callBackSuspend dbus_message_get_args failed, maybe timouted" << endl;
		failed = true;
	}

	dbus_message_unref (reply);

out:
	dbus_pending_call_unref (pcall);

	if (failed)
		emit ((dbusHAL*) myInstance)->backFromSuspend( -1 );
	else
		emit ((dbusHAL*) myInstance)->backFromSuspend( result );

	kdDebugFuncOut(trace);
	return;
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
int dbusHAL::isUserPrivileged(QString privilege, QString udi, QString ressource, QString user) {
	kdDebugFuncIn(trace);

	const char *_unique_name;
	const char *_user;
	const char *_privilege;	
	
	int retval = -1;

	if (user.isEmpty() || user.isNull()) 
		_user = getenv("USER"); 
	else 
		_user = user.latin1();

	if (_user == NULL || privilege.isEmpty()) 
		goto out;

	_unique_name = dbus_bus_get_unique_name(dbus_connection);
	_privilege = privilege.latin1();

#ifdef USE_LIBHAL_POLICYCHECK
	DBusError    error;
	char *result;

	if (udi.isEmpty()) {
		kdError() << "No UDI given ... could not lookup privileges" << endl;
		goto out;
	} 
	if (!hal_is_connected) {
		kdError() << "HAL not running, could not call libhal for lookup privileges" << endl;
		goto out;
	}

	dbus_error_init(&error);
	result = libhal_device_is_caller_privileged ( hal_ctx, udi.latin1(), _privilege, _unique_name, &error);
	
	if ( dbus_error_is_set( &error ) ) {
		kdWarning() << "Error while lookup privileges: " << error.message << endl;
		dbus_error_free( &error );
		retval = -1;
	} else {
		if (!strcmp(result, "yes")) {
			retval = 1;
		} else if (!strcmp(result, "no")) {
			retval = 0;
		} else {
			retval = -1;
		}
	}

	libhal_free_string(result);
#else
	// not sure if we need this, but to avoid problems
	dbus_bool_t _retval;
	const char *_ressource;
	_ressource = ressource.latin1();

	if (!dbusSystemMethodCall( "org.freedesktop.PolicyKit",
				   "/org/freedesktop/PolicyKit/Manager",
				   "org.freedesktop.PolicyKit.Manager",
				   "IsUserPrivileged",
				   &_retval, DBUS_TYPE_BOOLEAN,
				   DBUS_TYPE_STRING, &_unique_name,
				   DBUS_TYPE_STRING, &_user,
				   DBUS_TYPE_STRING, &_privilege,
				   DBUS_TYPE_STRING, &_ressource,
				   DBUS_TYPE_INVALID)) {
		retval = -1;	// only to be sure we have no changes trough the call
	} else {
		retval = (int) _retval;
	}
#endif

out:
	kdDebugFuncOut(trace);
	return retval;
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
void dbusHAL::emitMsgReceived( msg_type type, QString message, QString string ) {
	
	if (message.startsWith("dbus.terminate"))
		dbus_is_connected = false;

	if (type == POLICY_POWER_OWNER_CHANGED) {
		if (message.startsWith("NOW_OWNER"))
			aquiredPolicyPower = true;
		else 
			aquiredPolicyPower = false;
	}

	emit msgReceived_withStringString( type, message, string );
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
filterFunction (DBusConnection *connection, DBusMessage *message, void */*data*/) {
 	kdDebugFuncIn(trace);
	
	bool reply_wanted;
	char *value;
	QString ifaceType;

	DBusError error;
        dbus_error_init( &error );

	if (dbus_message_is_signal (message,
				    DBUS_INTERFACE_LOCAL,
				    "Disconnected")){
		((dbusHAL*) myInstance)->emitMsgReceived( DBUS_EVENT, "dbus.terminate", 0 );
		dbus_connection_unref(connection);
		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

        if ( dbus_message_get_type( message ) != DBUS_MESSAGE_TYPE_SIGNAL ) {
                if (trace) kdDebug() << "recieved message, but wasn't from type DBUS_MESSAGE_TYPE_SIGNAL" << endl;
		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

	ifaceType = dbus_message_get_interface( message );
        if (ifaceType == NULL) {
                kdDebug() << "Received message from invalid interface" << endl;
                kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

	reply_wanted = !dbus_message_get_no_reply( message );

	if (ifaceType.startsWith(DBUS_INTERFACE_DBUS)) {
		if(trace) kdDebug() << "Received from DBUS_INTERFACE_DBUS" << endl;
		/* get the name of the signal */
		const char *signal = dbus_message_get_member( message );
		
		/* get the first argument. This must be a string at the moment */
		dbus_message_get_args( message, &error, DBUS_TYPE_STRING, &value, DBUS_TYPE_INVALID );
	
		if ( dbus_error_is_set( &error ) ) {
			kdWarning() << "Received signal " << error.message << " but no string argument" << endl;
			dbus_error_free( &error );
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
	
		if (trace) kdDebug() << "filter_function::SIGNAL=" << signal << " VALUE=" << value << endl;
	
		/* our name is... */
		if ( ! strcmp( signal, "NameAcquired" ) ) {
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
		
		else if ( ! strcmp( signal, "NameOwnerChanged" )) {
			char *service;
			char *old_owner;
			char *new_owner;
			
                	if (dbus_message_get_args (message, NULL, DBUS_TYPE_STRING, &service,
                                                   DBUS_TYPE_STRING, &old_owner,
                                                   DBUS_TYPE_STRING, &new_owner, DBUS_TYPE_INVALID)) {
				if (!strcmp(service, "org.freedesktop.Hal")) {
					if (!strcmp(new_owner, "") && strcmp(old_owner, "")) {
						// Hal service stopped.
						kdDebug() << "=== org.freedesktop.Hal terminated ===" << endl;
						((dbusHAL*) myInstance)->emitMsgReceived( DBUS_EVENT,
											"hal.terminate", 
											NULL );
					}
					else if (!strcmp(old_owner, "") && strcmp(new_owner, "")) {
						// Hal service started.
						kdDebug() << "=== org.freedesktop.Hal started ===" << endl;
						((dbusHAL*) myInstance)->emitMsgReceived( DBUS_EVENT,
											"hal.started", 
											NULL );
					}
				} else if (!strcmp(service, "org.freedesktop.Policy.Power")) {
					const char *own_name;
					
					own_name = dbus_bus_get_unique_name(((dbusHAL*) myInstance)->get_DBUS_connection());

					if (!strcmp(new_owner, own_name)) {
						kdDebug() << "=== now owner of org.freedesktop.Policy.Power ===" << endl;
						// we have now again the ower of the name!
						((dbusHAL*) myInstance)->emitMsgReceived( POLICY_POWER_OWNER_CHANGED,
											  "NOW_OWNER", 
											  NULL );
					} else {
						// some other has now the interface
						kdDebug() << "=== someone owner of org.freedesktop.Policy.Power ===" << endl;
						((dbusHAL*) myInstance)->emitMsgReceived( POLICY_POWER_OWNER_CHANGED,
											  "OTHER_OWNER", 
											  NULL );
					}
				}
			}
		}
		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_HANDLED;	
	} else if (ifaceType.startsWith("org.freedesktop.Hal.Manager")) {
		kdDebug() << "Received from org.freedesktop.Hal.Manager" << endl;
		char *udi;

		const char *signal = dbus_message_get_member( message );
		/* get the first argument. This must be a string at the moment */
		dbus_message_get_args( message, &error, DBUS_TYPE_STRING, &value, DBUS_TYPE_INVALID );
	
		if ( dbus_error_is_set( &error ) ) {
			kdWarning() << "Received signal, but no string argument: " <<  error.message << endl;
			dbus_error_free( &error );
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

                if (dbus_message_get_args( message, &error, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID )) {
                        if (! strcmp(signal, "DeviceRemoved") || ! strcmp(signal, "DeviceAdded")) {
				((dbusHAL*) myInstance)->emitMsgReceived( HAL_DEVICE, signal, udi );
				kdDebug() << "org.freedesktop.Hal.Manager: uid: " << udi
					  << " signal: " << signal << endl;
                        } else {
				kdWarning() << "Received unknown signal from org.freedesktop.Hal.Manager: "
					    << signal << endl;
                        }
                }
		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_HANDLED;
		
	} else if (ifaceType.startsWith("org.freedesktop.Hal.Device")) {		
		const char *udi = dbus_message_get_path (message);
		const char *signal = dbus_message_get_member( message );

		/* Code taken from libhal */
		 if (! strcmp(signal, "PropertyModified")) {
			if (trace) kdDebug() << "-------- PropertyModified ------" << endl;
			int i, num_modifications;
			DBusMessageIter iter;
			DBusMessageIter iter_array;

			dbus_message_iter_init (message, &iter);
			dbus_message_iter_get_basic (&iter, &num_modifications);
			dbus_message_iter_next (&iter);
			
			dbus_message_iter_recurse (&iter, &iter_array);
			
			for (i = 0; i < num_modifications; i++) {
				dbus_bool_t removed, added;
				char *key;
				DBusMessageIter iter_struct;
			
				dbus_message_iter_recurse (&iter_array, &iter_struct);
			
				dbus_message_iter_get_basic (&iter_struct, &key);
				dbus_message_iter_next (&iter_struct);
				dbus_message_iter_get_basic (&iter_struct, &removed);
				dbus_message_iter_next (&iter_struct);
				dbus_message_iter_get_basic (&iter_struct, &added);
			
				/* don't check if we really need this device, check this in an other class */
				((dbusHAL*) myInstance)->emitMsgReceived( HAL_PROPERTY_CHANGED, udi, key);
				kdDebug() << "PropertyModified: uid: " << udi << " key: " << key << endl;

				dbus_message_iter_next (&iter_array);
			}
		} else if (! strcmp(signal, "Condition")) {
			if (trace) kdDebug() << "-------- Condition ------" << endl;
			char *name, *detail;

			dbus_message_get_args( message, &error, DBUS_TYPE_STRING, &value, DBUS_TYPE_INVALID );	

			if (dbus_message_get_args (message, &error, DBUS_TYPE_STRING, &name,
						   DBUS_TYPE_STRING, &detail, DBUS_TYPE_INVALID)) {
				((dbusHAL*) myInstance)->emitMsgReceived( HAL_CONDITION, name, detail );
			} else {
				if (dbus_error_is_set( &error )) dbus_error_free( &error );
			}
		} else {
			kdDebug() << "Received unknown signal from org.freedesktop.Hal.Device: "
				  << signal << endl;
		}
		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_HANDLED;
	} else if (ifaceType.startsWith("org.freedesktop.ConsoleKit.Session")) {
		kdDebug() << "Received from org.freedesktop.ConsoleKit.Session" << endl;

		const char *session = dbus_message_get_path (message);
		const char *signal = dbus_message_get_member( message );
		
		if (! strcmp(signal, "ActiveChanged")) {
			dbus_bool_t active;

			if (dbus_message_get_args( message, &error, DBUS_TYPE_BOOLEAN, &active, DBUS_TYPE_INVALID )) {
				((dbusHAL*) myInstance)->emitMsgReceived( CONSOLEKIT_SESSION_ACTIVE, 
									  session, QString("%1").arg((int)active));
			} else {
				if (dbus_error_is_set( &error )) dbus_error_free( &error );
			}
		} else {
			kdDebug() << "Received unknown signal from org.freedesktop.ConsoleKit.Session: "
				  << signal << endl;
			kdDebugFuncOut(trace);
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}
		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_HANDLED;
	} else {
		kdDebugFuncOut(trace);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
}

// --> some functions to get private members

//! to get the current connection to D-Bus
DBusConnection * dbusHAL::get_DBUS_connection() {
	return dbus_connection;
}
