#pragma once

#include "dbus_properties.hpp"
#include <dbus/dbus.h>
#include <string>
#include <optional>

namespace kps {

dict_type upower_get_all_props(DBusConnection *, const char *);
devices_type upower_get_devices(DBusConnection *);
std::string upower_get_display_device(DBusConnection *);

std::optional<std::string> upower_dev_serial(const dict_type&);
std::optional<uint32_t> upower_dev_type(const dict_type&);
std::optional<std::string> upower_dev_technology(const dict_type&);
std::optional<double> upower_dev_energy_rate(const dict_type&);
std::optional<double> upower_dev_energy_full_design(const dict_type&);
std::optional<double> upower_dev_percentage(const dict_type&);
std::optional<int64_t> upower_dev_remaining_time(const dict_type&,
						 const std::string&);
std::optional<uint32_t> upower_dev_state(const dict_type&);

}
