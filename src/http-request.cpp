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

#include "http-request.hpp"

HttpRequest::HttpRequest(std::map<std::string, std::string> getData, std::string const &page):
  m_getData(getData),
  m_postData(),
  m_page(page)
{
}

HttpRequest::~HttpRequest() {
}
  
std::map<std::string, std::string> HttpRequest::getGetData() const {
  return m_getData;
}

std::string HttpRequest::getPage() const {
  return m_page;
}

std::map<std::string, std::string> HttpRequest::getPostData() const {
  return m_postData;
}

void HttpRequest::setPostData(std::map<std::string, std::string> postData) {
  m_postData = postData;
}
