/*
 * Copyright (C) 2018 Ola Benderius
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WEBSOCKETSSERVER_HPP
#define WEBSOCKETSSERVER_HPP

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <libwebsockets.h>

class HttpRequest;
class HttpResponse;
class SessionData;

class WebsocketServer {

 public:
  WebsocketServer(std::function<std::shared_ptr<HttpResponse>(
        HttpRequest const &, std::shared_ptr<SessionData>)>,
      std::function<void(std::string const &, uint32_t)>);
  WebsocketServer(WebsocketServer const &) = delete;
  WebsocketServer &operator=(WebsocketServer const &) = delete;
  ~WebsocketServer();
  void runServer();
  void setDataReceiveDelegate(std::function<void(std::string const &, uint32_t)>);
  void startServer();
  void stopServer();
  void sendDataToAllClients(std::string);

 private:
  void addHttpRequest(uint16_t, std::shared_ptr<HttpRequest>);
  void createSessionData(uint16_t);
  void delegateReceivedData(std::string const &, uint32_t) const;
  std::shared_ptr<HttpResponse> delegateRequestedHttp(uint16_t);
  std::vector<char unsigned> getOutputData() const;
  std::string getResponseHtml(uint16_t);
  uint32_t loginUser();
  void setPostData(uint16_t, std::map<std::string, std::string>);

  static int32_t callbackHttp(struct lws *, enum lws_callback_reasons, void *, void *, size_t);
  static int32_t callbackData(struct lws *, enum lws_callback_reasons, void *, void *, size_t);
  static std::string createHttpHeader(HttpResponse const &, uint16_t);
  static std::vector<std::string> split(std::string const &, char);

  struct lws_protocols m_protocols[3] = {
    {"http-only", &callbackHttp, 2, 0, 0, nullptr, 0},
    {"data", &callbackData, sizeof(int32_t), 1024, 0, nullptr, 0},
    {nullptr, nullptr, 0, 0, 0, nullptr, 0}
  };

  std::function<void(std::string const &, uint32_t)> m_dataReceiveDelegate;
  std::function<std::shared_ptr<HttpResponse>(HttpRequest const &, 
      std::shared_ptr<SessionData>)> m_httpRequestDelegate;

  std::map<uint16_t, std::shared_ptr<HttpRequest>> m_httpRequests;
  std::map<uint16_t, std::shared_ptr<HttpResponse>> m_httpResponses;
  std::map<uint16_t, std::shared_ptr<SessionData>> m_sessionData;
  
  std::vector<char unsigned> m_outputData;

  std::unique_ptr<std::thread> m_serverThread;
  std::unique_ptr<struct lws_context, void(*)(struct lws_context *)> m_context;
  
  std::mutex m_websocketsMutex;
  std::mutex m_loginMutex;

  uint32_t m_clientCount;
  bool m_serverIsRunning;
};

#endif
