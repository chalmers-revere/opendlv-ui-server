#include "http-response.hpp"

HttpResponse::HttpResponse(std::string const &contentType, std::string const &html):
  m_contentType(contentType),
  m_html(html)
{
}

HttpResponse::~HttpResponse() {
}
  
std::string HttpResponse::getContentType() const {
  return m_contentType;
}

std::string HttpResponse::getHtml() const {
  return m_html;
}
