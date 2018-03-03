#include "error_codes.hpp"
#include "Exception.hpp"
#include <syslog.h>

namespace kps {

namespace EC_detail {

struct Exc : public GP::Exception {
	static bool init();
private:
	static bool dummy;
	static bool initialised;
};

bool Exc::initialised = false;
bool Exc::dummy = Exc::init();
bool
Exc::init() {
	if (initialised)
		return true;
	messages().insert({
		{EC::OK,          {LOG_INFO,    "OK."}},
		{EC::DBUS_ERROR,  {LOG_ERR,     "DBus error."}},
		{EC::LOGIC_ERROR, {LOG_ERR,     "Logic error."}}
	});
	initialised = true;
	return true;
}

}

const unsigned int EC::OK;
const unsigned int EC::DBUS_ERROR;
const unsigned int EC::UNREFERENCED_MSG;

}
