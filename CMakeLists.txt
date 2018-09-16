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

set(CLUON_COMPLETE cluon-complete-v0.0.110.hpp)

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

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/cluon-complete.hpp
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/test/${CLUON_COMPLETE} ${CMAKE_BINARY_DIR}/cluon-complete.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test/${CLUON_COMPLETE})

include(FindOpenSSL)
find_package(Threads REQUIRED)
find_package(Libwebsockets REQUIRED)

include_directories(${LIBWEBSOCKETS_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(SYSTEM ${CMAKE_BINARY_DIR})
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(LIBRARIES ${LIBRARIES} ${LIBWEBSOCKETS_LIBRARIES} ${OPENSSL_LIBRARIES} Threads::Threads)

add_library(${PROJECT_NAME}-objects OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/http-request.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/http-response.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/session-data.cpp
  )
set_target_properties(${PROJECT_NAME}-objects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(${PROJECT_NAME}-static STATIC $<TARGET_OBJECTS:${PROJECT_NAME}-objects>)
set_target_properties(${PROJECT_NAME}-static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

add_library(${PROJECT_NAME}-shared SHARED $<TARGET_OBJECTS:${PROJECT_NAME}-objects>)
set_target_properties(${PROJECT_NAME}-shared PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

enable_testing()
add_executable(${PROJECT_NAME}-runner ${CMAKE_CURRENT_SOURCE_DIR}/test/test-${PROJECT_NAME}.cpp ${CMAKE_BINARY_DIR}/cluon-complete.hpp)
target_link_libraries(${PROJECT_NAME}-runner ${PROJECT_NAME}-shared ${LIBRARIES})
add_test(NAME ${PROJECT_NAME}-runner COMMAND ${PROJECT_NAME}-runner)

install(TARGETS ${PROJECT_NAME}-static DESTINATION lib)
install(TARGETS ${PROJECT_NAME}-shared DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/http-request.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/http-response.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/session-data.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}.hpp DESTINATION include/${PROJECT_NAME})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/cmake/${PROJECT_NAME})
