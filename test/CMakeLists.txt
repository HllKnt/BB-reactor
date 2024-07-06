cmake_minimum_required(VERSION 3.29)

project(test)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(server ./server.cpp)
add_executable(client ./client.cpp)

add_subdirectory(localSocket)
target_link_libraries(server localSocket)
target_link_libraries(client localSocket)
