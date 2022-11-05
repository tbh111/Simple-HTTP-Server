cmake_minimum_required(VERSION 3.23)
project(Simple-HTTP-Server)
set(SOURCES
        src/main.cpp
        src/server.cpp)
set(CMAKE_CXX_STANDARD 11)
add_executable(Simple-HTTP-Server ${SOURCES})
target_include_directories(Simple-HTTP-Server
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include)
find_package(OpenSSL)
target_link_libraries(Simple-HTTP-Server PRIVATE OpenSSL::SSL)
target_link_libraries(Simple-HTTP-Server PRIVATE OpenSSL::Crypto)
target_link_libraries(Simple-HTTP-Server PRIVATE  -lpthread -lm)

