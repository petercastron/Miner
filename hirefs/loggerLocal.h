#if !defined(__LOGGERIO_LOCAL_QA_H)
#define __LOGGERIO_LOCAL_QA_H

#include "logger.h"
#include <sys/syslog.h>

class loggerIOlocal : public loggerIO
{
public:
  loggerIOlocal()
  { }
  ~loggerIOlocal()
  { }

  virtual bool open()
  { return true; }

  virtual void close()
  { }

  virtual void write(LOG_LEVEL level, LOG_MODULE_INDEX index, char *header, char *content)
  {
    char logBuf_prefix[256] = {0};
    syslog(LOG_INFO, "{%s} %s/%s(%s): %s", "HIREFS", get_strDebugLevel(level).c_str(), header, get_strModeName(index).c_str(), content);
  }
};

#endif //__LOGGERIO_LOCAL_QA_H
