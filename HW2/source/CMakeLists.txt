cmake_minimum_required(VERSION 3.13)
project(hw2)


set(SOURCES main.cpp)
add_executable(hw2 ${SOURCES})

target_link_libraries(hw2 GL)
target_link_libraries(hw2 glut)
target_link_libraries(hw2 GLEW)