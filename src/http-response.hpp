#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <map>
#include <string>

class HttpResponse {

 public:
  HttpResponse(std::string const &, std::string const &);
  HttpResponse(HttpResponse const &) = delete;
  HttpResponse &operator=(HttpResponse const &) = delete;
  ~HttpResponse();
  std::string getContentType() const;
  std::string getHtml() const;

 private:
  std::string m_contentType;
  std::string m_html;
};

#endif
