#if !defined(__LOGGER_H)
#define __LOGGER_H

#include "defs.h"
#include "commonHead.h"
#include "singleton.h"
#include <string>
#include <cstdio>
#include <ctype.h>

typedef enum
{
  LL_NONE = 0,
  LL_ASSERT,
  LL_ERROR,
  LL_WARN,
  LL_INFO,
  LL_DEBUG,
  LL_OBSERVE,
  LL_VERBOSE
} LOG_LEVEL;

typedef enum
{
  LOG_MODULE_INDEX_MAX = 0xffffffff,
  LOG_MODULE_INDEX_HIRE = 0,       //0   0x1
  LOG_MODULE_INDEX_COMMUNI,       //1   0x2
  LOG_MODULE_INDEX_SOCKET,        //2   0x4
  LOG_MODULE_INDEX_OTHER,         //3   0x8
} LOG_MODULE_INDEX;

# define COMET_DEBUG_FLAG(x)             (1 << x) 
# define COMET_DEBUG_FLAG_NONE            0x00
# define COMET_DEBUG_FLAG_ALL             0xFFFFFFFFFFFFFFFF

typedef enum
{
  LO_TIME = 1,
  LO_FILE = 2,
  LO_LINE = 4,
} LOG_OPTION;

class loggerIO
{
public:
  loggerIO()
  {
    _block_end = 0;
    _logfile_handle = -1;
    memset(_log_working_directory, 0, sizeof(_log_working_directory));
    memset(_log_prefix, 0, sizeof(_log_prefix));
    // default values
    strcpy (_log_working_directory, "/tmp/hirefs");
    strcpy (_log_prefix, "hirefs");
    bzero(_filename, 256);
  }

  virtual ~loggerIO() { close(); }

  void set_working_directory(const char *directory)
  {
    if (0 == strcmp (directory, _log_working_directory)) return;
    if (-1 != _logfile_handle) { close (); _logfile_handle = -1; }
    memset(_log_working_directory, 0, sizeof(_log_working_directory));
    strncpy(_log_working_directory, directory, sizeof(_log_working_directory));
  }
  
  void set_prefix(const char *prefix)
  {
    if (0 == strcmp (prefix, _log_prefix)) return;
    if (-1 != _logfile_handle) { close (); _logfile_handle = -1; }
    memset(_log_prefix, 0, sizeof(_log_prefix));
    strncpy(_log_prefix, prefix, sizeof(_log_prefix));
  }

  std::string get_strDebugLevel(LOG_LEVEL level)
  {
    std::string strDebugLevel = "";
    if (LL_ASSERT == level) strDebugLevel = "A";
    else if (LL_ERROR == level) strDebugLevel = "E";
    else if (LL_WARN == level) strDebugLevel = "W";
    else if (LL_INFO == level) strDebugLevel = "I";
    else if (LL_DEBUG == level) strDebugLevel = "D";
    else if (LL_OBSERVE == level) strDebugLevel = "O";
    else if (LL_VERBOSE == level) strDebugLevel = "V";
    else strDebugLevel = "N";
    return strDebugLevel;
  }

  std::string get_strModeName(LOG_MODULE_INDEX index)
  {
    std::string strModeName;
    if (LOG_MODULE_INDEX_HIRE == index) strModeName = "HIRE";
    else if (LOG_MODULE_INDEX_COMMUNI == index) strModeName = "COMMUNI";
    else if (LOG_MODULE_INDEX_SOCKET == index) strModeName = "SOCK";
    else strModeName = "OTHER";
    return strModeName;
  }

  virtual bool open()
  {
#define FILE_SIZE_MAX  200 * 1024 * 1024
    time_t now;
    struct tm loctime, endtime, pretime;
    struct stat st;

    do
    {
      now = time(NULL);
      if (-1 == _logfile_handle) break;
      
      bzero(&st, sizeof(st));
      if (0 != fstat(_logfile_handle, &st)) break;

      if (FILE_SIZE_MAX < st.st_size) close(true);
      
      if (now > _block_end) close(true);

    } while (false);

    if (-1 != _logfile_handle) return true;

    // open new log file
    memset(&endtime, 0, sizeof endtime);
    localtime_r(&now, &loctime);
    bzero(_filename, 256);
    sprintf(_filename, "%s/%s-%d-%d.log", _log_working_directory, _log_prefix, loctime.tm_year + 1900, loctime.tm_mon + 1);
    if (11 == loctime.tm_mon) {
      endtime.tm_mon = 0;
      endtime.tm_year = loctime.tm_year + 1;
    } else {
      endtime.tm_mon = loctime.tm_mon + 1;
      endtime.tm_year = loctime.tm_year;
    }
    endtime.tm_mday = 1;
    _block_end = mktime(&endtime) - 1;

    // create the logger directory
    bzero(&st, sizeof(st));
    if (0 != stat(_log_working_directory, &st))
      mkdir(_log_working_directory, S_IWUSR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
      
    return (-1 !=
            (_logfile_handle = ::open(_filename, O_WRONLY | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)));
  }

  virtual void close(bool bremove = false)
  {
    if (-1 != _logfile_handle) {
      ::close(_logfile_handle);
      _logfile_handle = -1;

      if (bremove) {
        if (0 < strlen(_filename)) unlink(_filename);
      }
    }
  }

  virtual void write(LOG_LEVEL level, LOG_MODULE_INDEX index, char *header, char *content)
  {
    char logBuf_prefix[256] = {0};
    sprintf(logBuf_prefix, "%s/%s(%s) ", get_strDebugLevel(level).c_str(), header, get_strModeName(index).c_str());
    ::write(_logfile_handle, logBuf_prefix, strlen(logBuf_prefix));
    ::write(_logfile_handle, content, strlen(content));
    ::write(_logfile_handle, "\n", 1);
    ::fsync(_logfile_handle);
  }

private:
  int _logfile_handle;
  time_t _block_end;
  char _log_working_directory[256];
  char _log_prefix[256];
  char _filename[256];
};

class logOutPutBase
{
public:
  virtual void add_log(const std::string &log) = 0;
  virtual bool is_start() = 0;
};

class logger : public singleton<logger>
{

public:
  logger()
  {
    _write_to_file = true;
    _log_level = LL_NONE;
    _loggerIO = &_defaultIO;
    _event_log_enabled = false;
    memset(_log_working_directory, 0, sizeof(_log_working_directory));
    memset(_log_prefix, 0, sizeof(_log_prefix));
    _log_option = (LO_TIME | LO_FILE | LO_LINE);
    _plc = NULL;
    _explog_obj = NULL;
    _init_locker_logger();
  }

  ~logger()
  {
    _loggerIO->close();
    _destroy_locker_logger();
  }
  void set_working_directory(const char *directory)
  {
    memset(_log_working_directory, 0, sizeof(_log_working_directory));
    strncpy(_log_working_directory, directory, sizeof(_log_working_directory));
    _loggerIO->set_working_directory(directory);
  }
  void set_prefix(const char *prefix)
  {
    memset(_log_prefix, 0, sizeof(_log_prefix));
    strncpy(_log_prefix, prefix, sizeof(_log_prefix));
    _loggerIO->set_prefix(prefix);
  }
  bool getDebugModel() 
  { return _debug_model; }
  void setDebugModel(bool debug_model = true)
  { _debug_model = debug_model; }
  bool isDebugModel()
  { return _debug_model; }
  void set_write_log_file(bool write_to_file)
  { _write_to_file = write_to_file; }

  LOG_LEVEL getLevel(){ return _log_level;}
  void setLevel(LOG_LEVEL level, u_int idx = LOG_MODULE_INDEX_MAX)
  {
    if (level > LL_VERBOSE) _log_level = LL_VERBOSE;
    else _log_level = level;
    
    _log_index = idx;
    
    if (LL_NONE == level)
      _loggerIO->close();
  }
  void set_IOObj(loggerIO *obj)
  {
    _loggerIO = obj;
    _loggerIO->set_working_directory(_log_working_directory);
    _loggerIO->set_prefix(_log_prefix);
  }
  loggerIO *get_IOObj()
  { return _loggerIO; }

  void disable_log_option(LOG_OPTION lo)
  {
    _log_option &= ~lo;
  }

  inline bool isOutPut(LOG_LEVEL level, LOG_MODULE_INDEX idx)
  {
    if (level > _log_level || LL_NONE == level || false == _loggerIO->open()) return false;
    else {
      u_int flag = COMET_DEBUG_FLAG (idx);
      if (((_log_index & flag) != flag) && (LOG_MODULE_INDEX_MAX != _log_index))
        return false;
      else return true;
    }
  }

  void set_omc_obj(logOutPutBase *pomc) { _pomc = pomc; }
  void set_log_channel_obj(logOutPutBase *plc) { _plc = plc; }
  void set_explog_obj (logOutPutBase *obj) { _explog_obj = obj; }
  void outDebugLog(const char *function_name, const char *filename, int line, LOG_LEVEL level, LOG_MODULE_INDEX index,
                   const char *format,
                   ...)
  {
#define LOG_BUFFER_LEN_KB    30
    va_list argptr;
    char logBuf[1024 * LOG_BUFFER_LEN_KB] = {0}, logBuf_prefix[256] = {0};
    char timeBuf[30] = {0}, timeBuf_ms[40] = {0};
    const char *filename_ptr;
    struct tm *loctime;

    struct timeval now;

    gettimeofday(&now, NULL);
    loctime = localtime(&now.tv_sec);
    strftime(timeBuf, 30, "%Y/%m/%d %H:%M:%S", loctime);
    sprintf(timeBuf_ms, "[%s:%06d]", timeBuf, (int) now.tv_usec);

    filename_ptr = strrchr(filename, '/');
    if (filename_ptr) filename_ptr++;
    else filename_ptr = filename;

    do {
      va_start(argptr, format);
      vsnprintf(logBuf, 1024 * LOG_BUFFER_LEN_KB, format, argptr);
      va_end(argptr);
      sprintf(logBuf_prefix, "%s(%s:%d): ", (LO_TIME == (_log_option & LO_TIME)) ? timeBuf_ms : "", filename_ptr, line);

      lock_logger();
      if(_write_to_file)
        _loggerIO->write(level, index, logBuf_prefix, logBuf);
      unlock_logger();

    } while (false);
  }

  void outDebugDump(const char *function_name, const char *filename, int line, LOG_LEVEL level, LOG_MODULE_INDEX index,
                    const char *caption,
                    const u_char *buffer, const int len)
  {
    if (level > _log_level || len <= 0) return;
    char hexline[50], ascline[20], blanks[50];
    int blankcount = 50;
    int total_len = (int) strlen(caption) + len * 5 + 200;
    char *lines = new char[total_len];
    if (NULL == lines) return;
    memset(lines, 0, total_len);
    memset(blanks, 0x20, 50);
    memset(hexline, 0, 50);
    memset(ascline, 0, 20);
    sprintf(lines, "%s [%d]\n", caption, len);
    for (int i = 0; i < len; ++i) {
      sprintf(lines, "%s%02x ", lines, buffer[i]);
      if (isalnum(buffer[i])) sprintf(ascline, "%s%c", ascline, buffer[i]);
      else sprintf(ascline, "%s.", ascline);
      blankcount -= 3;
      if (0 != i && 0 == (i + 1) % 16) {
        strncat(lines, blanks, blankcount);
        strcat(lines, ascline);
        strcat(lines, "\n");
        memset(hexline, 0, 50);
        memset(ascline, 0, 20);
        blankcount = 50;
      }
    }
    if (50 != blankcount) {
      strncat(lines, blanks, blankcount);
      strcat(lines, ascline);
      strcat(lines, "\n");
    }
    logger::instance().outDebugLog(function_name, filename, line, level, index, lines);
    delete[] lines;
  }

  void outDebugDump2(const char *function_name, const char *filename, int line, LOG_LEVEL level, LOG_MODULE_INDEX index,
                     const char *caption,
                     const u_char *buffer, const int siz)
  {
    if (level > _log_level || siz <= 0) return;
    std::string str = "";
    int len;
    char c;
    char hexline[50], ascline[20], blanks[50];
    int total_len = (int) strlen(caption) + siz * 10 + 200;
    char *lines = new char[total_len];
    if (NULL == lines) return;
    memset(lines, 0, total_len);
    memset(blanks, 0x20, 50);
    memset(hexline, 0, 50);
    memset(ascline, 0, 20);
    sprintf(lines, "%s [%d]\n", caption, siz);


    for (int i = 0; i < siz; i += 16) {
      char msg[512] = {0};
      len = siz - i;
      if (len > 16)
        len = 16;
      sprintf(msg, "%08x ", i);
      str += msg;
      for (int j = 0; j < 16; j++) {
        if (j < len) {
          sprintf(msg, " %02x", buffer[i + j]);
          str += msg;
        } else {
          str += "   ";
        }
      }
      str += " ";
      for (int j = 0; j < len; j++) {
        c = buffer[i + j];
        if (c < ' ' || c > '~') c = '.';
        sprintf(msg, "%c", c);
        str += msg;
      }
      str += "\n";
    }
    str += "\n";

    sprintf(lines, "%s%s", lines, str.c_str());
    logger::instance().outDebugLog(function_name, filename, line, level, index, lines);
    delete[] lines;
  }

LOCK_V(logger);

private:
  bool _write_to_file;
  logOutPutBase * _pomc;
  logOutPutBase * _explog_obj;
  logOutPutBase* _plc;
  u_int _log_index;
  LOG_LEVEL _log_level;
  bool _debug_model;
  bool _event_log_enabled;
  char _event_log_filename[256];
  u_int _log_option;
  char _log_working_directory[256];
  char _log_prefix[256];

  loggerIO *_loggerIO;
  loggerIO _defaultIO;
};

#define debugEntry(level, index, format, ...)  \
do { if( true == logger::instance().isOutPut(level, index) ) logger::instance().outDebugLog(__FUNCTION__,__FILE__,__LINE__,level,index,format,## __VA_ARGS__); } while (0)

#define debugDump(level, index, caption, buffer, len) \
do { if( true == logger::instance().isOutPut(level, index) ) logger::instance().outDebugDump(__FUNCTION__,__FILE__,__LINE__,level,index,caption,buffer,len); } while (0)

#define debugDump2(level, index, caption, buffer, len) \
do { if( true == logger::instance().isOutPut(level, index) ) logger::instance().outDebugDump2(__FUNCTION__,__FILE__,__LINE__,level,index,caption,buffer,len); } while (0)

#endif
