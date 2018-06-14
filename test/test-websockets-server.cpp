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

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "cluon-complete.hpp"
#include "http-request.hpp"
#include "http-response.hpp"
#include "websockets-server.hpp"

TEST_CASE("Test websockets server start and simple connection with GET data.") {
  std::string const REQUESTED_PAGE = "/testpage.html";
  std::string const GET_KEY1 = "key1";
  std::string const GET_VALUE1 = "value1";
  std::string const GET_KEY2 = "key2";
  std::string const GET_VALUE2 = "value2";
  uint32_t PORT = 8000;

  auto httpRequestDelegate([&REQUESTED_PAGE, &GET_KEY1, &GET_VALUE1, 
      &GET_KEY2, &GET_VALUE2](HttpRequest const &httpRequest,
        std::shared_ptr<SessionData>, std::string const &) -> std::unique_ptr<HttpResponse>
      {
        REQUIRE(REQUESTED_PAGE == httpRequest.getPage()); 

        auto getData = httpRequest.getGetData();
        REQUIRE(getData[GET_KEY1] == GET_VALUE1);
        REQUIRE(getData[GET_KEY2] == GET_VALUE2);
        REQUIRE(getData.size() == 2);

        std::unique_ptr<HttpResponse> httpResponse(new HttpResponse("text/plain", "hello world"));
        return httpResponse;
      });
 
  WebsocketServer ws(PORT, httpRequestDelegate, nullptr, "", "");

  bool gotResponse = false;
  auto clientReceiveDelegate([&gotResponse](std::string &&, std::chrono::system_clock::time_point &&) noexcept
    {
      gotResponse = true;
    });
  auto connectionLostDelegate([]()
    { 
    });

  cluon::TCPConnection connection("127.0.0.1", PORT, clientReceiveDelegate, connectionLostDelegate);
  connection.send(std::move("GET /" + REQUESTED_PAGE + "?" + GET_KEY1 + "=" + GET_VALUE1 + "&" + GET_KEY2 + "=" + GET_VALUE2 + " HTTP/1.1\r\nHost: localhost\r\n\r\n"));

  while (!gotResponse) {
    ws.stepServer();
  }
  
  REQUIRE(gotResponse);
}

TEST_CASE("Test websockets server start and nullptr return and no GET data.") {
  std::string const REQUESTED_PAGE = "/testpage.html";
  uint32_t PORT = 8000;

  auto httpRequestDelegate([](HttpRequest const &httpRequest,
        std::shared_ptr<SessionData>, std::string const &) -> std::unique_ptr<HttpResponse>
      {
        REQUIRE(httpRequest.getGetData().size() == 0);
        return nullptr;
      });
 
  WebsocketServer ws(PORT, httpRequestDelegate, nullptr, "", "");

  bool gotResponse = false;
  auto clientReceiveDelegate([&gotResponse](std::string &&, std::chrono::system_clock::time_point &&) noexcept
    {
      gotResponse = true;
    });
  auto connectionLostDelegate([]()
    { 
    });

  cluon::TCPConnection connection("127.0.0.1", PORT, clientReceiveDelegate, connectionLostDelegate);
  connection.send(std::move("GET /" + REQUESTED_PAGE + " HTTP/1.1\r\nHost: localhost\r\n\r\n"));

  while (!gotResponse) {
    ws.stepServer();
  }
  
  REQUIRE(gotResponse);
}
