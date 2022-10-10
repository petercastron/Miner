#include "httpsession.h"
#include "logger.h"

#ifndef NOTUSING_CURL
//class httpsession
httpsession::httpsession()
{
  m_curl_handle = NULL;
  m_user_agent = "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.1; Trident/4.0; SLCC2; .NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; Media Center PC 6.0; Tablet PC 2.0)";
  m_header = NULL;
  m_encoding = "gzip,deflate";
  m_extern_writer = false;
}

httpsession::~httpsession()
{
  if (NULL != m_curl_handle) {
    curl_easy_cleanup(m_curl_handle);
    m_curl_handle = NULL;
  }
  if (NULL != m_header) {
    curl_slist_free_all(m_header);
    m_header = NULL;
  }
}

bool httpsession::init_content_type (HTTP_DATA_ARRIVAL hda, void *pContext, std::string content_type, u_int connect_timeout, u_int opt_timeout)
{
  m_curl_handle = curl_easy_init();
  if (NULL == m_curl_handle) return (false);
  curl_easy_setopt(m_curl_handle, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(m_curl_handle, CURLOPT_HEADER, 0);
  curl_easy_setopt (m_curl_handle, CURLOPT_CONNECTTIMEOUT, connect_timeout);
  curl_easy_setopt(m_curl_handle, CURLOPT_TIMEOUT, opt_timeout);
  curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, httpsession::writer);
  curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(m_curl_handle, CURLOPT_HEADERFUNCTION, httpsession::writer_header);
  curl_easy_setopt(m_curl_handle, CURLOPT_WRITEHEADER, &m_content_header);
  curl_easy_setopt(m_curl_handle, CURLOPT_USERAGENT, m_user_agent.c_str());
  curl_easy_setopt(m_curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  m_header = curl_slist_append(m_header, "Connection: keep-alive");
  m_header = curl_slist_append(m_header, "Cache-Control: max-age=0");
  m_header = curl_slist_append(m_header,
                               "Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5");
  m_header = curl_slist_append(m_header, "Accept-Language: zh-CN,en-US;q=0.8,en;q=0.6");
  m_header = curl_slist_append(m_header, "Accept-Charset: GBK,utf-8;q=0.7,*;q=0.3");
  if (!content_type.empty()) m_header = curl_slist_append(m_header, content_type.c_str());
  curl_easy_setopt(m_curl_handle, CURLOPT_HTTPHEADER, m_header);
  curl_easy_setopt(m_curl_handle, CURLOPT_ENCODING, m_encoding.c_str());

  if (!m_proxy.empty() && m_proxy_port > 0) {
    curl_easy_setopt (m_curl_handle, CURLOPT_PROXY, m_proxy.c_str());
    curl_easy_setopt (m_curl_handle, CURLOPT_PROXYPORT, m_proxy_port);
    curl_easy_setopt (m_curl_handle, CURLOPT_PROXYTYPE, m_proxy_type);
  }
  m_data_arrival_callback = hda;
  m_dac_context = pContext;
  return (true);
}

bool httpsession::init(HTTP_DATA_ARRIVAL hda, void *pContext, u_int connect_timeout, u_int opt_timeout)
{
  return init_content_type(hda, pContext, "", connect_timeout, opt_timeout);
}

std::string httpsession::urlencode(const std::string url, std::string &encode_url)
{
  char *encoded = NULL;
  encode_url.clear();
  encoded = curl_easy_escape(m_curl_handle, url.c_str(), url.length());
  if (NULL != encoded) encode_url = encoded;
  curl_free(encoded);
  return encode_url;
}

bool httpsession::get(std::string url, bool is_file, unsigned long resume_pos, const std::string ca_info)
{
  CURLcode res;
  bool bReturn = true;
  long response_code;

  do {
    bReturn = true;
    curl_easy_setopt(m_curl_handle, CURLOPT_URL, url.c_str());

    curl_easy_setopt(m_curl_handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(m_curl_handle, CURLOPT_NOBODY, 0);
    curl_easy_setopt(m_curl_handle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(m_curl_handle, CURLOPT_POST, 0);
    curl_easy_setopt (m_curl_handle, CURLOPT_RESUME_FROM, resume_pos);

    if (0 == strncmp(url.c_str(), "https", strlen("https")) && !ca_info.empty()) {
      curl_easy_setopt(m_curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
      curl_easy_setopt(m_curl_handle, CURLOPT_SSL_VERIFYHOST, 1L);
      curl_easy_setopt(m_curl_handle, CURLOPT_CAINFO, ca_info.c_str());
    } else {
      curl_easy_setopt(m_curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(m_curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    m_content.clear();
    m_content_header.clear();
    m_extern_writer = is_file;
    res = curl_easy_perform(m_curl_handle);
    if (CURLE_OK != res && CURLE_PARTIAL_FILE != res) {
      debugEntry (LL_WARN, LOG_MODULE_INDEX_OTHER, "get %s(%s) fail, the error code is %d.", url.c_str(), ca_info.c_str(), res);
      return (false);
    }
    res = curl_easy_getinfo(m_curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    if (CURLE_OK != res || (200 != response_code && 206 != response_code)) {
      debugEntry (LL_WARN, LOG_MODULE_INDEX_OTHER, "server response %d when get %s", response_code, url.c_str());
      return (false);
    }
  } while (false == bReturn);
  debugEntry (LL_OBSERVE, LOG_MODULE_INDEX_OTHER, "get %s succ.", url.c_str());
  return (true);
}

bool httpsession::post(std::string url, std::string post_data, bool is_file, std::string ca_info)
{
  CURLcode res;

  curl_easy_setopt(m_curl_handle, CURLOPT_URL, url.c_str());

  curl_easy_setopt(m_curl_handle, CURLOPT_HEADER, 0);
  curl_easy_setopt(m_curl_handle, CURLOPT_NOBODY, 0);
  curl_easy_setopt(m_curl_handle, CURLOPT_HTTPGET, 0);
  curl_easy_setopt(m_curl_handle, CURLOPT_POST, 1);
  curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDSIZE, post_data.size());
  curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDS, post_data.c_str());

  if (0 == strncmp(url.c_str(), "https", strlen("https")) && !ca_info.empty()) {
    curl_easy_setopt(m_curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(m_curl_handle, CURLOPT_SSL_VERIFYHOST, 1L);
    curl_easy_setopt(m_curl_handle, CURLOPT_CAINFO, ca_info.c_str());
  } else {
    curl_easy_setopt(m_curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
  }

  m_content.clear();
  m_content_header.clear();
  m_extern_writer = is_file;
  res = curl_easy_perform(m_curl_handle);
  if (CURLE_OK != res) {
    debugEntry (LL_WARN, LOG_MODULE_INDEX_OTHER, "post %s(%s) fail, the error code is %d.", url.c_str(), ca_info.c_str(), res);
    return (false);
  }
  debugEntry (LL_OBSERVE, LOG_MODULE_INDEX_OTHER, "post %s succ.", url.c_str());
  return (true);
}

int httpsession::writer_header(char *data, size_t size, size_t nmemb, std::string *writerData)
{
  unsigned long sizes = size * nmemb;
  if (NULL == writerData) return 0;
  writerData->append(data, sizes);
  return sizes;
}

int httpsession::writer(char *data, size_t size, size_t nmemb, void *writerData)
{
  httpsession *_this = (httpsession *) writerData;
  unsigned long sizes = size * nmemb;
  if (NULL == writerData) return 0;
  if (_this->is_extern_writer()) {
    if (_this->m_data_arrival_callback)
      sizes = _this->m_data_arrival_callback(_this->m_dac_context, (u_char *) data, sizes);
  } else _this->m_content.append(data, sizes);
  return sizes;
}

bool httpsession::get_remote_file_length(std::string url, unsigned long &length)
{
  long response_code = 0;
  double remote_length = 0;
  CURLcode rtCode;

  curl_easy_setopt(m_curl_handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(m_curl_handle, CURLOPT_HEADER, 1);
  curl_easy_setopt(m_curl_handle, CURLOPT_NOBODY, 1);

  if (CURLE_OK == curl_easy_perform(m_curl_handle)) {
    rtCode = curl_easy_getinfo(m_curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    if (CURLE_OK == rtCode && 200 == response_code)
      rtCode = curl_easy_getinfo(m_curl_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &remote_length);
  }

  if (remote_length > 0) length = (unsigned long) remote_length;
  else length = 0;

  return (length > 0);
}

void httpsession::get_download_speed_info(double &speed, double &total_time)
{
  curl_easy_getinfo (m_curl_handle, CURLINFO_SPEED_DOWNLOAD, &speed);
  curl_easy_getinfo (m_curl_handle, CURLINFO_TOTAL_TIME, &total_time);
}
// enf of httpsession
#endif //NOTUSING_CURL
