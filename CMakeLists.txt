cmake_minimum_required(VERSION 3.23)
project(Simple-HTTP-Server)
set(SOURCES
        src/main.cpp
        src/server.cpp)
add_executable(Simple-HTTP-Server ${SOURCES})
target_include_directories(Simple-HTTP-Server
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include)