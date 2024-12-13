cmake_minimum_required(VERSION 3.29)
project(chat_project C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
set(CMAKE_C_FLAGS_DEBUG "-g0")


include_directories(.)
include_directories(chat_project)

add_executable(client chat_project/client.c)
target_link_libraries(client ws2_32)

add_executable(server chat_project/server.c)
target_link_libraries(server ws2_32)

add_executable(test chat_project/test.c)
target_link_libraries(test ws2_32)




