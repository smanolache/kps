#include "dbus_properties.hpp"
#include <dbus/dbus.h>
#include "Exception.hpp"
#include <any>
#include "error_codes.hpp"
#include "dbus_wrappers.hpp"
#include "QuoteJSON.hpp"

using GP::Exception;
using kps::EC;
using GP::QuoteJSON;

static std::ostream& operator<<(std::ostream&, const std::any&);

namespace kps {

static dict_type& get_dictionary(DBusMessageIter *, dict_type&);
static dict_type::value_type get_dict_entry(DBusMessageIter *);
static std::any get_variant(DBusMessageIter *, const char *);

static std::vector<std::string>& get_string_array(DBusMessageIter *,
						  std::vector<std::string>&);

dict_type&
operator>>(DBusMessage *reply, dict_type& d) {
	DBusMessageIter i;
	if (!dbus_message_iter_init(reply, &i))
		return d;
	int type;
	do {
		type = dbus_message_iter_get_arg_type(&i);
		switch (type) {
		case DBUS_TYPE_ARRAY:
			return get_dictionary(&i, d);
		case DBUS_TYPE_INVALID:
			throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" org.freedesktop.DBus.Properties.GetAll: "
				"Expected array.");
		default:
			throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" org.freedesktop.DBus.Properties.GetAll: "
				"Expected array.");
		}
	} while (dbus_message_iter_next(&i));
}

static dict_type&
get_dictionary(DBusMessageIter *i, dict_type& d) {
	DBusMessageIter j;
	dbus_message_iter_recurse(i, &j);
	do {
		int type;
		type = dbus_message_iter_get_arg_type(&j);
		switch (type) {
		case DBUS_TYPE_DICT_ENTRY:
			d.insert(get_dict_entry(&j));
			break;
		case DBUS_TYPE_INVALID:
			break;
		default:
			throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" org.freedesktop.DBus.Properties.GetAll: "
				"Expected dictionary entry element type.");
		}
	} while (dbus_message_iter_next(&j));
	return d;
}

static dict_type::value_type
get_dict_entry(DBusMessageIter *i) {
	DBusMessageIter j;

	dbus_message_iter_recurse(i, &j);

	// key
	int type = dbus_message_iter_get_arg_type(&j);
	if (DBUS_TYPE_STRING != type)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" org.freedesktop.DBus.Properties.GetAll: "
				"Expected dictionary element key.");

	const char *key = nullptr;
	dbus_message_iter_get_basic(&j, &key);

	if (!dbus_message_iter_next(&j))
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" org.freedesktop.DBus.Properties.GetAll: "
				"Truncated dictionary element key.");

	// value
	type = dbus_message_iter_get_arg_type(&j);
	if (DBUS_TYPE_VARIANT != type)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" org.freedesktop.DBus.Properties.GetAll: "
				"Expected dictionary element value.");

	return dict_type::value_type(key, get_variant(&j, key));
}

static std::any
get_variant(DBusMessageIter *i, const char *key) {
	DBusMessageIter j;
	dbus_message_iter_recurse(i, &j);
	char *sign = dbus_message_iter_get_signature(&j);
	if (nullptr == sign)
		return std::any();
	DBusString __sign(sign, &dbus_free);

	if ('\0' == *sign)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" Empty type signature.");
	char *s;
	bool flag;
	uint64_t u64;
	uint32_t u32;
	int64_t s64;
	double d;

	for (const char *v = sign; '\0' != *v;) {
		switch (static_cast<int>(*v)) {
		case DBUS_TYPE_STRING: // s
			dbus_message_iter_get_basic(&j, &s);
			return std::any(std::string(s));
		case DBUS_TYPE_BOOLEAN: // b
			dbus_message_iter_get_basic(&j, &flag);
			return std::any(flag);
		case DBUS_TYPE_UINT64: // t
			dbus_message_iter_get_basic(&j, &u64);
			return std::any(u64);
		case DBUS_TYPE_UINT32: // u
			dbus_message_iter_get_basic(&j, &u32);
			return std::any(u32);
		case DBUS_TYPE_INT64: // x
			dbus_message_iter_get_basic(&j, &s64);
			return std::any(s64);
		case DBUS_TYPE_DOUBLE: // d
			dbus_message_iter_get_basic(&j, &d);
			return std::any(d);
		default:
			throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
					" Unsupported type.");
		}
		++v;
		if ('\0' == *v)
			break;
		if (!dbus_message_iter_next(&j))
			break;
	}
	return std::any();
}

devices_type&
operator>>(DBusMessage *reply, devices_type& d) {
	DBusMessageIter i;
	if (!dbus_message_iter_init(reply, &i))
		return d;
	int type;
	do {
		type = dbus_message_iter_get_arg_type(&i);
		switch (type) {
		case DBUS_TYPE_ARRAY:
			return get_string_array(&i, d);
		case DBUS_TYPE_INVALID:
			throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" org.freedesktop.DBus.Properties.GetAll: "
				"Expected array.");
		default:
			throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" org.freedesktop.DBus.Properties.GetAll: "
				"Expected array.");
		}
	} while (dbus_message_iter_next(&i));
}

static std::vector<std::string>&
get_string_array(DBusMessageIter *i, std::vector<std::string>& d) {
	DBusMessageIter j;
	dbus_message_iter_recurse(i, &j);
	do {
		int type;
		const char *obj = nullptr;
		type = dbus_message_iter_get_arg_type(&j);
		switch (type) {
		case DBUS_TYPE_OBJECT_PATH:
			dbus_message_iter_get_basic(&j, &obj);
			d.push_back(obj);
			break;
		case DBUS_TYPE_INVALID:
			break;
		default:
			throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
				" org.freedesktop.DBus.Properties.GetAll: "
				"Expected dictionary entry element type.");
		}
	} while (dbus_message_iter_next(&j));
	return d;
}

modified_props_type&
operator>>(DBusMessage *reply, modified_props_type& d) {
	DBusMessageIter i;
	if (!dbus_message_iter_init(reply, &i))
		return d;

	// interface
	int type = dbus_message_iter_get_arg_type(&i);
	if (DBUS_TYPE_STRING != type)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
			" org.freedesktop.DBus.Properties.PropertiesChanged: "
			"Expected interface name.");
	char *s = nullptr;
	dbus_message_iter_get_basic(&i, &s);
	d.interface = s;

	if (!dbus_message_iter_next(&i))
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
			" org.freedesktop.DBus.Properties.PropertiesChanged: "
			"Truncated.");

	// dictionary
	type = dbus_message_iter_get_arg_type(&i);
	if (DBUS_TYPE_ARRAY != type)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
			" org.freedesktop.DBus.Properties.PropertiesChanged: "
			"Expected dictionary.");
	get_dictionary(&i, d.modified);

	if (!dbus_message_iter_next(&i))
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
			" org.freedesktop.DBus.Properties.PropertiesChanged: "
			"Truncated.");

	// protected properties
	type = dbus_message_iter_get_arg_type(&i);
	if (DBUS_TYPE_ARRAY != type)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
			" org.freedesktop.DBus.Properties.PropertiesChanged: "
			"Expected dictionary.");
	get_string_array(&i, d.protected_modified);

	if (dbus_message_iter_next(&i))
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
			" org.freedesktop.DBus.Properties.PropertiesChanged: "
			"Unexpected data.");

	return d;
}

std::string&
operator>>(DBusMessage *reply, std::string& d) {
	DBusMessageIter i;
	if (!dbus_message_iter_init(reply, &i))
		return d;

	// interface
	int type = dbus_message_iter_get_arg_type(&i);
	if (DBUS_TYPE_OBJECT_PATH != type)
		throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
			" org.freedesktop.DBus.Properties.PropertiesChanged: "
			"Expected object path.");
	char *s = nullptr;
	dbus_message_iter_get_basic(&i, &s);
	d = s;

	return d;
}

}

std::ostream&
operator<<(std::ostream& os, const kps::dict_type& d) {
	os << '{';
	kps::dict_type::const_iterator i = d.begin(), __li = d.end();
	if (i == __li)
		return os << '}';
	os << '"' << QuoteJSON::quote(i->first) << "\": "
	   << i->second;
	for (++i; i != __li; ++i)
		os << ",\"" << QuoteJSON::quote(i->first) << "\": "
		   << i->second;
	return os << '}';
}

static std::ostream&
operator<<(std::ostream& os, const std::any& v) {
	const std::string *s = std::any_cast<std::string>(&v);
	if (s)
		return os << '"' << QuoteJSON::quote(*s) << '"';
	const bool *b = std::any_cast<bool>(&v);
	if (b)
		return os << (*b ? "true" : "false");
	const uint64_t *u64 = std::any_cast<uint64_t>(&v);
	if (u64)
		return os << *u64;
	const uint32_t *u32 = std::any_cast<uint32_t>(&v);
	if (u32)
		return os << *u32;
	const int64_t *s64 = std::any_cast<int64_t>(&v);
	if (s64)
		return os << *s64;
	const double *d = std::any_cast<double>(&v);
	if (d)
		return os << *d;
	throw Exception(__FILE__, __LINE__, EC::DBUS_ERROR,
			" Unsupported property type.");
}

std::ostream&
operator<<(std::ostream& os, const kps::devices_type& v) {
	os << '[';
	kps::devices_type::const_iterator i = v.begin(), __li = v.end();
	if (i == __li)
		return os << ']';
	os << '"' << QuoteJSON::quote(*i) << '"';
	for (++i; i != __li; ++i)
		os << ",\"" << QuoteJSON::quote(*i) << '"';
	return os << ']';
}

std::ostream&
operator<<(std::ostream& os, const kps::modified_props_type& v) {
	return os << "{\"interface\": \"" << QuoteJSON::quote(v.interface)
		  << "\", \"modified\": " << v.modified
		  << ", \"protected\": " << v.protected_modified
		  << '}';
}
