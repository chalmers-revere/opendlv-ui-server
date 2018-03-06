#ifndef SESSIONDATA_HPP
#define SESSIONDATA_HPP

#include <map>
#include <string>

class SessionData {

 public:
  SessionData(uint16_t);
  SessionData(uint16_t, std::map<std::string, std::string>);
  SessionData(SessionData const &) = delete;
  SessionData &operator=(SessionData const &) = delete;
  ~SessionData();
  std::map<std::string, std::string> getAll() const;
  uint16_t getSessionId() const;
  void add(std::string const &, std::string const &);

 private:
  std::map<std::string, std::string> m_sessionData;
  uint16_t m_sessionId;
};

#endif
