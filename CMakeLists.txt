cmake_minimum_required(VERSION 3.10)
project(cpu-monitor LANGUAGES CXX)

add_executable(cpu-monitor WIN32 main.cpp)

if(MSVC)
    target_compile_options(cpu-monitor PRIVATE /utf-8)
    target_compile_definitions(cpu-monitor PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
