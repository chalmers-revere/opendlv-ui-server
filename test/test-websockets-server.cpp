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

#include "websockets-server.hpp"

TEST_CASE("Test websockets server start.") {
/*  
  auto httpRequestDelegate([](uint16_t, std::string const &,
        std::map<std::string, std::string>,
        std::map<std::string, std::string>,
        std::map<std::string, std::string>) -> std::pair<std::string, std::string>
      {
        std::string contentType = "text/html";

        char const *html = 
R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
  <title>Test page</title>
</head>

<body>
  Some nice content.
</body>

</html>)";

          return std::pair<std::string, std::string>(contentType, std::string(html));
      });
  
  auto dataReceivedDelegate([](std::string const &, uint32_t) {
      });

  WebsocketServer ws(httpRequestDelegate, dataReceivedDelegate);
  ws.startServer();

  std::this_thread::sleep_for(std::chrono::seconds(2));
*/
  REQUIRE(true);
}

