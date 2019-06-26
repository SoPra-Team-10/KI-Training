cmake_minimum_required(VERSION 3.10)

# Build options
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Werror -Wno-implicit-fallthrough")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
endif ()

# Building
project(Server VERSION 0.0.1 DESCRIPTION "Train program for Sopra AI")

set(SOURCES
        ${CMAKE_SOURCE_DIR}/src/Util/ArgumentParser.cpp
        ${CMAKE_SOURCE_DIR}/src/Game/Game.cpp
        ${CMAKE_SOURCE_DIR}/src/Game/MemberSelector.cpp
        ${CMAKE_SOURCE_DIR}/src/Game/PhaseManager.cpp
        ${CMAKE_SOURCE_DIR}/src/Game/ConfigCheck.cpp)

set(LIBS pthread stdc++fs SopraGameLogic SopraMessages SopraUtil)

if (USE_LOCAL_NETWORK)
    include_directories(../Network/)
    include_directories(../Network/Server)
    set(SOURCES ${SOURCES}
            ${CMAKE_SOURCE_DIR}/../Network/Server/Connection.cpp
            ${CMAKE_SOURCE_DIR}/../Network/Server/WebSocketServer.cpp)
    set(LIBS ${LIBS} websockets)
else()
    set(LIBS ${LIBS} SopraNetwork)
endif()

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable(${PROJECT_NAME} src/main.cpp ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBS})
