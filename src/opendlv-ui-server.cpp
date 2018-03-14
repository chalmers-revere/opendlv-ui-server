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

#include <experimental/filesystem>
#include <fstream>
#include <iostream>

#include "cluon-complete.hpp"
#include "http-request.hpp"
#include "http-response.hpp"
#include "session-data.hpp"
#include "websockets-server.hpp"

int32_t main(int32_t argc, char **argv)
{
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("cid") || 0 == commandlineArguments.count("port") || 0 == commandlineArguments.count("http-root")) {
    std::cerr << argv[0] << " is the default HTTP/WebSocket server for OpenDLV user interfaces." << std::endl;
    std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session> --port=<the port where HTTP/WebSocket is served> --http-root=<folder where HTTP content can be found> [--id=<Identifier in case of multiple running instances>] [--verbose]" << std::endl;
    std::cerr << "Example: " << argv[0] << " --cid=111 --port=8000 --http-root=./http" << std::endl;
    retCode = 1;
  } else {
    uint32_t const ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
    bool const VERBOSE{commandlineArguments.count("verbose") != 0};

    uint32_t const HTTP_PORT = static_cast<uint32_t>(std::stoi(commandlineArguments["port"]));
    std::string const HTTP_ROOT = commandlineArguments["http-root"];

    (void)ID;
    (void)VERBOSE;

    auto httpRequestDelegate([&HTTP_ROOT](HttpRequest const &httpRequest, 
          std::shared_ptr<SessionData>) -> std::shared_ptr<HttpResponse>
        {
          std::string const PAGE = httpRequest.getPage();
          std::experimental::filesystem::path path{HTTP_ROOT + PAGE};

          if (!std::experimental::filesystem::exists(path)) {
            std::cout << "ERROR: file '" << path.string() <<  "' not found." << std::endl;
            return nullptr;
          }
          
          std::ifstream ifs(path.string());
          std::stringstream ss;
          ss << ifs.rdbuf();
          std::string content = ss.str();

          std::string contentType;
          std::string const EXTENSION = path.extension();
          std::cout << "DEBUG: page " << PAGE << " extension " << EXTENSION << std::endl;
          if (EXTENSION == ".html") {
            contentType = "text/html";
          } else if (EXTENSION == ".css") {
            contentType = "text/css";
          } else if (EXTENSION == ".js") {
            contentType = "text/javascript";
          } else if (EXTENSION == ".gif") {
            contentType = "image/gif";
          } else if (EXTENSION == ".png") {
            contentType = "image/png";
          } else if (EXTENSION == ".jpeg" || EXTENSION == ".jpg") {
            contentType = "image/jpeg";
          } else {
            contentType = "text/plain";
          }

          std::shared_ptr<HttpResponse> response(
              new HttpResponse(contentType, content));
          return response;
        });
    
    WebsocketServer ws(HTTP_PORT, httpRequestDelegate, nullptr);
    ws.startServer();

    uint16_t const CID = static_cast<uint16_t>(
        std::stoi(commandlineArguments["cid"]));
    auto onIncomingEnvelope([&ws](cluon::data::Envelope &&envelope) {
        auto gm = cluon::extractMessage<cluon::GenericMessage>(std::move(envelope));
        cluon::ToProtoVisitor protoEncoder;
        gm.accept(protoEncoder);

        ws.sendDataToAllClients(protoEncoder.encodedData());
      });
    
    cluon::OD4Session od4{CID, onIncomingEnvelope};

    auto dataReceivedDelegate([](std::string const &message, uint32_t senderId) {
        std::cout << "Got message '" << message << "' from " << senderId 
          << std::endl;
        });
    
    ws.setDataReceiveDelegate(dataReceivedDelegate);

    while (od4.isRunning()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  return retCode;
}
