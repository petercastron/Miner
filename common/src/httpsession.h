#ifndef _HTTPSESSION_H
#define _HTTPSESSION_H

#include "commonHead.h"
#include <string>
#ifndef NOTUSING_CURL
#include <curl/curl.h>

typedef int (*HTTP_DATA_ARRIVAL) (void *context, u_char *data, size_t size);

class httpsession
{
public:
  httpsession();
  virtual ~httpsession();
  static void global_init ()
  {
    curl_global_init (CURL_GLOBAL_DEFAULT);
  }
  static void global_cleanup ()
  {
    curl_global_cleanup ();
  }

  bool init (HTTP_DATA_ARRIVAL hda, void *pContext, u_int connect_timeout = 5, u_int opt_timeout = 30);
  bool init_content_type (HTTP_DATA_ARRIVAL hda, void *pContext, std::string content_type, u_int connect_timeout = 5, u_int opt_timeout = 30);

  void set_callback (HTTP_DATA_ARRIVAL hda)
  {
    m_data_arrival_callback = hda;
  }

  void set_callback_context (void *context)
  {
    m_dac_context = context;
  }
  void set_agent (std::string agent)
  {
    m_user_agent = agent;
  }
  std::string get_content ()
  {
    return m_content;
  }
  std::string get_header ()
  {
    return m_content_header;
  }
  void set_proxy (std::string proxy, long port, long type)
  {
    // type:
    // 0: HTTP
    // 1: HTTP_1_0
    // 4: SOCKS4
    // 5: SOCKS5
    // 6: SOCKS4A
    // 7: SOCKS5_HOSTNAME
    m_proxy = proxy;
    m_proxy_port = port;
    m_proxy_type = (curl_proxytype) type;
  }
  bool is_extern_writer ()
  {
    return (m_extern_writer);
  }

  bool get (std::string url, bool is_file = false, unsigned long resume_pos = 0, const std::string ca_info = "");
  bool post (std::string url, std::string post_data, bool is_file = false, const std::string ca_info = "");
  static int writer(char *data, size_t size, size_t nmemb, void *writerData);
  static int writer_header(char *data, size_t size, size_t nmemb, std::string *writerData);
  std::string urlencode (const std::string url, std::string &encode_url);
  bool get_remote_file_length (std::string url, unsigned long &length);
  void get_download_speed_info(double& speed, double& total_time);
  HTTP_DATA_ARRIVAL m_data_arrival_callback;
  void *m_dac_context;

private:
  CURL *m_curl_handle;
  std::string m_user_agent;
  std::string m_encoding;
  std::string m_content;
  std::string m_content_header;
  std::string m_proxy;
  long   m_proxy_port;
  curl_proxytype m_proxy_type;
  struct curl_slist *m_header;
  bool   m_extern_writer;
};
#endif
#endif //NOTUSING_CURL
