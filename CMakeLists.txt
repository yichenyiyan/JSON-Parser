cmake_minimum_required(VERSION 3.10)
project(JSON_PARSER)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置头文件路径
include_directories(include)

# 添加源文件
file(GLOB SOURCES "src/*.c")

# 添加可执行文件
add_executable(test_runner tests/test.c ${SOURCES})
