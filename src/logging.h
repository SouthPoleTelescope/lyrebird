#pragma once

/*
 * Logging framework: use log_info()/log_warn()/log_fatal() in your code
 *  instead of printf(). To have class-specific settable log levels, add
 *  SET_LOGGER("classname") somewhere under private:.
 *
 *  This code is 100% taken from bits of IceTray that Nwhitehorn wrote.
 */

#include <signal.h>
#include <memory>

#define L3_POINTERS(x) \
typedef std::shared_ptr<x> x##Ptr; \
typedef std::shared_ptr<const x> x##ConstPtr; \


typedef enum {
	L3LOG_TRACE,
	L3LOG_DEBUG,
	L3LOG_INFO,
	L3LOG_NOTICE,
	L3LOG_WARN,
	L3LOG_ERROR,
	L3LOG_FATAL
} L3LogLevel;

#if defined(__cplusplus)

#ifdef NDEBUG
const L3LogLevel L3DefaultLogLevel = L3LOG_NOTICE;
#else
const L3LogLevel L3DefaultLogLevel = L3LOG_INFO;
#endif

#include <stdexcept>
#include <map>
#include <sstream>

class L3Logger {
public:
	L3Logger(L3LogLevel default_level = L3DefaultLogLevel);
	virtual ~L3Logger();

	virtual void Log(L3LogLevel level, const std::string &unit,
	    const std::string &file, int line, const std::string &func,
	    const std::string &message) = 0;

	virtual L3LogLevel LogLevelForUnit(const std::string &unit);
	virtual void SetLogLevelForUnit(const std::string &unit,
	    L3LogLevel level);

	virtual void SetLogLevel(L3LogLevel level);
private:
	std::map<std::string, L3LogLevel> log_levels_;
	L3LogLevel default_log_level_;
};

class L3BasicLogger : public L3Logger {
public:
	L3BasicLogger(L3LogLevel default_level = L3DefaultLogLevel);

	virtual void Log(L3LogLevel level, const std::string &unit,
	    const std::string &file, int line, const std::string &func,
	    const std::string &message);
	virtual void BasicLog(const std::string &string) = 0;
};

L3_POINTERS(L3Logger);

// Root logger. If class/namespace has a method with the same name in scope,
// it can have its own logger.
L3LoggerPtr GetRootLogger();
void SetRootLogger(L3LoggerPtr);

std::string L3LoggingStringF(const char *format, ...)
    __attribute__((__format__ (__printf__, 1, 2)));
#define L3_LOGGER(level, id, file, line, func, format, ...) \
    GetRootLogger()->Log(level, id, file, line, func, \
    L3LoggingStringF(format, ##__VA_ARGS__))

#define L3_STREAM_LOGGER(level, id, file, line, func, msg, epilogue) \
    do { std::ostringstream _l3_str_logger_str; _l3_str_logger_str << msg; GetRootLogger()->Log(level, \
    id, file, line, func, _l3_str_logger_str.str()); epilogue } while (0)

extern "C" {
#endif // __cplusplus
void l3_clogger(L3LogLevel level, const char *unit, const char *file,
    int line, const char *func, const char *format, ...)
    __attribute__((__format__ (__printf__, 6, 7)));
#ifdef __cplusplus
}
#else
#define L3_LOGGER l3_clogger
#endif

#define SET_LOGGER(X) \
	static inline const char *__logger_id(void) { return X; }

// Set default logger in global namespace
#ifdef PROJECT
#warn "setting project"
#define L3LOGSTR1(X) #X
#define L3LOGSTR(X) L3LOGSTR1(X)
SET_LOGGER(L3LOGSTR(PROJECT));
#undef L3LOGSTR
#undef L3LOGSTR1
#else
SET_LOGGER("Unknown");
#endif

#define log_custom(level, format, ...) L3_LOGGER(level, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, format, \
    ##__VA_ARGS__)
#define log_custom_unit(level, unit, format, ...) L3_LOGGER(level, \
    unit, __FILE__, __LINE__, __PRETTY_FUNCTION__, format, \
    ##__VA_ARGS__)

#ifndef L3_COMPILE_OUT_VERBOSE_LOGGING
#define log_trace(format, ...) L3_LOGGER(L3LOG_TRACE, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, format, \
    ##__VA_ARGS__)
#define log_debug(format, ...) L3_LOGGER(L3LOG_DEBUG, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, format, \
    ##__VA_ARGS__)
#ifdef __cplusplus
#define log_trace_stream(msg) L3_STREAM_LOGGER(L3LOG_TRACE, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, msg, )
#define log_debug_stream(msg) L3_STREAM_LOGGER(L3LOG_DEBUG, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, msg, )
#endif
#else
#define log_trace(format, ...)
#define log_debug(format, ...)
#define log_trace_stream(msg)
#define log_debug_stream(msg)
#endif

#define log_info(format, ...) L3_LOGGER(L3LOG_INFO, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, format, \
    ##__VA_ARGS__)
#define log_notice(format, ...) L3_LOGGER(L3LOG_NOTICE, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, format, \
    ##__VA_ARGS__)
#define log_warn(format, ...) L3_LOGGER(L3LOG_WARN, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, format, \
    ##__VA_ARGS__)
#define log_error(format, ...) L3_LOGGER(L3LOG_ERROR, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, format, \
    ##__VA_ARGS__)

#ifdef __cplusplus
#define log_fatal(format, ...) L3_LOGGER(L3LOG_FATAL, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, format, \
    ##__VA_ARGS__), throw std::runtime_error(L3LoggingStringF(format, \
    ##__VA_ARGS__) + " (in " + __PRETTY_FUNCTION__ + ")")
#define log_info_stream(msg) L3_STREAM_LOGGER(L3LOG_INFO, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, msg, )
#define log_notice_stream(msg) L3_STREAM_LOGGER(L3LOG_NOTICE, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, msg, )
#define log_warn_stream(msg) L3_STREAM_LOGGER(L3LOG_WARN, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, msg, )
#define log_error_stream(msg) L3_STREAM_LOGGER(L3LOG_ERROR, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, msg, )
#define log_fatal_stream(msg) L3_STREAM_LOGGER(L3LOG_FATAL, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, msg, \
    throw std::runtime_error(_l3_str_logger_str.str() + " (in " + __PRETTY_FUNCTION__ + ")");)
#else
#define log_fatal(format, ...) L3_LOGGER(L3LOG_FATAL, \
    __logger_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__, format, \
    ##__VA_ARGS__), kill(getpid(), SIGABRT)
#endif

#define l3_assert(cond) do{ if(!(cond)) log_fatal("Assertion failed: %s", #cond); } while(0)
#ifdef NDEBUG
#define l3_debug_assert(cond) do{ /*nothing*/ } while(0)
#else
#define l3_debug_assert(cond) do{ if(!(cond)) log_fatal("Assertion failed: %s", #cond); } while(0)
#endif


class L3NullLogger : public L3Logger {
public:
	virtual void Log(L3LogLevel level, const std::string &unit,
	    const std::string &file, int line, const std::string &func,
	    const std::string &message) {};
};

class L3PrintfLogger : public L3Logger {
public:
	L3PrintfLogger(L3LogLevel default_level = L3DefaultLogLevel);
	virtual void Log(L3LogLevel level, const std::string &unit,
	    const std::string &file, int line, const std::string &func,
	    const std::string &message);

	bool TrimFileNames;
private:
	bool tty_;
};
