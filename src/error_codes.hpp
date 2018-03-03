#pragma once

namespace kps {

struct EC {
	static const unsigned int OK                     = 0;
	static const unsigned int DBUS_ERROR             = 500;
	static const unsigned int LOGIC_ERROR            = 600;
	static const unsigned int UNREFERENCED_MSG       = 999;
};

}
