#include "UPowerProperties.hpp"
#include "dbus_properties.hpp"
#include "Exception.hpp"
#include "dbus_wrappers.hpp"
#include <dbus/dbus.h>
#include "error_codes.hpp"
#include <optional>

using GP::Exception;
using kps::EC;

namespace kps {

static const char UPOWER_DEVICE_PATH[] = "org.freedesktop.UPower.Device";

dict_type
upower_get_all_props(DBusConnection *conn, const char *obj) {
	DBusMessage *msg =
		dbus_message_new_method_call(
			"org.freedesktop.UPower", obj,
			"org.freedesktop.DBus.Properties", "GetAll");
	if (nullptr == msg)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_new_method_call returned null.");
	DBusMsg __msg(msg, &dbus_message_unref);
	const char *dev = &UPOWER_DEVICE_PATH[0];
	if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &dev,
				      DBUS_TYPE_INVALID))
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_message_append_args error.");
	DBusError err;
	dbus_error_init(&err);
	DBusErr __err(&err, &dbus_error_free);

	DBusMessage *reply =
		dbus_connection_send_with_reply_and_block(
			conn, msg, -1, &err);
	if (nullptr == reply)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_connection_send_with_reply_and_block "
				"returned null. Error message: %s.",
				err.message ? err.message : "null");

	DBusMsg __reply(reply, &dbus_message_unref);
	if (dbus_error_is_set(&err))
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_connection_send_with_reply_and_block: "
				"%s.", err.message);

	dict_type d;
	int type = dbus_message_get_type(reply);
	switch (type) {
	case DBUS_MESSAGE_TYPE_METHOD_RETURN:
		return reply >> d;
	default:
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_connection_send_with_reply_and_block: "
				"Expected DBUS_MESSAGE_TYPE_METHOD_RETURN.");
	}
}

devices_type
upower_get_devices(DBusConnection *conn) {
	DBusMessage *msg =
		dbus_message_new_method_call(
			"org.freedesktop.UPower", "/org/freedesktop/UPower",
			"org.freedesktop.UPower", "EnumerateDevices");
	if (nullptr == msg)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_new_method_call returned null.");
	DBusMsg __msg(msg, &dbus_message_unref);
	if (!dbus_message_append_args(msg, DBUS_TYPE_INVALID))
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_message_append_args error.");
	DBusError err;
	dbus_error_init(&err);
	DBusErr __err(&err, &dbus_error_free);

	DBusMessage *reply =
		dbus_connection_send_with_reply_and_block(
			conn, msg, -1, &err);
	if (nullptr == reply)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_connection_send_with_reply_and_block "
				"returned null. Error message: %s.",
				err.message ? err.message : "null");

	DBusMsg __reply(reply, &dbus_message_unref);
	if (dbus_error_is_set(&err))
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_connection_send_with_reply_and_block: "
				"%s.", err.message);

	devices_type d;
	int type = dbus_message_get_type(reply);
	switch (type) {
	case DBUS_MESSAGE_TYPE_METHOD_RETURN:
		return reply >> d;
	default:
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_connection_send_with_reply_and_block: "
				"Expected DBUS_MESSAGE_TYPE_METHOD_RETURN.");
	}
}

std::string
upower_get_display_device(DBusConnection *conn) {
	DBusMessage *msg =
		dbus_message_new_method_call(
			"org.freedesktop.UPower", "/org/freedesktop/UPower",
			"org.freedesktop.UPower", "GetDisplayDevice");
	if (nullptr == msg)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_new_method_call returned null.");
	DBusMsg __msg(msg, &dbus_message_unref);
	if (!dbus_message_append_args(msg, DBUS_TYPE_INVALID))
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_message_append_args error.");
	DBusError err;
	dbus_error_init(&err);
	DBusErr __err(&err, &dbus_error_free);

	DBusMessage *reply =
		dbus_connection_send_with_reply_and_block(
			conn, msg, -1, &err);
	if (nullptr == reply)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_connection_send_with_reply_and_block "
				"returned null. Error message: %s.",
				err.message ? err.message : "null");

	DBusMsg __reply(reply, &dbus_message_unref);
	if (dbus_error_is_set(&err))
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_connection_send_with_reply_and_block: "
				"%s.", err.message);

	std::string d;
	int type = dbus_message_get_type(reply);
	switch (type) {
	case DBUS_MESSAGE_TYPE_METHOD_RETURN:
		return reply >> d;
	default:
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" dbus_connection_send_with_reply_and_block: "
				"Expected DBUS_MESSAGE_TYPE_METHOD_RETURN.");
	}
}

std::optional<std::string>
upower_dev_serial(const dict_type& props) {
	dict_type::const_iterator i = props.find("Serial");
	if (props.end() == i)
		return {};
	const std::string *s = std::any_cast<std::string>(&i->second);
	if (!s)
		return {};
	return *s;
}

std::optional<uint32_t>
upower_dev_type(const dict_type& props) {
	dict_type::const_iterator i = props.find("Type");
	if (props.end() == i)
		return {};
	const uint32_t *n = std::any_cast<uint32_t>(&i->second);
	if (!n)
		return {};
	return *n;
}

std::optional<std::string>
upower_dev_technology(const dict_type& props) {
	dict_type::const_iterator i = props.find("Technology");
	if (props.end() == i)
		return {};
	const uint32_t *n = std::any_cast<uint32_t>(&i->second);
	if (!n)
		return {};
	switch (*n) {
	default:
	case 0:
		return {};
	case 1:
		return "Lithium ion";
	case 2:
		return "Lithium polymer";
	case 3:
		return "Lithium iron phosphate";
	case 4:
		return "Lead acid";
	case 5:
		return "Nickel cadmium";
	case 6:
		return "Nickel metal hydride";
	}
}

std::optional<double>
upower_dev_energy_rate(const dict_type& props) {
	dict_type::const_iterator i = props.find("EnergyRate");
	if (props.end() == i)
		return {};
	const double *n = std::any_cast<double>(&i->second);
	if (!n)
		return {};
	return *n;
}

std::optional<double>
upower_dev_energy_full_design(const dict_type& props) {
	dict_type::const_iterator i = props.find("EnergyFullDesign");
	if (props.end() == i)
		return {};
	const double *n = std::any_cast<double>(&i->second);
	if (!n)
		return {};
	return *n;
}

std::optional<double>
upower_dev_percentage(const dict_type& props) {
	dict_type::const_iterator i = props.find("Percentage");
	if (props.end() == i)
		return {};
	const double *n = std::any_cast<double>(&i->second);
	if (!n)
		return {};
	return *n;
}

std::optional<int64_t>
upower_dev_remaining_time(const dict_type& props,const std::string& key) {
	dict_type::const_iterator i = props.find(key);
	if (props.end() == i)
		return {};
	const int64_t *n = std::any_cast<int64_t>(&i->second);
	if (!n)
		return {};
	return *n;
}

std::optional<uint32_t>
upower_dev_state(const dict_type& props) {
	dict_type::const_iterator i = props.find("State");
	if (props.end() == i)
		return {};
	const uint32_t *n = std::any_cast<uint32_t>(&i->second);
	if (!n)
		return {};
	return *n;
}

}
