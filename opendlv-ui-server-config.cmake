# Copyright (C) 2017-2018 Ola Benderius
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_path(OPENDLVUISERVER_INCLUDE_DIR opendlv-ui-server.hpp
  NAMES opendlv-ui-server
  PATHS 
  /usr/include
  /usr/local/include)

# Shared libraries
find_library(OPENDLVUISERVER_LIBRARY
  NAMES 
  opendlv-ui-server
  PATHS 
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64)
find_library(OPENDLVUISERVER_LIBRARY_WEBSOCKETS
  NAMES 
  websockets
  PATHS 
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64)
find_library(OPENDLVUISERVER_LIBRARY_SSL
  NAMES 
  ssl
  PATHS 
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64)
find_library(OPENDLVUISERVER_LIBRARY_CRYPTO
  NAMES 
  crypto
  PATHS 
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64)
set(OPENDLVUISERVER_LIBRARIES 
  ${OPENDLVUISERVER_LIBRARY} 
  ${OPENDLVUISERVER_LIBRARY_WEBSOCKETS} 
  ${OPENDLVUISERVER_LIBRARY_SSL}
  ${OPENDLVUISERVER_LIBRARY_CRYPTO})

# Static libraries
find_library(OPENDLVUISERVER_LIBRARY_STATIC
  NAMES 
  libopendlv-ui-server.a
  PATHS 
  /lib
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64)
find_library(OPENDLVUISERVER_LIBRARY_WEBSOCKETS_STATIC
  NAMES 
  libwebsockets.a
  PATHS 
  /lib
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64)
find_library(OPENDLVUISERVER_LIBRARY_SSL_STATIC
  NAMES 
  libssl.a
  PATHS 
  /lib
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64)
find_library(OPENDLVUISERVER_LIBRARY_CRYPTO_STATIC
  NAMES 
  libcrypto.a
  PATHS 
  /lib
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64)
find_library(OPENDLVUISERVER_LIBRARY_Z_STATIC
  NAMES 
  libz.a
  PATHS 
  /lib
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64)
set(OPENDLVUISERVER_LIBRARIES_STATIC
  ${OPENDLVUISERVER_LIBRARY_STATIC} 
  ${OPENDLVUISERVER_LIBRARY_WEBSOCKETS_STATIC} 
  ${OPENDLVUISERVER_LIBRARY_SSL_STATIC}
  ${OPENDLVUISERVER_LIBRARY_CRYPTO_STATIC} 
  ${OPENDLVUISERVER_LIBRARY_Z_STATIC})

if("${OPENDLVUISERVER_INCLUDE_DIR}" STREQUAL "")
  message(FATAL_ERROR "Could not find opendlv-ui-server.")
else()
  set(FOUND_OPENDLVUISERVER 1)
endif()
