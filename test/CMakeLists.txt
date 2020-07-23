cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Test)

add_executable(Test
    "main.cpp"
    "SystemTest.cpp")

set_property(TARGET Test
    PROPERTY CXX_STANDARD 20)

enable_testing()
add_test(NAME Test COMMAND Test)
