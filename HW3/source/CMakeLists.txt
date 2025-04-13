cmake_minimum_required(VERSION 3.13)
project(hw3)


set(SOURCES main.cpp)
add_executable(hw3 ${SOURCES})

target_link_libraries(hw3 GL)
target_link_libraries(hw3 glut)
target_link_libraries(hw3 GLEW)