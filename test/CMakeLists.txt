add_executable(server server.cpp)

find_package(sockpp REQUIRED)

target_link_libraries(server PRIVATE Sockpp::sockpp frame)
