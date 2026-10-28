cmake_minimum_required(VERSION 3.22)
project(heart)
add_library(heart SHARED heart.cpp)
target_link_libraries(heart GLESv2 log)
