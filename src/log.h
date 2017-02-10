#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/loggingmacros.h>

namespace lei {

  using namespace log4cplus::helpers;
  using namespace log4cplus;

  extern Logger zLog;

  inline void InitLog(Logger &log, const char *name, const char *filename, LogLevel value) {
    SharedAppenderPtr append(new DailyRollingFileAppender(filename, HOURLY, true, 10));
    append->setName(name);
    std::string pattern = "%D [%p] [%l] %m%n";
    std::auto_ptr<Layout> layout(new PatternLayout(pattern));
    append->setLayout(layout);
    log.addAppender(append);
    log.setLogLevel(value);
  }

}

