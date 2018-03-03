#pragma once

#include <string>

namespace GP {

class QuoteJSON {
public:
	static std::string quote(std::string&&);
	static std::string quote(const std::string&);
	static std::string unquote(std::string&&);
	static std::string unquote(const std::string&);
};

}
