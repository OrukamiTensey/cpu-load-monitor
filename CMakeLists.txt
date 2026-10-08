cmake_minimum_required(VERSION 3.14)
project(cpu_load_monitor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Werror -O2)

include_directories(include)

add_executable(cpu_monitor
    src/main.cpp
    src/CpuMonitor.cpp
    src/CpuStatParser.cpp
)