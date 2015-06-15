#include "logging.h"
#include <unistd.h>
#include <stdarg.h> 

static L3LoggerPtr _global_logger;

L3LoggerPtr
GetRootLogger()
{
	if (!_global_logger)
		_global_logger = L3LoggerPtr(new L3PrintfLogger);

	return _global_logger;
}

void
SetRootLogger(L3LoggerPtr logger)
{

        _global_logger = logger;
}

std::string
L3LoggingStringF(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	int messagesize = vsnprintf(NULL, 0, format, args);
	char log_message[messagesize + 1];

	va_start(args, format);
	vsprintf(log_message, format, args);

	return std::string(log_message);
}

L3Logger::L3Logger(L3LogLevel default_level) :
    default_log_level_(default_level) {}
L3Logger::~L3Logger() {}

L3LogLevel
L3Logger::LogLevelForUnit(const std::string &unit)
{
	std::map<std::string, L3LogLevel>::const_iterator iter =
	    log_levels_.find(unit);
	if (iter == log_levels_.end())
		return default_log_level_;

	return iter->second;
}

void
L3Logger::SetLogLevelForUnit(const std::string &unit, L3LogLevel level)
{
	log_levels_[unit] = level;
}

void
L3Logger::SetLogLevel(L3LogLevel level)
{
	default_log_level_ = level;
}

L3BasicLogger::L3BasicLogger(L3LogLevel level)
    : L3Logger(level) {}

void
L3BasicLogger::Log(L3LogLevel level, const std::string &unit,
    const std::string &file, int line, const std::string &func,
    const std::string &message)
{
	const char *log_description;

	if (LogLevelForUnit(unit) > level)
		return;

	switch (level) {
	case L3LOG_TRACE:
		log_description = "TRACE";
		break;
	case L3LOG_DEBUG:
		log_description = "DEBUG";
		break;
	case L3LOG_INFO:
		log_description = "INFO";
		break;
        case L3LOG_NOTICE:
                log_description = "NOTICE";
                break;
	case L3LOG_WARN:
		log_description = "WARN";
		break;
	case L3LOG_ERROR:
		log_description = "ERROR";
		break;
	case L3LOG_FATAL:
		log_description = "FATAL";
		break;
	default:
		log_description = "UNKNOWN";
		break;
	}

	int messagesize = snprintf(NULL, 0, "%s (%s): %s (%s:%d in %s)",
	    log_description, unit.c_str(), message.c_str(), file.c_str(), line,
	    func.c_str());
	char log_message[messagesize + 1];
	sprintf(log_message, "%s (%s): %s (%s:%d in %s)", log_description,
	    unit.c_str(), message.c_str(), file.c_str(), line, func.c_str());

	BasicLog(log_message);
}

void
l3_clogger(L3LogLevel level, const char *unit, const char *file, int line,
    const char *func, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	int messagesize = vsnprintf(NULL, 0, format, args);
	char log_message[messagesize + 1];

	va_start(args, format);
	vsprintf(log_message, format, args);

	GetRootLogger()->Log(level, unit, file, line, func, log_message);
}






L3PrintfLogger::L3PrintfLogger(L3LogLevel level)
    : L3Logger(level), TrimFileNames(true)
{
	tty_ = isatty(STDERR_FILENO);
}

void
L3PrintfLogger::Log(L3LogLevel level, const std::string &unit,
    const std::string &file, int line, const std::string &func,
    const std::string &message)
{
	const char *log_description;
	const char *log_prolog = "", *file_prolog = "", *log_epilog = "";

	if (LogLevelForUnit(unit) > level)
		return;

	if (tty_) {
		log_prolog = "\x1b[1m";
		file_prolog = "\x1b[1m";
		log_epilog = "\x1b[0m";
	}

	switch (level) {
	case L3LOG_TRACE:
		log_description = "TRACE";
		break;
	case L3LOG_DEBUG:
		log_description = "DEBUG";
		break;
	case L3LOG_INFO:
		log_description = "INFO";
		break;
        case L3LOG_NOTICE:
                log_description = "NOTICE";
                break;
	case L3LOG_WARN:
		log_description = "WARN";
		break;
	case L3LOG_ERROR:
		log_description = "ERROR";
		if (tty_)
			log_prolog = "\x1b[1;31m";
		break;
	case L3LOG_FATAL:
		log_description = "FATAL";
		if (tty_)
			log_prolog = "\x1b[1;31m";
		break;
	default:
		log_description = "UNKNOWN";
		break;
	}

	std::string trimmed_filename;
	size_t lastslash = file.rfind('/');
	if (lastslash != std::string::npos && TrimFileNames)
		trimmed_filename = file.substr(lastslash+1);
	else
		trimmed_filename = file;

	int messagesize = snprintf(NULL, 0,
	    "%s%s (%s):%s %s (%s%s:%d%s in %s%s%s)\n",
	    log_prolog, log_description, unit.c_str(), log_epilog,
	    message.c_str(), file_prolog, trimmed_filename.c_str(), line,
	    log_epilog, file_prolog, func.c_str(), log_epilog);

	char log_message[messagesize + 1];

	sprintf(log_message, "%s%s (%s):%s %s (%s%s:%d%s in %s%s%s)\n",
	    log_prolog, log_description, unit.c_str(), log_epilog,
	    message.c_str(), file_prolog, trimmed_filename.c_str(), line,
	    log_epilog, file_prolog, func.c_str(), log_epilog);

	fputs(log_message, stderr);
}

