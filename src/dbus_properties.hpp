#pragma once

#include <dbus/dbus.h>
#include <string>
#include <any>
#include <map>
#include <vector>
#include <ostream>

namespace kps {

typedef std::map<std::string, std::any> dict_type;
typedef std::vector<std::string> devices_type;

struct modified_props_type {
	typedef std::vector<std::string> prop_names_type;
	std::string interface;
	dict_type modified;
	prop_names_type protected_modified;
};

dict_type& operator>>(DBusMessage *, dict_type&);
devices_type& operator>>(DBusMessage *, devices_type&);
modified_props_type& operator>>(DBusMessage *, modified_props_type&);
std::string& operator>>(DBusMessage *, std::string&);

}

std::ostream& operator<<(std::ostream&, const kps::dict_type&);
std::ostream& operator<<(std::ostream&, const kps::devices_type&);
std::ostream& operator<<(std::ostream&, const kps::modified_props_type&);
