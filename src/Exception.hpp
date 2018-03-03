#ifndef __GP__Exception__hpp__
#define __GP__Exception__hpp__

#include <exception>
#include <map>
#include <utility>
#include <cstdarg>

namespace GP {

/**
 * \brief Encapsulates information about errors/warning/notices that may occur during HPP Mediation operation.
 */
class Exception : public std::exception {
public:
	/**
	 * \brief The constructor.
	 * 
	 * \param f The source file where the error occurred.
	 * \param l The line in the source file where the error occurred.
	 * \param error_code The error code. See #const unsigned int.
	 * \param args Extra arguments.
	 */
	Exception(const char *, unsigned int, unsigned int) noexcept;
	Exception(const char *, unsigned int, unsigned int, const char *, ...) noexcept __attribute__((format(printf, 5, 6)));
	Exception(const Exception&) noexcept;
	Exception(Exception&&) noexcept;
	Exception& operator=(const Exception&) noexcept;
	Exception& operator=(Exception&&) noexcept;
	/**
	 * \brief The empty destructor.
	 */
	virtual ~Exception() noexcept;
    
    	/**
    	 * \brief Accessor method that returns the source file name where the error occurred.
    	 * 
    	 * \return The source file name where the error occurred.
    	 */
	const char *file() const noexcept;
    	/**
    	 * \brief Accessor method that returns the line in the source file where the error occurred.
    	 * 
    	 * \return The line in the source file where the error occurred.
    	 */
	unsigned int line() const noexcept;
	/**
	 * \brief Accessor method returning the error code of this error.
	 * 
	 * \return The code of this error.
	 */
	unsigned int code() const noexcept;
	/**
	 * \brief Accessor method returning the error level.
	 * 
	 * \return The level of this error
	 */
	int level() const noexcept;
	/**
	 * \brief Returns the error message. Inherited from std::exception.
	 * 
	 * \return The error message of this error.
	 */
	virtual const char *what() const noexcept;

	const char *args() const noexcept;
protected:
	struct impl {
		char storage[256];
		char *str;
	};

	typedef std::pair<int, const char *> ErrorMessageAndLogLevel;
	typedef std::map<unsigned int, ErrorMessageAndLogLevel> ErrorMessages;

	//! \brief The static map mapping error codes to error messages and levels. Initialised in TLBMediation::child_init.
	static ErrorMessages& messages();
	static const ErrorMessages& const_messages();
	static ErrorMessages::const_iterator lookup_msg(unsigned int) noexcept;

	static std::pair<char *, char *>
	message(impl *, const char *, unsigned int, int, const char *,
		const char *, va_list) noexcept;
	static const char *basename(const char *) noexcept;

	//! \brief The source file where the error occurred.
	const char *file_;
	//! \brief The line in the source file where the error occurred.
	unsigned int line_;
	//! \brief The error code of this error.
	unsigned int code_;

	int level_;

	impl buf;

	const char *args_;
};

inline const char *
Exception::file() const noexcept {
	return file_;
}

inline unsigned int
Exception::line() const noexcept  {
	return line_;
}

inline unsigned int
Exception::code() const noexcept  {
	return code_;
}	

inline int
Exception::level() const noexcept  {
	return level_;
}

inline const char *
Exception::what() const noexcept {
	return buf.str;
}

inline const char *
Exception::args() const noexcept {
	return args_;
}

inline
Exception::Exception(const char *file__, unsigned int line__,
		     unsigned int ec__) noexcept
	: Exception(file__, line__, ec__, "%s", "")
{
}

}

#endif
