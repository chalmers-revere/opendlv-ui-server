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
