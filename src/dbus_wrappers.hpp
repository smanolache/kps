#pragma once

#include <dbus/dbus.h>
#include <memory>
#include <functional>

namespace kps {

typedef std::unique_ptr<DBusError, std::function<void(DBusError *)> > DBusErr;
typedef std::unique_ptr<DBusMessage,
			std::function<void(DBusMessage *)> > DBusMsg;
typedef std::unique_ptr<char, std::function<void(char *)> > DBusString;

}
