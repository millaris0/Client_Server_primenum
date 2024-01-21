cmake_minimum_required(VERSION 3.26)
project(km_lab2)

set(CMAKE_CXX_STANDARD 17)

add_executable(km_lab2 main.cpp
        server.cpp
        client.cpp)

find_package(Boost REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(km_lab2 ${Boost_LIBRARIES})
