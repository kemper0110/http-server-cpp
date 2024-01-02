cmake_minimum_required(VERSION 3.26)
project(http-server-cpp)

set(CMAKE_CXX_STANDARD 23)

add_executable(http-server-cpp
        src/main.cpp
        src/Server.cpp src/Controller.cpp src/StaticMiddleware.cpp
        src/utils.cpp
)
include_directories(include)