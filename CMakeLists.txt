cmake_minimum_required(VERSION 3.0)
project(lambda-erasure)

add_executable(lambda-erasure
lambda-erasure.cpp
)

set(CXX_STANDARD_REQUIRED ON)

set_property(TARGET lambda-erasure PROPERTY CXX_STANDARD 17)