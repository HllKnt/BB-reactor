cmake_minimum_required(VERSION 3.29)

project(test)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(client client.cpp)
add_executable(server server.cpp)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../src LIBSRC)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(sockpp REQUIRED)

target_sources(client PUBLIC ${SRC} ${LIBSRC})
target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc)
target_link_libraries(client PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(client PUBLIC Sockpp::sockpp)

target_sources(server PUBLIC ${SRC} ${LIBSRC})
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc)
target_link_libraries(server PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(server PUBLIC Sockpp::sockpp)
