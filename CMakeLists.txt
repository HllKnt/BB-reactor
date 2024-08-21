cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(reactor)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_library(frame STATIC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC)
target_sources(frame PRIVATE ${SRC})
target_include_directories(frame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

add_subdirectory(test)
