cmake_minimum_required(VERSION 3.10)

# Build options
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Werror -march=native -mtune=native")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
else()
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -03")
endif ()

# Building
project(KiTraining VERSION 0.0.1 DESCRIPTION "Train program for Sopra AI")

set(SOURCES
        ${CMAKE_SOURCE_DIR}/src/Game/Game.cpp
        ${CMAKE_SOURCE_DIR}/src/Game/MemberSelector.cpp
        ${CMAKE_SOURCE_DIR}/src/Game/PhaseManager.cpp
        ${CMAKE_SOURCE_DIR}/src/Game/ConfigCheck.cpp)

set(LIBS pthread stdc++fs SopraGameLogic SopraMessages SopraUtil)

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable(${PROJECT_NAME} src/main.cpp ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBS})
