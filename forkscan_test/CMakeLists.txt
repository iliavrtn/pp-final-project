cmake_minimum_required(VERSION 3.22)
project(td_test C)

set(CMAKE_C_STANDARD 11)

#set(CMAKE_BUILD_RPATH "/home/ivorotyntsev/CLionProjects/ThreadScanChanged/cmake-build-debug")
#set(CMAKE_INSTALL_RPATH "/home/ivorotyntsev/CLionProjects/ThreadScanChanged/cmake-build-debug")

add_executable(main main.c)

target_link_libraries(main PRIVATE /usr/local/lib/libforkscan.so)