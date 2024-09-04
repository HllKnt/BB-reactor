add_executable(server server.cpp)
add_executable(echo echo.cpp)
add_executable(pingpang pingpang.cpp)

find_package(sockpp REQUIRED)

target_link_libraries(server PRIVATE Sockpp::sockpp frame)
target_link_libraries(echo PRIVATE Sockpp::sockpp frame)
target_link_libraries(pingpang PRIVATE Sockpp::sockpp frame)
