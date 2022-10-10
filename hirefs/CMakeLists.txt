cmake_minimum_required(VERSION 2.8)
set(PROJECT_NAME hirefsd)
#set(CMAKE_BUILD_TYPE Debug CACHE STRING "set build type to debug" FORCE)
set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "set build type to MinSizeRel" FORCE)
project(${PROJECT_NAME})

set(STRIP strip)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -static-libstdc++ -Wno-unused-result")
set(CMAKE_SYSTEM_NAME linux)
set(CMAKE_C_COMPILER ${C_COMPILER})
set(CMAKE_CXX_COMPILER ${CPP_COMPILER})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CUSTOM_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CUSTOM_CPPFLAGS}")
# end cross compile options


# secp256k1 library
set(SECP256K1_DIR "../3rd-libraries/secp256k1")
# include files
include_directories (${SECP256K1_DIR}/include)
include_directories (${SECP256K1_DIR}/src)
#source files
set(SECP256K1_SOURCES ${SECP256K1_DIR}/src/secp256k1.c)
add_definitions(-DUSE_BASIC_CONFIG)
add_definitions(-DENABLE_MODULE_ECDH)
add_definitions(-DENABLE_MODULE_RECOVERY)
add_definitions(-DENABLE_MODULE_EXTRAKEYS)
add_definitions(-DENABLE_MODULE_SCHNORRSIG)
add_library (secp256k1-static STATIC ${SECP256K1_SOURCES})
set_target_properties (secp256k1-static PROPERTIES OUTPUT_NAME "secp256k1")


# include directoies
include_directories(./)
include_directories(../common/include)
include_directories(../common/src)
include_directories(../3rd-libraries)

#source
set(SOURCE_FILES ./main.cpp
                 ./websocket_server.hpp
                 ./hireLogManager.cpp
                 ./hireManager.cpp
                 ./hireVolume.cpp
                 ../common/src/blockChainTools.cpp
                 ../common/src/messageHelp.cpp
                 ../common/src/rlpvalue/rlpvalue.cpp
                 ../common/src/rlpvalue/rlpvalue_read.cpp
                 ../common/src/rlpvalue/rlpvalue_write.cpp
                 ../common/src/rlpvalue/rlpvalue_get.cpp
                 ../common/src/httpsession.cpp
                 ../common/src/hmac_sha256.cpp
                 ../common/src/sha256.cpp
                 ../common/src/md5.cpp
                 ../common/src/sha3.cpp
                 ../common/src/pevents.cpp)
AUX_SOURCE_DIRECTORY(. SOURCE_FILES)

add_definitions(-DSECP256K1_SUPPORT)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${STRIP} ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/../3rd-libraries/curl/libcurl.a)
target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/../3rd-libraries/openssl/libssl.a)
target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/../3rd-libraries/openssl/libcrypto.a)
target_link_libraries(${PROJECT_NAME} dl)
target_link_libraries(${PROJECT_NAME} z)
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} secp256k1-static)