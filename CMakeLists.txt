set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(localSocket STATIC)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC)
target_sources(localSocket PRIVATE ${SRC})

target_include_directories(localSocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

find_package(nlohmann_json 3.2.0 REQUIRED)
target_link_libraries(localSocket PUBLIC nlohmann_json::nlohmann_json)
find_package(sockpp REQUIRED)
target_link_libraries(localSocket PUBLIC Sockpp::sockpp)
