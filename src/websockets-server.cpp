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
    std::function<std::unique_ptr<HttpResponse>(HttpRequest const &, 
      std::shared_ptr<SessionData>)> httpRequestDelegate,
    std::function<void(std::string const &, uint32_t)> dataReceiveDelegate):
  m_dataReceiveDelegate{dataReceiveDelegate},
  m_httpRequestDelegate{httpRequestDelegate},
  m_sessionData{},
  m_outputData{},
  m_context{nullptr, [](struct lws_context *context) {
    lws_context_destroy(context);
  }},
  m_outputDataMutex{},
  m_clientCount{0},
  m_port{port}
{
  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  info.port = m_port;
  info.protocols = m_protocols;
  info.gid = -1;
  info.uid = -1;
  info.user = static_cast<void *>(this);
  
  {
    m_context.reset(lws_create_context(&info));
  }

  lws_set_log_level(7, nullptr);
}

WebsocketServer::~WebsocketServer() {
}

int32_t WebsocketServer::callbackHttp(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {

  WebsocketServer *websocketServer = 
    reinterpret_cast<WebsocketServer *>(lws_context_user(lws_get_context(wsi)));
  
  ClientData *clientData = static_cast<ClientData *>(user);

  if (reason == LWS_CALLBACK_HTTP) {

    if (len < 1) {
			lws_return_http_status(wsi, HTTP_STATUS_BAD_REQUEST, nullptr);
			if (lws_http_transaction_completed(wsi)) {
        return -1;
      }
    }

    char buf[256];

    std::string page(static_cast<const char *>(in), len);
    std::map<std::string, std::string> cookies;
    
    // Extract cookies from the header, to find any sessionId.
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
    
    clientData->httpRequest = std::unique_ptr<HttpRequest>(
        new HttpRequest(getData, page));
    clientData->sessionId = sessionId;
   
    // If POST URL, continue to accept data.
    int32_t result;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI)) {
			result = lws_hdr_copy(wsi, buf, sizeof(buf), WSI_TOKEN_POST_URI);
			if (result < 0) {
				return -1;
      }
			return 0;
    }

    clientData->httpResponse = std::move(
        websocketServer->delegateRequestedHttp(
          *clientData->httpRequest, sessionId));
    if (clientData->httpResponse == nullptr) {
      lws_return_http_status(wsi, HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, "Unknown request");
      return -1;
    }
    
    std::string header = createHttpHeader(*clientData->httpResponse, sessionId);

    unsigned char *headerBuf = new unsigned char[header.length() + 1];
    strcpy((char *)headerBuf, header.c_str());

    result = lws_write(wsi, headerBuf, header.length(), LWS_WRITE_HTTP_HEADERS);
    delete[] headerBuf;

    if (result < 0) {
      return -1;
    }

    lws_callback_on_writable(wsi);

  } else if (reason == LWS_CALLBACK_HTTP_WRITEABLE) {
    std::string const CONTENT = clientData->httpResponse->getContent() + "\n";

    uint32_t const LEN = CONTENT.length();

    unsigned char *contentBuf = new unsigned char[LEN];
    memcpy(contentBuf, CONTENT.c_str(), LEN);

    lws_write(wsi, contentBuf, LEN, LWS_WRITE_HTTP);
    delete[] contentBuf;

    return -1;

  } else if (reason == LWS_CALLBACK_HTTP_BODY) {
    std::string request(static_cast<const char *>(in), len);
    lwsl_notice("HTTP body: '%s'\n", request.c_str());
  } else if (reason == LWS_CALLBACK_HTTP_DROP_PROTOCOL) {
 /*   if (clientData->httpRequest != nullptr) {
      delete clientData->httpRequest;
    }
    if (clientData->httpResponse != nullptr) {
      delete clientData->httpResponse;
    }*/
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
    auto data = websocketServer->getOutputData();

    unsigned char *dataBuf = new unsigned char[data.length() + LWS_PRE];
    memcpy(dataBuf + LWS_PRE, data.c_str(), data.size());
    lws_write(wsi, &dataBuf[LWS_PRE], data.length(), LWS_WRITE_BINARY);
    delete[] dataBuf;
  }
  
  return 0;
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

std::unique_ptr<HttpResponse> WebsocketServer::delegateRequestedHttp(
    HttpRequest const &request, uint16_t sessionId) {
  if (m_httpRequestDelegate != nullptr) {
    auto sessionData = m_sessionData[sessionId];
    auto response = m_httpRequestDelegate(request, sessionData);
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

std::string WebsocketServer::getOutputData() {
  std::lock_guard<std::mutex> guard(m_outputDataMutex);
  return m_outputData;
}

uint32_t WebsocketServer::loginUser() {
  m_clientCount++;
  return m_clientCount;
}

void WebsocketServer::stepServer() {
  lws_service(&(*m_context), 10000);
}

void WebsocketServer::setDataReceiveDelegate(
    std::function<void(std::string const &, uint32_t)> dataReceiveDelegate) {
  m_dataReceiveDelegate = dataReceiveDelegate;
}

void WebsocketServer::sendDataToAllClients(std::string data) {

  if (m_context == nullptr) {
    return;
  } else {
    std::lock_guard<std::mutex> guard(m_outputDataMutex);
    m_outputData = data;
  }

  {
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
