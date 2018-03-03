#include "Exception.hpp"
#include <cstdarg>
#include <cstdio>
#include <new>
#include <cstring>
#include <cstddef>

namespace GP {

Exception::Exception(const char *file__, unsigned int line__,
		     unsigned int ec__, const char *fmt, ...) noexcept
	: file_(file__)
	, line_(line__)
	, code_(ec__)
	, args_(nullptr)
{
	va_list ap;
	va_start(ap, fmt);
	ErrorMessages::const_iterator msg = lookup_msg(code_);
	const char *errmsg = msg == const_messages().end() ?
		"" : msg->second.second;
	std::pair<char *, char *> p = message(&buf, file_, line_, code_,
					      errmsg, fmt, ap);
	buf.str = p.first;
	args_ = p.second;
	va_end(ap);
	level_ = msg == const_messages().end() ? 0 : msg->second.first;
}

std::pair<char *, char *>
Exception::message(impl *u, const char *file, unsigned int line, int code,
		   const char *errmsg, const char *fmt, va_list v) noexcept {
	char *buf = u->storage;
	unsigned int allocated = sizeof(u->storage);
	int n = snprintf(buf, sizeof(u->storage), "%s(%u): %s", basename(file),
			 line, errmsg);
	if (n >= static_cast<int>(sizeof(u->storage))) {
		allocated = n + 1;
		buf = reinterpret_cast<char *>(
			::operator new[](allocated, std::nothrow_t()));
		if (nullptr == buf)
			return std::make_pair(
				u->storage,
				u->storage + sizeof(u->storage) - 1);
		snprintf(buf, allocated, "%s(%u): %s", basename(file), line,
			 errmsg);
	}
	va_list cpy;
	va_copy(cpy, v);
	int n2 = vsnprintf(buf + n, allocated - n, fmt, v);
	if (n2 < static_cast<int>(allocated - n)) {
		va_end(cpy);
		return std::make_pair(buf, buf + n);
	}
	char *nbuf = reinterpret_cast<char *>(
		::operator new[](n2 + n + 1, std::nothrow_t()));
	if (nullptr == nbuf) {
		va_end(cpy);
		return std::make_pair(buf, buf + n);
	}
	memcpy(nbuf, buf, n);
	if (buf != u->storage)
		delete [] buf;
	vsnprintf(nbuf + n, n2 + 1, fmt, cpy);
	va_end(cpy);
	return std::make_pair(nbuf, nbuf + n);
}

Exception::ErrorMessages::const_iterator
Exception::lookup_msg(unsigned int code) noexcept {
	try {
		return const_messages().find(code);
	} catch (...) {
		return const_messages().end();
	}
}

Exception::Exception(const Exception& other) noexcept
	: file_(other.file_)
	, line_(other.line_)
	, code_(other.code_)
	, level_(other.level_)
{
	if (other.buf.str == other.buf.storage) {
		// source is in storage => it will fit in storage
		memcpy(buf.storage, other.buf.str, sizeof(buf.storage));
		buf.str = buf.storage;
		args_ = buf.str + (other.args_ - other.buf.str);
		return;
	}
	// source is not in storage
	std::size_t l = strlen(other.buf.str);
	if (l < sizeof(buf.storage)) {
		// but it could fit in storage
		memcpy(buf.storage, buf.str, l + 1);
		buf.str = buf.storage;
		args_ = buf.str + (other.args_ - other.buf.str);
	} else {
		// source is not in storage and does not fit there
		buf.str = reinterpret_cast<char *>(
			::operator new[](l + 1, std::nothrow_t()));
		if (nullptr == buf.str) {
			// allocation error => salvage as much as possible from the source
			memcpy(buf.storage, other.buf.str, sizeof(buf.storage) - 1);
			buf.storage[sizeof(buf.storage) - 1] = '\0';
			buf.str = buf.storage;
			args_ = buf.str + (other.args_ - other.buf.str);
			if (args_ >= buf.str + sizeof(buf.storage))
				args_ = buf.str + sizeof(buf.storage) - 1;
		} else {
			memcpy(buf.str, other.buf.str, l + 1);
			args_ = buf.str + (other.args_ - other.buf.str);
		}
	}
}

Exception::Exception(Exception&& other) noexcept
	: file_(other.file_)
	, line_(other.line_)
	, code_(other.code_)
	, level_(other.level_)
{
	if (other.buf.str == other.buf.storage) {
		// source is in storage => it will fit in storage
		memcpy(buf.storage, other.buf.str, sizeof(buf.storage));
		buf.str = buf.storage;
		args_ = buf.str + (other.args_ - other.buf.str);
		return;
	}
	// source is not in storage
	std::size_t l = strlen(other.buf.str);
	if (l < sizeof(buf.storage)) {
		// but it could fit in storage
		memcpy(buf.storage, buf.str, l + 1);
		buf.str = buf.storage;
		args_ = buf.str + (other.args_ - other.buf.str);
	} else {
		buf.str = other.buf.str;
		other.buf.str = nullptr;
		args_ = other.args_;
		other.args_ = nullptr;
	}
}

Exception::~Exception() noexcept {
	if (buf.str != buf.storage)
		delete [] buf.str;
}

Exception&
Exception::operator=(const Exception& other) noexcept {
	if (this == &other)
		return *this;
	file_ = other.file_;
	line_ = other.line_;
	code_ = other.code_;
	level_ = other.level_;

	if (other.buf.str == other.buf.storage) {
		// source is in storage => it will fit in storage
		memcpy(buf.storage, other.buf.str, sizeof(buf.storage));
		if (buf.str != buf.storage) {
			delete [] buf.str;
			buf.str = buf.storage;
		}
		args_ = buf.str + (other.args_ - other.buf.str);
		return *this;
	}
	// source is not in storage
	std::size_t l = strlen(other.buf.str);
	if (l < sizeof(buf.storage)) {
		// but it could fit in storage
		memcpy(buf.storage, buf.str, l + 1);
		if (buf.str != buf.storage) {
			delete [] buf.str;
			buf.str = buf.storage;
		}
		args_ = buf.str + (other.args_ - other.buf.str);
		return *this;
	}
	// source is not in storage and does not fit there
	std::size_t m = strlen(buf.str);
	if (buf.str != buf.storage) {
		// destination is not in storage either
		if (l < m) {
			// as the destination is larger, the source fits
			strcpy(buf.str, other.buf.str);
			args_ = buf.str + (other.args_ - other.buf.str);
			return *this;
		}
		// the source does not fit => reallocate
		delete [] buf.str;
	}
	buf.str = reinterpret_cast<char *>(
		::operator new[](l + 1, std::nothrow_t()));
	if (nullptr == buf.str) {
		// allocation error => salvage as much as possible from the source
		memcpy(buf.storage, other.buf.str, sizeof(buf.storage) - 1);
		buf.storage[sizeof(buf.storage) - 1] = '\0';
		buf.str = buf.storage;
		args_ = buf.str + (other.args_ - other.buf.str);
		if (args_ >= buf.str + sizeof(buf.storage))
			args_ = buf.str + sizeof(buf.storage) - 1;
		return *this;
	}
	memcpy(buf.str, other.buf.str, l + 1);
	args_ = buf.str + (other.args_ - other.buf.str);
	return *this;
}

Exception&
Exception::operator=(Exception&& other) noexcept {
	file_ = other.file_;
	line_ = other.line_;
	code_ = other.code_;
	level_ = other.level_;

	if (other.buf.str == other.buf.storage) {
		// source is in storage => it will fit in storage
		memcpy(buf.storage, other.buf.str, sizeof(buf.storage));
		if (buf.str != buf.storage) {
			delete [] buf.str;
			buf.str = buf.storage;
		}
		args_ = buf.str + (other.args_ - other.buf.str);
		return *this;
	}
	// source is not in storage
	std::size_t l = strlen(other.buf.str);
	if (l < sizeof(buf.storage)) {
		// but it could fit in storage
		memcpy(buf.storage, buf.str, l + 1);
		if (buf.str != buf.storage) {
			delete [] buf.str;
			buf.str = buf.storage;
		}
		args_ = buf.str + (other.args_ - other.buf.str);
		return *this;
	}
	// source is not in storage and does not fit there
	if (buf.str != buf.storage)
		delete [] buf.str;
	buf.str = other.buf.str;
	other.buf.str = nullptr;
	args_ = other.args_;
	other.args_ = nullptr;
	return *this;
}

Exception::ErrorMessages&
Exception::messages() {
	static ErrorMessages messages_;
	return messages_;
}

const Exception::ErrorMessages&
Exception::const_messages() {
	return messages();
}

const char *
Exception::basename(const char *file) noexcept {
	const char *p = strrchr(file, '/');
	if (nullptr == p)
		return file;
	return p + 1;
}

}
