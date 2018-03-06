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
