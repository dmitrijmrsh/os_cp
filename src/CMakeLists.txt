set (CMAKE_CXX_STANDARD 20)
cmake_minimum_required(VERSION 3.17)
project(kp)

add_definitions(-lzmq)

add_compile_options(-pthread)

add_executable(client client.cpp)
add_executable(server server.cpp)

target_link_libraries(client zmq)
target_link_libraries(server zmq)