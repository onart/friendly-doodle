#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <sstream>

template<class... T>
std::string asString(const T&... args) {
	std::stringstream ss;
	((ss << args << ' '), ...);
	return ss.str();
}

template<char sep = ' ', class... T>
std::string asString2(const T&... args) {
	std::stringstream ss;
	((ss << args << sep), ...);
	return ss.str();
}

#endif

