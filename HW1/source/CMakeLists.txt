cmake_minimum_required(VERSION 3.13)
project(hw1)


set(SOURCES main.cpp)
add_executable(hw1 ${SOURCES})

target_link_libraries(hw1 GL)
target_link_libraries(hw1 glut)
target_link_libraries(hw1 GLEW)