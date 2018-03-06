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
