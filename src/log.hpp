#pragma once

#include <string>
#include <sstream>
#include <stdio.h> 
#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#include <stdexcept>
#include "util/errno_except.hpp"
#include "editor.hpp"

enum LogLevel {
								NONE = 0,
								CRITICAL,
								ERROR,
								WARNING,
								PRINT,
								INFO,
								DETAIL,
								DEBUG,
								DEBUG2
};


template<int L=4>
struct default_logger_traits
{
	constexpr static const char* name = appName;
	constexpr static int logLevel = L;
};


template <class logger_traits = default_logger_traits<4>>
struct Log
{
	static FILE* logfile;
	
protected:
	static void ezprint (const char* fmt, ...);
	
public:

	static void initialize ();
	
	static void finalize ();
	
	static void detail (const char* fmt, ...);
	
	static void info (const char* fmt, ...);
	
	static void print (const char* fmt, ...);
	
	static void warn (const char* fmt, ...);
	
	static void error (const char* fmt, ...);
	
	static void critical (const char* fmt, ...);
	
};


template<class logger_traits>
void Log<logger_traits>::initialize ()
{
	using namespace std;
	pid_t mypid = getpid();
	stringstream sstr;
	sstr << "/tmp/" << appName << "/" << logger_traits::name << "." << mypid;
	logfile = fopen(sstr.str().c_str(), "w+");
	if (!logfile) { throw errno_exception(std::runtime_error); }
}


template<class logger_traits>
inline void Log<logger_traits>::finalize ()
{
	assert(logfile != nullptr);
	if (!fclose(logfile)) { throw errno_exception(std::runtime_error); }
}


template<class logger_traits>
inline void Log<logger_traits>::detail (const char* fmt, ...)
{
	if (logger_traits::level > LogLevel::NONE) {
		
	}
}


template<class logger_traits>
inline void Log<logger_traits>::info (const char* fmt, ...)
{

}


template<class logger_traits>
inline void Log<logger_traits>::print (const char* fmt, ...)
{

}


template<class logger_traits>
inline void Log<logger_traits>::warn (const char* fmt, ...)
{

}


template<class logger_traits>
inline void Log<logger_traits>::error (const char* fmt, ...)
{

}


template<class logger_traits>
inline void Log<logger_traits>::critical (const char* fmt, ...)
{

}



// Our core application logger, a singleton.
typedef Log<> log;
