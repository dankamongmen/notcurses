#ifndef __NCPP_EXCEPTIONS_HH
#define __NCPP_EXCEPTIONS_HH

#include <stdexcept>

#include "_helpers.hh"

namespace ncpp
{
	class NCPP_API_EXPORT init_error : public std::logic_error
	{
	public:
		explicit init_error (const std::string& what_arg)
			: logic_error (what_arg)
		{}

		explicit init_error (const char* what_arg)
			: logic_error (what_arg)
		{}
	};

	class NCPP_API_EXPORT invalid_state_error : public std::logic_error
	{
	public:
		explicit invalid_state_error (const std::string& what_arg)
			: logic_error (what_arg)
		{}

		explicit invalid_state_error (const char* what_arg)
			: logic_error (what_arg)
		{}
	};

	class NCPP_API_EXPORT invalid_argument : public std::invalid_argument
	{
	public:
		explicit invalid_argument (const std::string& what_arg)
			: std::invalid_argument (what_arg)
		{}

		explicit invalid_argument (const char* what_arg)
			: std::invalid_argument (what_arg)
		{}
	};

	class NCPP_API_EXPORT call_error : public std::logic_error
	{
	public:
		explicit call_error (const std::string& what_arg)
			: logic_error (what_arg)
		{}

		explicit call_error (const char* what_arg)
			: logic_error (what_arg)
		{}
	};
}
#endif
