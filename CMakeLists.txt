cmake_minimum_required(VERSION 3.0)

project(vulkan)

add_executable(vulkan main.cpp)

install(TARGETS vulkan RUNTIME DESTINATION bin)
