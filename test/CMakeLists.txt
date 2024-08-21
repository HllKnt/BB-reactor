add_executable(client client.cpp)
add_executable(echoServer echoServer.cpp)

find_package(sockpp REQUIRED)

target_link_libraries(client PRIVATE Sockpp::sockpp frame)
target_link_libraries(echoServer PRIVATE Sockpp::sockpp frame)
