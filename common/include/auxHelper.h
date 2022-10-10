#pragma once

#include "defs.h"
#include "commonHead.h"
#include <string>
#include <vector>

class auxHelper
{
public:
  static inline void generate_random(u_char *buffer, u_int length)
  {
    memset(buffer, 0x8, length);
#if 0 
    FILE *fp;
    fp = fopen("/dev/urandom", "rb");
    if (fp) {
      fread(buffer, length, 1, fp);
      fclose(fp);
    }
#endif
    srand(time(0));
    for (u_int i = 0; i < length; ++i)
    {
      u_char x, s;
      s = rand() % 3;
      if (1 == s)
        x = rand() % ('Z' - 'A' + 1) + 'A';
      else if (2 == s)
        x = rand() % ('z' - 'a' + 1) + 'a';
      else 
        x = rand() % ('9' - '0' + 1) + '0';
      buffer[i] = x;
    }
  }

  static inline void strTokenize(const std::string &str, std::vector<std::string> &tokens,
                                 const std::string &delimiters)
  {
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    std::string::size_type pos = str.find_first_of(delimiters, lastPos);
    while (std::string::npos != pos || std::string::npos != lastPos)
    {
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      lastPos = str.find_first_not_of(delimiters, pos);
      pos = str.find_first_of(delimiters, lastPos);
    }
  }
  static inline std::string strJoin(const std::string &delimiters, const u_int count, ...)
  {
    std::string rtString = "";
    va_list argptr;
    va_start(argptr, count);
    for (u_int i = 0; i < count; ++i)
    {
      char *str = va_arg(argptr, char *);
      if (NULL == str)
        continue;
      if (0 == i)
        rtString = std::string(str);
      else
        rtString += (delimiters + std::string(str));
    }
    va_end(argptr);
    return rtString;
  }

  static inline u_int GetTickCount()
  {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
      return 0;
    return (u_int32_t)(tv.tv_sec * 1000) + (tv.tv_usec / 1000);
  }

  static inline u_int get_time_diff(time_t tm)
  {
    return (u_int)(time(NULL) - tm);
  }

  static inline void dump(const char *title, u_char *data, u_int len)
  {
    char ascline[20];
    int blankcount = 50;

    printf("Memory Dump => %s [%d]: \n", title, len);
    memset(ascline, 0, 20);
    for (int i = 0; i < len; ++i)
    {
      printf("%02x ", data[i]);
      if (isalnum(data[i]))
        sprintf(ascline, "%s%c", ascline, data[i]);
      else
        sprintf(ascline, "%s.", ascline);
      blankcount -= 3;
      if (0 != i && 0 == (i + 1) % 16)
      {
        for (int j = 0; j < blankcount; ++j)
          printf(" ");
        printf("%s\n", ascline);
        memset(ascline, 0, 20);
        blankcount = 50;
      }
    }
    if (50 != blankcount)
    {
      for (int j = 0; j < blankcount; ++j)
        printf(" ");
      printf("%s\n", ascline);
    }
  }

  static inline std::string der_to_pem(const std::string &der_data)
  {
    std::string pem_data = "";

    if (0 == der_data.length()) return pem_data;

    u_int buf_len = der_data.length() + 1;
    u_char *pbuf = new u_char[buf_len];
    if (NULL == pbuf) return pem_data;

    auxHelper::hexstring2Byte(der_data.c_str(), pbuf, buf_len);

    pem_data.assign((const char *)pbuf, buf_len);
    
    return pem_data;
  }

  static inline std::string pem_to_der(const std::string &pem_data)
  {
    return auxHelper::byte2hexstring((const u_char *)pem_data.c_str(), pem_data.length());
  }

  static inline bool hexstring2Byte(const std::string &str, u_char *byte, u_int &len)
  {
    if (str.length() / 2 > len)
      return false;

    char dst[5] = {0};
    char *psrc = (char *)str.c_str();

    len = 0;
    for (int i = 0; *psrc; ++i) {
      strcpy(dst, "0x");
      strncat(dst, psrc, 2);
      byte[i] = strtol(dst, NULL, 16);
      psrc += 2;
      //printf("%02X ",byte[i]);
      ++len;
    }
    return true;
  }

  static inline std::string byte2hexstring(const u_char *byte, u_int len)
  {
    std::string strResult = "";

    if (NULL == byte || 0 == len) return strResult;
   
    u_int buf_len = len * 2 + 1;
    char *pbuf = new char[buf_len];
    if (NULL == pbuf) return strResult;
    
    bzero(pbuf, buf_len);
    for (int i = 0; i < len; ++i)
      sprintf(pbuf + i * 2, "%02x", byte[i]);
    
    pbuf[buf_len] = 0;
    strResult = pbuf;
    
    delete[] pbuf;
    
    return strResult;
  }

  static void exec_cmd(const char *cmd)
  {
#if !defined(_NO_SYSTEM_SYSCALL)
    for (int i = 0; i < 2; ++i)
    {
      if (0 == system(cmd))
        break;
      sleep(1);
    }
#endif
  }

  static int System(const char *fmt, ...)
  {
    int32_t nret = 0;
#if !defined(_NO_SYSTEM_SYSCALL)
    char szcmd[1024];
    va_list ap;

    bzero(szcmd, 1024);

    va_start(ap, fmt);
    vsnprintf(szcmd, 1023, fmt, ap);
    va_end(ap);

    nret = system(szcmd);
#endif
    return (nret);
  }

  static inline void wait_time_msec(u_int msecs) { usleep(msecs * 1000); }
  static bool sleep_time_with_cancle(u_int seconds, bool *pcancle_flag)
  {
    u_int total = seconds * 2; // 1s = 500 * 2 ms
    u_int count = 0;

    do {
      count++;
      auxHelper::wait_time_msec(500);
    } while ((false == (*pcancle_flag)) && (count < total));
    
    return !(*pcancle_flag);
  }

  static bool isStringContain(const std::string &ids, const std::string &id)
  {
    bool bfind = false;
    std::vector<std::string> vids;
    splitString2vector(ids, vids);

    for (size_t i = 0; i < vids.size(); ++i)
    {
      if (id == vids[i])
      {
        bfind = true;
        break;
      }
    }

    return bfind;
  }

  static void splitString2vector(const std::string &id, std::vector<std::string> &id_verctor)
  {
    size_t ipos = 0;
    std::string dst_id = id;

    if (dst_id.empty())
      return;

    id_verctor.clear();

    do
    {
      ipos = dst_id.find(",");
      if (ipos == std::string::npos)
      {
        id_verctor.push_back(dst_id);
        break;
      }
      else
      {
        std::string temp_id = "";
        temp_id.append(dst_id.begin(), dst_id.begin() + ipos);
        if (!temp_id.empty())
          id_verctor.push_back(temp_id);
        dst_id = dst_id.substr(ipos + 1, dst_id.length());
      }
    } while (true);
  }

  static void remove_sn_id_from_ids(std::string &ids, const std::string &id)
  {
    std::vector<std::string> vSnid;
    splitString2vector(ids, vSnid);

    for (size_t i = 0; i < vSnid.size(); ++i)
    {
      if (id == vSnid[i])
      {
        vSnid[i] = "";
        break;
      }
    }
    vectorId2StringId(ids, vSnid);
  }

  static void vectorId2StringId(std::string &snid, const std::vector<std::string> &sn_id_verctor)
  {
    snid = "";
    for (size_t i = 0; i < sn_id_verctor.size(); ++i)
    {
      if (sn_id_verctor[i].empty())
        continue;
      if (snid.empty())
        snid = sn_id_verctor[i];
      else
      {
        snid += ",";
        snid += sn_id_verctor[i];
      }
    }
  }
  
  //num  [t, d)  t < d
  static int get_rand_num(int t, int d)
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand((u_int)(tv.tv_sec * 1000 + tv.tv_usec / 1000)); // milli time
    int min = (t < d) ? t : d;
    int max = (t < d) ? d : t;
    int c = max - min;
    if (0 == c) c = 1;
    return ((rand() % c) + min);
  }

  static std::string getFileName(std::string filePath, bool withExtension = true, char seperator = '/')
  {
    // Get last dot position
    std::size_t dotPos = filePath.rfind('.');
    std::size_t sepPos = filePath.rfind(seperator);
    if(sepPos != std::string::npos)
        return filePath.substr(sepPos + 1, filePath.size() - (withExtension || dotPos != std::string::npos ? 1 : dotPos) );
    return "";
  }

  static std::string get_process_name_by_pid(int pid)
  {
    char name[256] = {0};
    char cmdline[1024] = {0};

    sprintf(cmdline, "/proc/%d/cmdline", pid);
    FILE* fp = fopen(cmdline,"rb");
    if(fp) {
      fread(name, 256, 1, fp);
      fclose(fp);
    }
    return getFileName(name);
  }

  static std::string get_time_hour_str(time_t ts) {
    char timeBuf[10] = {0};
    struct tm *loctime = localtime(&ts);
    strftime (timeBuf, 10, "%H", loctime);
    return std::string(timeBuf);
  }

  static std::string get_time_str(time_t ts) {
    char timeBuf[30] = {0};
    struct tm *loctime = localtime(&ts);
    strftime (timeBuf, 30, "%Y-%m-%d %H:%M:%S", loctime);
    return std::string(timeBuf);
  }

  static bool read_from_file(const std::string &file_name, std::string &data)
  {
    FILE *fp = NULL;
    char *enc_buf = NULL;
    int length = 0, nFileLen = 0;
    
    data.clear();
    fp = fopen(file_name.c_str(), "rb");
    if (fp) {
      //file size
      fseek(fp, 0, SEEK_END);
      nFileLen = (int) ftell(fp);
      fseek(fp, 0, SEEK_SET);
      
      enc_buf = new char[nFileLen + 1];
      if (NULL != enc_buf) {
        bzero(enc_buf, nFileLen + 1);
        length = (int) fread(enc_buf, 1, nFileLen, fp);
        if (length > 0) data.assign(enc_buf, length);
      }
      fclose(fp);
      if (NULL != enc_buf) delete[] enc_buf;
    }

    return (false == data.empty());
  }

  static bool save_to_file(const std::string &file_name, const std::string &data)
  {
    bool rtCode = true;

    if (file_name.empty() || data.empty()) return false;

    FILE *fp = fopen(file_name.c_str(), "wt");
    if (fp) {
      size_t bytes = fwrite((const void *) data.c_str(), data.size(), 1, fp);
      if (bytes <= 0) {
        auxHelper::wait_time_msec(200);
        bytes = fwrite((const void *) data.c_str(), data.size(), 1, fp);
        if (bytes <= 0) rtCode = false;
      }
      fclose(fp);
    } else rtCode = false;

    return rtCode;
  }

  static std::string strToDecimalStr(const std::string &numer_str)
  {
    int ipos_0x = 0, ipos_0X = 0;
    std::string result_number_str = "";
    u_int64_t result_number =  0;

    do {
      ipos_0x= numer_str.find("0x");
      ipos_0X= numer_str.find("0X");
      if (std::string::npos != ipos_0x ||  std::string::npos != ipos_0X) {
        result_number = strtoll(numer_str.c_str(), NULL, 16);

        char szBuffer[128] = {0};
        sprintf(szBuffer, "%llu", result_number);
        result_number_str = std::string(szBuffer);

      } else result_number_str = numer_str;

    } while (false);
    
    return result_number_str;
  }

  static std::string intBytesToGBStr(u_int64_t int_bytes)
  {
    u_int64_t volume_GB = int_bytes / ( 1024 * 1024 * 1024LL);
    char szBytes[128] = {0};
    sprintf(szBytes, "%llu", volume_GB);
    return std::string(szBytes);  
  }
};

template <class T>
class smart_ptr
{
public:
  smart_ptr(T *ptr, bool array = true) : _object(ptr), _array(array)
  {
  }
  ~smart_ptr()
  {
    if (true == _array)
      delete[] _object;
    else
      delete _object;
  }
  T *get() { return _object; }

private:
  T *_object;
  bool _array;
};
