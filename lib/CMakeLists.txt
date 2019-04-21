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

# create executable
add_library(TraderBot SHARED ${MAIN_SRC_FILES}
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

# link third party libraries
target_link_libraries(TraderBot ${CURL_LIBRARIES}
								${OPENSSL_SSL_LIBRARY}
								${OPENSSL_CRYPTO_LIBRARY}
								cryptopp
								cassandra
								ta_lib)
