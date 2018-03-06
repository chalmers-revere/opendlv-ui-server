#include "session-data.hpp"

SessionData::SessionData(uint16_t sessionId):
  m_sessionData(),
  m_sessionId(sessionId)
{
}

SessionData::SessionData(uint16_t sessionId, 
    std::map<std::string, std::string> sessionData):
  m_sessionData(sessionData),
  m_sessionId(sessionId)
{
}

SessionData::~SessionData() {
}

std::map<std::string, std::string> SessionData::getAll() const {
  return m_sessionData;
}

uint16_t SessionData::getSessionId() const {
  return m_sessionId;
}

void SessionData::add(std::string const &key, std::string const &value) {
  m_sessionData[key] = value;
}
