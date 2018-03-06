#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <map>
#include <string>

class HttpRequest {

 public:
  HttpRequest(std::map<std::string, std::string>, std::string const &);
  HttpRequest(HttpRequest const &) = delete;
  HttpRequest &operator=(HttpRequest const &) = delete;
  ~HttpRequest();
  std::map<std::string, std::string> getGetData() const;
  std::string getPage() const;
  std::map<std::string, std::string> getPostData() const;
  void setPostData(std::map<std::string, std::string>);

 private:
  std::map<std::string, std::string> m_getData;
  std::map<std::string, std::string> m_postData;
  std::string m_page;
};

#endif
