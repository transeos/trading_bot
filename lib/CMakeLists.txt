include_directories(include)
include_directories(include/utils)
include_directories(include/tradeAlgos)
include_directories(include/indicators)
include_directories(include/triggers)

# source files
file(GLOB MAIN_SRC_FILES src/*.cpp)
file(GLOB MAIN_HEADER_FILES include/*.h include/*.hpp)
file(GLOB UTIL_SRC_FILES src/utils/*.cpp)
file(GLOB UTIL_HEADER_FILES include/utils/*.hpp include/utils/*.h)
file(GLOB EXCHANGE_SRC_FILES src/exchanges/*.cpp)
file(GLOB EXCHANGE_HEADER_FILES include/exchanges/*.hpp include/exchanges/*.h)
file(GLOB TRADEALGO_SRC_FILES src/tradeAlgos/*.cpp)
file(GLOB TRADEALGO_HEADER_FILES include/tradeAlgos/*.hpp include/tradeAlgos/*.h)
file(GLOB INDICATOR_SRC_FILES src/indicators/*.cpp)
file(GLOB INDICATOR_HEADER_FILES include/indicators/*.hpp include/indicators/*.h)
file(GLOB TRIGGER_SRC_FILES src/triggers/*.cpp)
file(GLOB TRIGGER_HEADER_FILES include/triggers/*.hpp include/triggers/*.h)

# Build TraderBot as a static library on Windows: a shared library (DLL) would
# require every symbol used across the DLL boundary to be explicitly exported,
# whereas a static archive links cleanly. On Linux keep the original shared lib.
if(WIN32)
  set(TRADERBOT_LIB_TYPE STATIC)
else()
  set(TRADERBOT_LIB_TYPE SHARED)
endif()

# create executable
add_library(TraderBot ${TRADERBOT_LIB_TYPE} ${MAIN_SRC_FILES}
        ${MAIN_HEADER_FILES}
        ${UTIL_SRC_FILES}
        ${UTIL_HEADER_FILES}
        ${EXCHANGE_SRC_FILES}
        ${EXCHANGE_HEADER_FILES}
        ${INDICATOR_SRC_FILES}
        ${INDICATOR_HEADER_FILES}
        ${TRADEALGO_SRC_FILES}
        ${TRADEALGO_HEADER_FILES}
        ${TRIGGER_SRC_FILES}
        ${TRIGGER_HEADER_FILES})

# force-include the platform shim (Windows); set in the top-level CMakeLists.txt
if(MSVC)
  target_compile_options(TraderBot PRIVATE ${CT_FORCE_INCLUDE})
endif()

# link third party libraries
target_link_libraries(TraderBot PUBLIC
								CURL::libcurl
								OpenSSL::SSL
								OpenSSL::Crypto
								${CRYPTOPP_LIBRARIES}
								${CASSANDRA_LIBRARY}
								${TALIB_LIBRARY}
								Threads::Threads)
