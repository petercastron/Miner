#pragma once

#include "websocket.h"
#include "loggerLocal.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <signal.h>

typedef bool (*funWSmessage)(const std::string &ip, u_short port, const std::string &message, std::string &ret_message, void *parmame);
typedef bool (*funWSConn)(const std::string &ip, u_short port, const std::string &request_uri, void *parmame);

class Server
{
public:
  struct CMDConnData
  {
    bool login;
  };
  using WSServer = websocket::WSServer<Server, CMDConnData>;
  using WSConn = WSServer::Connection;

  void run(funWSConn connProcess, funWSmessage messageProcess, void *parmame) {
    if (!wsserver.init("0.0.0.0", 1234)) {
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "wsserver init failed: : %s", wsserver.getLastError());
      return;
    }

    if (NULL != connProcess) m_WSconnProcess = connProcess;
    if (NULL != messageProcess) m_WSmessageProcess = messageProcess;
    if (NULL != parmame) m_parmame = parmame;

    running = true;
    ws_thr = std::thread([this]() {
      while (running.load(std::memory_order_relaxed)) {
        wsserver.poll(this);
        std::this_thread::yield();
        usleep(50 * 1000); //50 ms
      }
    });

    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Server running...");
    ws_thr.join();
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Server stopped");
  }

  void stop() { running = false; }

  // called when a new websocket connection is about to open
  // optional: origin, protocol, extensions will be nullptr if not exist in the request headers
  // optional: fill resp_protocol[resp_protocol_size] to add protocol to response headers
  // optional: fill resp_extensions[resp_extensions_size] to add extensions to response headers
  // return true if accept this new connection
  bool onWSConnect(WSConn& conn, const char* request_uri, const char* host, const char* origin, const char* protocol,
                   const char* extensions, char* resp_protocol, uint32_t resp_protocol_size, char* resp_extensions,
                   uint32_t resp_extensions_size) {

    struct sockaddr_in addr;
    conn.getPeername(addr);

#if 0
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "ws connection from: %s:%u", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "request_uri: %s", request_uri);
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "host: %s", host);

    if (origin) {
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "origin: %s", origin);
    }
    if (protocol) 
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "protocol: %s", protocol);
    
    if (extensions) 
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "extensions: %s", extensions);
#endif    

    m_WSconnProcess(std::string(inet_ntoa(addr.sin_addr)), addr.sin_port, std::string(request_uri), m_parmame);

    return true;
  }

  // called when a websocket connection is closed
  // status_code 1005 means no status code in the close msg
  // status_code 1006 means not a clean close(tcp connection closed without a close msg)
  void onWSClose(WSConn& conn, uint16_t status_code, const char* reason) {
    debugEntry(LL_VERBOSE, LOG_MODULE_INDEX_HIRE, "ws close, status_code: %u, reason: %s", status_code, reason);
  }

  // onWSMsg is used if RecvSegment == false(by default), called when a whole msg is received
  void onWSMsg(WSConn& conn, uint8_t opcode, const uint8_t* payload, uint32_t pl_len) {
    if (opcode == websocket::OPCODE_PING) {
      conn.send(websocket::OPCODE_PONG, payload, pl_len);
      return;
    }
    if (opcode != websocket::OPCODE_TEXT) {
      conn.close(1003, "not text msg");
      return;
    }
    
    std::string request_message = "", response_message = "";
    request_message.assign((const char*)payload, pl_len);
    //printf("REQUEST MESSAGE : %s\n", request_message.c_str());

    struct sockaddr_in addr;
    conn.getPeername(addr);
    bool rtCode = m_WSmessageProcess(std::string(inet_ntoa(addr.sin_addr)), addr.sin_port, request_message, response_message, m_parmame);
    if (!rtCode) {
      conn.close(1013, "Try Again Later");
      return;
    }

    if (0 < response_message.size()) {
      conn.send(websocket::OPCODE_TEXT, (const uint8_t*)response_message.data(), response_message.size());
      //printf("RESPONSE MESSAGE : %s\n", response_message.c_str());
    }
#if 0
    const char* data = (const char*)payload;
    const char* data_end = data + pl_len;
    char buf[4096] = {0};
    const char* argv[4096];
    char* out = buf + 1;
    int argc = 0;
    bool in_quote = false;
    bool single_quote = false;
    while (data < data_end) {
      char ch = *data++;
      if (!in_quote) {
        if (ch == ' ') *out++ = 0;
        else {
          if (*(out - 1) == 0) argv[argc++] = out;
          if (ch == '\'')
            in_quote = single_quote = true;
          else if (ch == '"')
            in_quote = true;
          else if (ch == '\\')
            *out++ = *data++;
          else
            *out++ = ch;
        }
      }
      else {
        if (single_quote) {
          if (ch == '\'')
            in_quote = single_quote = false;
          else
            *out++ = ch;
        }
        else {
          if (ch == '"')
            in_quote = false;
          else if (ch == '\\' && (*data == '\\' || *data == '"'))
            *out++ = *data++;
          else
            *out++ = ch;
        }
      }
    }
    if (argc) {
      *out = 0;
      std::string resp = onCMD(conn.user_data, argc, argv);
      if (resp.size()) conn.send(websocket::OPCODE_TEXT, (const uint8_t*)resp.data(), resp.size());
    }
#endif  
  }

  // onWSSegment is used if RecvSegment == true, called when a segment is received
  // pl_start_idx: index in the whole msg for the 1st byte of payload
  // fin: whether it's the last segment
  void onWSSegment(WSConn& conn, uint8_t opcode, const uint8_t* payload, uint32_t pl_len, uint32_t pl_start_idx,
                   bool fin) {
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "error: onWSSegment should not be called");
  }

private:
  std::string onCMD(CMDConnData& conn, int argc, const char** argv) {
    std::string resp;
    std::string admincmd_help = "Server help:\n"
                                "login password\n"
                                "echo str\n"
                                "stop\n";

    if (!strcmp(argv[0], "help")) {
      resp = admincmd_help;
    }
    else if (!strcmp(argv[0], "login")) {
      if (argc < 2 || strcmp(argv[1], "123456")) {
        resp = "wrong password";
      }
      else {
        conn.login = true;
        resp = "login success";
      }
    }
    else if (!conn.login) {
      resp = "must login first";
    }
    else if (!strcmp(argv[0], "echo")) {
      if (argc >= 2) resp = std::string(argv[1]);
    }
    else if (!strcmp(argv[0], "stop")) {
      stop();
    }
    else {
      resp = "invalid cmd, check help";
    }

    return resp;
  }

private:
  funWSmessage m_WSmessageProcess;
  funWSConn m_WSconnProcess;
  void *m_parmame;
  WSServer wsserver;
  std::thread ws_thr;
  std::atomic<bool> running;
};
