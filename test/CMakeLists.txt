cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)

project(web_rapid_unittests)

enable_testing()

include(FetchContent)
FetchContent_Declare(
        googletest
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})
include(GoogleTest)

add_executable(web_rapid_unittests
        base.spec.cpp
        template.spec.cpp
        server.spec.cpp

        ../src/file.cpp
        ../src/template.cpp
        ../src/web.cpp
)

file(COPY files DESTINATION ${CMAKE_BINARY_DIR})

target_link_libraries(web_rapid_unittests GTest::gtest_main gmock)
gtest_add_tests(TARGET web_rapid_unittests)