cmake_minimum_required(VERSION 3.6)
project(os_ex5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wextra -Wall ")

set(SOURCE_FILES whatsappClient.cpp)
add_executable(Server whatsappServer.cpp)

add_executable(Tomer whatsappClient.cpp)
add_executable(Yarin whatsappClient.cpp)
add_executable(Ido whatsappClient.cpp)
add_executable(Niv whatsappClient.cpp)
add_executable(Afek whatsappClient.cpp)
add_executable(ben whatsappClient.cpp)