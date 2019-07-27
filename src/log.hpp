#pragma once

#include <string>
#include <sstream>
#include <stdio.h>
#include <sys/stat.h>
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


#ifndef LOGLEVEL_LIVEPARSE
#define LOGLEVEL_LIVEPARSE 4
#endif


template<int L=LOGLEVEL_LIVEPARSE>
struct default_logger_traits
{
	constexpr static const char* name = appName;
	constexpr static int logLevel = L;
};


template <class logger_traits = default_logger_traits<>>
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
	
	static void warning (const char* fmt, ...);
	
	static void error (const char* fmt, ...);
	
	static void critical (const char* fmt, ...);
	
};


template <>
FILE* Log<>::logfile;


template<class logger_traits>
void Log<logger_traits>::initialize ()
{
	using namespace std;
	pid_t mypid = getpid();
	stringstream sstrfn;
	stringstream sstrdir;
	sstrdir << "/tmp/" << appName;
	mkdir(sstrdir.str().c_str(),S_IWUSR|S_IRUSR|S_IXUSR);
	sstrfn << "/tmp/" << appName << "/" << logger_traits::name << ".log";
	logfile = fopen(sstrfn.str().c_str(), "w+");
	if (!logfile) { throw errno_exception(std::runtime_error); }
}


template<class logger_traits>
inline void Log<logger_traits>::finalize ()
{
	assert(logfile != nullptr);
	if (!fclose(logfile)) { throw errno_exception(std::runtime_error); }
}


template<class logger_traits>
inline void Log<logger_traits>::ezprint (const char* fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	char msg[1024];
	int chrs = vsnprintf(msg,1024,fmt,ap);
	va_end(ap);
	if (!fwrite(msg,1,chrs,logfile)) {
		throw errno_runtime_error;
	}
	if (!fputc('\n',logfile)) {
		throw errno_runtime_error;
	}
}


template<class logger_traits>
inline void Log<logger_traits>::detail (const char* fmt, ...)
{
	if (logger_traits::logLevel < LogLevel::DETAIL) return;
	ezprint(fmt);
}


template<class logger_traits>
inline void Log<logger_traits>::info (const char* fmt, ...)
{
	if (logger_traits::logLevel < LogLevel::INFO) return;
	ezprint(fmt);	
}


template<class logger_traits>
inline void Log<logger_traits>::print (const char* fmt, ...)
{
	if (logger_traits::logLevel < LogLevel::PRINT) return;
	ezprint(fmt);
}


template<class logger_traits>
inline void Log<logger_traits>::warning (const char* fmt, ...)
{
	if (logger_traits::logLevel < LogLevel::WARNING) return;
	ezprint(fmt);	
}


template<class logger_traits>
inline void Log<logger_traits>::error (const char* fmt, ...)
{
	if (logger_traits::logLevel < LogLevel::ERROR) return;
	ezprint(fmt);	
}


template<class logger_traits>
inline void Log<logger_traits>::critical (const char* fmt, ...)
{
	if (logger_traits::logLevel < LogLevel::CRITICAL) return;
	ezprint(fmt);	
}



// Our core application logger, a singleton.
typedef Log<> log;
