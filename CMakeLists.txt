cmake_minimum_required(VERSION 3.15)
project(cryptotrader CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /opt/local/lib)

# ---------------------------------------------------------------------------
# Compiler configuration
# ---------------------------------------------------------------------------
if(MSVC)
  # /FI force-includes the cross-platform shim into every translation unit so
  # the (originally Linux/GCC) sources build unmodified under MSVC.
  add_compile_options(/EHsc /bigobj /utf-8 /Zc:__cplusplus
                      /FI${CMAKE_SOURCE_DIR}/lib/include/utils/PlatformCompat.h)
  add_compile_definitions(
    DEBUG
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0A00
    _USE_MATH_DEFINES
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
    _WINSOCK_DEPRECATED_NO_WARNINGS
    _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    ASIO_STANDALONE)
else()
  #set(CMAKE_CXX_FLAGS "-g -DDEBUG -pthread -fsanitize=address")
  set(CMAKE_CXX_FLAGS "-g -DDEBUG -pthread -Werror -Wall -Wno-unused-function")
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Header-only / vendored third-party includes
# ---------------------------------------------------------------------------
include_directories(3rdparty/fast-cpp-csv-parser)
include_directories(3rdparty/Simple-Iterator-Template)
include_directories(3rdparty/json/single_include)
include_directories(3rdparty/asio-1.10.8/include)
include_directories(3rdparty/thread-pool/include)
include_directories(3rdparty/websocketpp)

# ---------------------------------------------------------------------------
# External libraries
#
# On Windows these are provided by vcpkg (pass the vcpkg toolchain file at
# configure time). On Linux they come from the system package manager, matching
# the original setup.
# ---------------------------------------------------------------------------
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

if(WIN32)
  # crypto++ from vcpkg exposes a CONFIG package with the cryptopp::cryptopp target.
  find_package(cryptopp CONFIG REQUIRED)
  set(CRYPTOPP_LIBRARIES cryptopp::cryptopp)
else()
  find_package(CryptoPP REQUIRED)
  set(CRYPTOPP_LIBRARIES cryptopp)
  include_directories(3rdparty/cryptopp/build/include)
endif()

# ta-lib: built from the 3rdparty/ta-lib-rt submodule and installed into
# 3rdparty/ta-lib-rt/ta-lib/build/install (see the Windows build steps in the
# README).
set(TALIB_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/ta-lib-rt/ta-lib/build/install")
if(EXISTS "${TALIB_ROOT}/include")
  include_directories("${TALIB_ROOT}/include")
  # Prefer the static archive (ta_lib_a) so no ta_lib.dll needs deploying.
  find_library(TALIB_LIBRARY
    NAMES ta_lib_a ta_lib_static ta_lib ta-lib
    PATHS "${TALIB_ROOT}/lib" NO_DEFAULT_PATH)
else()
  # Linux layout from the original setup
  include_directories(3rdparty/ta-lib-rt/ta-lib/build/include)
  find_library(TALIB_LIBRARY NAMES ta_lib PATHS 3rdparty/ta-lib-rt/ta-lib/build/lib)
endif()

# Cassandra cpp-driver: built from the 3rdparty/cpp-driver submodule and
# installed into 3rdparty/cpp-driver/build/install (see the README).
set(CASS_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/cpp-driver/build/install")
if(EXISTS "${CASS_ROOT}/include")
  include_directories("${CASS_ROOT}/include")
  find_library(CASSANDRA_LIBRARY
    NAMES cassandra cassandra_static
    PATHS "${CASS_ROOT}/lib" NO_DEFAULT_PATH)
else()
  include_directories(3rdparty/cpp-driver/include)
  find_library(CASSANDRA_LIBRARY NAMES cassandra PATHS 3rdparty/cpp-driver/build/lib)
endif()

message(STATUS "OpenSSL libraries:  ${OPENSSL_LIBRARIES}")
message(STATUS "crypto++ libraries: ${CRYPTOPP_LIBRARIES}")
message(STATUS "ta-lib library:     ${TALIB_LIBRARY}")
message(STATUS "cassandra library:  ${CASSANDRA_LIBRARY}")

add_subdirectory(lib)

include_directories(lib/include)
include_directories(lib/src)

add_library(Catch2 INTERFACE)
file(GLOB TEST_SOURCES tests/*.cpp)

target_include_directories(Catch2 INTERFACE 3rdparty/Catch2/single_include)

add_executable(cryptotrader bin/main.cpp)
target_link_libraries(cryptotrader TraderBot)

# create test executable
add_executable(test_cryptotrader ${TEST_SOURCES})
target_link_libraries(test_cryptotrader TraderBot Catch2)

enable_testing()
add_test(NAME AllTests COMMAND test_cryptotrader)
