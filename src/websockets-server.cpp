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


#include <chrono>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>

#include "cluon-complete.hpp"
#include "http-request.hpp"
#include "http-response.hpp"
#include "session-data.hpp"
#include "websockets-server.hpp"
#include "mustache.hpp"

WebsocketServer::WebsocketServer(uint32_t port,
    std::function<std::shared_ptr<HttpResponse>(HttpRequest const &, 
      std::shared_ptr<SessionData>)> httpRequestDelegate,
    std::function<void(std::string const &, uint32_t)> dataReceiveDelegate):
  m_dataReceiveDelegate{dataReceiveDelegate},
  m_httpRequestDelegate{httpRequestDelegate},
  m_httpRequests{},
  m_httpResponses{},
  m_sessionData{},
  m_outputData{},
  m_serverThread{nullptr},
  m_context{nullptr, [](struct lws_context *context) {
    lws_context_destroy(context);
  }},
  m_websocketsMutex{},
  m_loginMutex{},
  m_clientCount{0},
  m_port{port},
  m_serverIsRunning{false} {
}

WebsocketServer::~WebsocketServer() {
  stopServer();
}

int32_t WebsocketServer::callbackHttp(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {

  WebsocketServer *websocketServer = 
    reinterpret_cast<WebsocketServer *>(lws_context_user(lws_get_context(wsi)));

  int16_t *userId = static_cast<int16_t *>(user);
  if (reason == LWS_CALLBACK_HTTP) {
    std::string page(static_cast<const char *>(in), len);
    std::map<std::string, std::string> cookies;
    
    // Extract cookies from the header, to find any sessionId.
    char buf[256];
    if (lws_hdr_copy(wsi, buf, sizeof(buf), WSI_TOKEN_HTTP_COOKIE) > 0) {
      std::string cookiesStr(buf);

      for (auto cookieStr : split(cookiesStr, ';')) {
        std::vector<std::string> cookie = split(cookiesStr, '=');
        if (cookie.size() == 2) {
          cookies[cookie[0]] = cookie[1];
        }
      }
    }

    uint16_t sessionId;
    if (cookies.count("sessionId") != 0) {
      sessionId = std::stoi(cookies["sessionId"]);
    } else {
      // Unknown user (no cookie from client) generate and add new session id.
      std::mt19937 rng;
      rng.seed(std::random_device()());
      std::uniform_int_distribution<std::mt19937::result_type> dist(
          std::numeric_limits<decltype(sessionId)>::min(),
          std::numeric_limits<decltype(sessionId)>::max());
      sessionId = dist(rng);

      websocketServer->createSessionData(sessionId);
    }
    
    *userId = sessionId;
    
    // Extract GET data from the HTTP request.
    std::map<std::string, std::string> getData;
    uint32_t n = 0;
    while (lws_hdr_copy_fragment(wsi, buf, sizeof(buf), WSI_TOKEN_HTTP_URI_ARGS, n) > 0) {
      std::string getDataStr(buf);
      std::vector<std::string> getDataVec = split(getDataStr, '=');
      if (getDataVec.size() == 2) {
        getData[getDataVec[0]] = getDataVec[1];
      }
      n++;
    }

    std::shared_ptr<HttpRequest> httpRequest(new HttpRequest(getData, page));
    websocketServer->addHttpRequest(sessionId, httpRequest);
    
   // if (lws_hdr_copy(wsi, buf2, sizeof(buf2), WSI_TOKEN_POST_URI) > 0) {
   //   lwsl_notice("Cookie: '%s'\n", buf2);
   // }


    // TODO: Should be after post data?
    auto response = websocketServer->delegateRequestedHttp(sessionId);
    std::string header;
    if (response != nullptr) {
      header = createHttpHeader(*response, sessionId);
    } else {
      header = createHttpHeaderNotFound();
    }

    unsigned char *headerBuf = new unsigned char[header.length() + 1];
    strcpy((char *)headerBuf, header.c_str());

    lws_write(wsi, headerBuf, header.length(), LWS_WRITE_HTTP_HEADERS);

    free(headerBuf);

    lws_callback_on_writable(wsi);

  } else if (reason == LWS_CALLBACK_HTTP_WRITEABLE) {
  
    uint16_t sessionId = *userId; 

    std::string html = websocketServer->getResponseContent(sessionId) + "\n";
    
    unsigned char *htmlBuf = new unsigned char[html.length()];
    strcpy((char *)htmlBuf, html.c_str());

    lws_write(wsi, htmlBuf, html.length(), LWS_WRITE_HTTP);

    free(htmlBuf);

  } else if (reason == LWS_CALLBACK_HTTP_BODY) {
    std::string request(static_cast<const char *>(in), len);
    lwsl_notice("HTTP body: '%s'\n", request.c_str());
  } else if (reason == LWS_CALLBACK_HTTP_FILE_COMPLETION) {
    return -1;
  }
  
  return 0;
}

int32_t WebsocketServer::callbackData(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {

  WebsocketServer *websocketServer =
      reinterpret_cast<WebsocketServer *>(lws_context_user(lws_get_context(wsi)));

  int32_t *userId = static_cast<int32_t *>(user);
  if (reason == LWS_CALLBACK_ESTABLISHED) {
    *userId = websocketServer->loginUser();
  } else if (reason == LWS_CALLBACK_RECEIVE) {
    std::string data(static_cast<const char *>(in), len);
    websocketServer->delegateReceivedData(data, *userId);
  } else if (reason == LWS_CALLBACK_SERVER_WRITEABLE) {
    auto outputData = websocketServer->getOutputData();
    char unsigned *buf = outputData.data();
    uint32_t const DATA_LENGTH = static_cast<uint32_t>(outputData.size()) - LWS_PRE;
    lws_write(wsi, &buf[LWS_PRE], DATA_LENGTH, LWS_WRITE_TEXT);
  }
  
  return 0;
}

void WebsocketServer::addHttpRequest(uint16_t sessionId, std::shared_ptr<HttpRequest> httpRequest) {
  m_httpRequests[sessionId] = httpRequest;
}

void WebsocketServer::createSessionData(uint16_t sessionId) {
  std::shared_ptr<SessionData> sessionData(new SessionData(sessionId));
  m_sessionData[sessionId] = sessionData;
}

void WebsocketServer::delegateReceivedData(std::string const &message, uint32_t senderId) const {
  if (m_dataReceiveDelegate != nullptr) {
    m_dataReceiveDelegate(message, senderId);
  }
}

std::shared_ptr<HttpResponse> WebsocketServer::delegateRequestedHttp(
    uint16_t sessionId) {
  if (m_httpRequestDelegate != nullptr) {
    auto response = m_httpRequestDelegate(*m_httpRequests[sessionId],
        m_sessionData[sessionId]);

    m_httpResponses[sessionId] = response;
   
    return response;
  }

  return nullptr;
}

std::string WebsocketServer::createHttpHeader(HttpResponse const &response, uint16_t sessionId) {
 
  std::string contentType = response.getContentType();
  int32_t contentLength = response.getContent().length();

  char const *headerTemplate = 
R"(HTTP/1.1 200 OK
content-type: {{content-type}}
accept-ranges: bytes
content-length: {{content-length}}
cache-control: no-store
connection: keep-alive
set-cookie: sessionId={{session-id}})";

  kainjow::mustache::data dataToBeRendered;
  dataToBeRendered.set("content-type", contentType);
  dataToBeRendered.set("content-length", std::to_string(contentLength + 1));
  dataToBeRendered.set("session-id", std::to_string(sessionId));

  kainjow::mustache::mustache tmpl{headerTemplate};
  
  std::stringstream sstr;
  sstr << tmpl.render(dataToBeRendered);
  std::string const header(sstr.str() + "\n\n");

  return header;
}

std::string WebsocketServer::createHttpHeaderNotFound() {
  std::string const header("HTTP/1.1 404 Not Found\ncontent-length: 0\ncache-control: no-store\n\n");
  return header;
}

std::vector<char unsigned> WebsocketServer::getOutputData() const {
  return m_outputData;
}

std::string WebsocketServer::getResponseContent(uint16_t sessionId) {
  auto response = m_httpResponses[sessionId];
  if (response != nullptr) {
    return response->getContent();
  }

  return "";
}

uint32_t WebsocketServer::loginUser() {
  std::lock_guard<std::mutex> guard(m_loginMutex);
  m_clientCount++;
  return m_clientCount;
}

void WebsocketServer::setPostData(uint16_t sessionId, 
    std::map<std::string, std::string> postData) {
  m_httpRequests[sessionId]->setPostData(postData);
}

void WebsocketServer::startServer() {
  m_serverThread.reset(new std::thread(&WebsocketServer::runServer, this));
}

void WebsocketServer::stopServer() {
  std::lock_guard<std::mutex> guard(m_websocketsMutex);
  m_serverIsRunning = false;
  m_serverThread->join();
}

void WebsocketServer::runServer() {

  m_serverIsRunning = true;

  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  info.port = m_port;
  info.protocols = m_protocols;
  info.gid = -1;
  info.uid = -1;
  info.user = static_cast<void *>(this);
  
  {
    std::lock_guard<std::mutex> guard(m_websocketsMutex);
    m_context.reset(lws_create_context(&info));
  }

  while (m_serverIsRunning) {
    lws_service(&(*m_context), 10000);
  }
}

void WebsocketServer::setDataReceiveDelegate(
    std::function<void(std::string const &, uint32_t)> dataReceiveDelegate) {
  m_dataReceiveDelegate = dataReceiveDelegate;
}

void WebsocketServer::sendDataToAllClients(std::string data) {

  if (m_context == nullptr) {
    return;
  }

  m_outputData = std::vector<char unsigned>(LWS_PRE, ' ');
  std::copy(data.begin(), data.end(), std::back_inserter(m_outputData));

  {
    std::lock_guard<std::mutex> guard(m_websocketsMutex);
    if (m_context == nullptr) {
      return;
    }
    lws_cancel_service(&(*m_context));
    lws_callback_on_writable_all_protocol(&(*m_context), &m_protocols[1]);
  }
}
  
std::vector<std::string> WebsocketServer::split(std::string const &text, char delimiter) {
  std::vector<std::string> tokens;
  std::size_t start = 0, end = 0;
  while ((end = text.find(delimiter, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = end + 1;
  }
  tokens.push_back(text.substr(start));
  return tokens;
}
