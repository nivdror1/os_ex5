cmake_minimum_required(VERSION 3.6)
project(os_ex5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wextra -Wall ")

set(SOURCE_FILES whatsappClient.cpp)
add_executable(os_ex5 ${SOURCE_FILES})