# Copyright (C) 2018 Ola Benderius
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.2)

project(opendlv-ui-server)

set(CLUON_COMPLETE cluon-complete-v0.0.101.hpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -D_XOPEN_SOURCE=700 \
    -D_FORTIFY_SOURCE=2 \
    -O2 \
    -fstack-protector \
    -fomit-frame-pointer \
    -pipe \
    -pedantic -pedantic-errors \
    -Werror \
    -Weffc++ \
    -Wall -Wextra -Wshadow -Wdeprecated \
    -Wdiv-by-zero -Wfloat-equal -Wfloat-conversion -Wsign-compare -Wpointer-arith \
    -Wuninitialized -Wunreachable-code \
    -Wunused -Wunused-function -Wunused-label -Wunused-parameter -Wunused-but-set-parameter -Wunused-but-set-variable \
    -Wunused-value -Wunused-variable -Wunused-result \
    -Wmissing-field-initializers -Wmissing-format-attribute -Wmissing-include-dirs -Wmissing-noreturn")

include(FindOpenSSL)
find_package(Threads REQUIRED)
find_package(Libwebsockets REQUIRED)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/cluon-complete.hpp
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/src/${CLUON_COMPLETE} ${CMAKE_BINARY_DIR}/cluon-complete.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/${CLUON_COMPLETE})

add_custom_target(create-cluon-symlink ${CMAKE_BINARY_DIR}/cluon-complete.hpp)

include_directories(${LIBWEBSOCKETS_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_BINARY_DIR})
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(LIBRARIES ${LIBRARIES} ${LIBWEBSOCKETS_LIBRARIES} ${OPENSSL_LIBRARIES} Threads::Threads stdc++fs)

add_library(${PROJECT_NAME}-core OBJECT 
  ${CMAKE_BINARY_DIR}/cluon-complete.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/websockets-server.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/http-request.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/http-response.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/session-data.cpp
  )

add_library(${PROJECT_NAME}-static STATIC $<TARGET_OBJECTS:${PROJECT_NAME}-core>) 

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}.cpp $<TARGET_OBJECTS:${PROJECT_NAME}-core>)
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

enable_testing()
add_executable(${PROJECT_NAME}-runner ${CMAKE_CURRENT_SOURCE_DIR}/test/test-websockets-server.cpp $<TARGET_OBJECTS:${PROJECT_NAME}-core>)
target_link_libraries(${PROJECT_NAME}-runner ${LIBRARIES})
add_test(NAME ${PROJECT_NAME}-runner COMMAND ${PROJECT_NAME}-runner)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT ${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME}-static DESTINATION lib COMPONENT ${CMAKE_PROJECT_NAME})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/http-request.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/http-response.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/session-data.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/websockets-server.hpp DESTINATION include/${PROJECT_NAME} COMPONENT ${CMAKE_PROJECT_NAME})
