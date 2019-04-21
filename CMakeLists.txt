cmake_minimum_required(VERSION 3.2)
project(cryptotrader)

set(CMAKE_CXX_STANDARD 11)

#set(CMAKE_CXX_FLAGS "-g -DDEBUG -pthread -fsanitize=address")
set(CMAKE_CXX_FLAGS "-g -DDEBUG -pthread -Werror -Wall -Wno-unused-function")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /opt/local/lib)


include_directories(3rdparty/fast-cpp-csv-parser)
include_directories(3rdparty/Simple-Iterator-Template)
include_directories(3rdparty/json/single_include)
include_directories(3rdparty/cryptopp/build/include)
include_directories(3rdparty/ta-lib-rt/ta-lib/build/include)
include_directories(3rdparty/cpp-driver/include)
include_directories(3rdparty/asio-1.10.8/include)
include_directories(3rdparty/thread-pool/include)
include_directories(3rdparty/websocketpp)

#find_package(spdlog CONFIG REQUIRED) will be integrated later
find_package(CURL REQUIRED)

set(OPENSSL_ROOT_DIR /usr/local/Cellar/openssl/1.0.2n/lib)
find_package(OpenSSL)

# 3rd party libraries
include_directories(${CURL_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

link_directories(3rdparty/cryptopp/build/lib)
link_directories(3rdparty/cpp-driver/build/lib)
link_directories(3rdparty/ta-lib-rt/ta-lib/build/lib)

MESSAGE( STATUS "OPENSSL_LIBRARY_DIRS: " ${OPENSSL_LIBRARIES} )

add_subdirectory(lib)

include_directories(lib/include)
include_directories(lib/src)

#include(cotire)

add_library(Catch2 INTERFACE)
file(GLOB TEST_SOURCES tests/*.cpp)

# add precompiled headers
#set_target_properties(cryptotrader PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/stdafx.h")
#cotire(cryptotrader)
target_include_directories(Catch2 INTERFACE 3rdparty/Catch2/single_include)

add_executable(cryptotrader bin/main.cpp)
target_link_libraries(cryptotrader TraderBot)

# create test executable
add_executable(test_cryptotrader ${TEST_SOURCES})
target_link_libraries(test_cryptotrader TraderBot Catch2)

enable_testing()
add_test(NAME AllTests COMMAND test_cryptotrader)
