#include "QuoteJSON.hpp"
#include <string>
#include <utility>
#include <boost/algorithm/string/replace.hpp>

namespace GP {

static const std::pair<const char *, const char *> json_entities__[] = {
	std::pair<const char *, const char *>("\\", "\\\\"),
	std::pair<const char *, const char *>("\"", "\\\""),
	std::pair<const char *, const char *>("\t", "\\t"),
	std::pair<const char *, const char *>("\r", "\\r"),
	std::pair<const char *, const char *>("\n", "\\n"),
	std::pair<const char *, const char *>("\f", "\\f"),
	std::pair<const char *, const char *>("\b", "\\b")
};

std::string
QuoteJSON::quote(std::string&& s) {
	std::string r(std::move(s));
	for (unsigned int i = 0; i < sizeof(json_entities__) / sizeof(json_entities__[0]); ++i)
		boost::replace_all(r, json_entities__[i].first, json_entities__[i].second);
	return r;
}

std::string
QuoteJSON::quote(const std::string& s) {
	std::string r(s);
	for (unsigned int i = 0; i < sizeof(json_entities__) / sizeof(json_entities__[0]); ++i)
		boost::replace_all(r, json_entities__[i].first, json_entities__[i].second);
	return r;
}

std::string
QuoteJSON::unquote(std::string&& s) {
	std::string r(std::move(s));
	for (unsigned int i = sizeof(json_entities__) / sizeof(json_entities__[0]); i > 0;) {
		--i;
		boost::replace_all(r, json_entities__[i].second, json_entities__[i].first);
	}
	return r;
}

std::string
QuoteJSON::unquote(const std::string& s) {
	std::string r(s);
	for (unsigned int i = sizeof(json_entities__) / sizeof(json_entities__[0]); i > 0;) {
		--i;
		boost::replace_all(r, json_entities__[i].second, json_entities__[i].first);
	}
	return r;
}

}
