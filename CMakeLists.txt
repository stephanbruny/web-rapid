cmake_minimum_required(VERSION 3.0)
project(web_rapid)

set(CMAKE_CXX_STANDARD 17)

add_executable(web_rapid
    src/template.cpp
    src/file.cpp
    src/main.cpp
)

enable_testing()
add_subdirectory(test)
