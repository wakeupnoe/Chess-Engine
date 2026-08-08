cmake_minimum_required(VERSION 3.23)

project(Chess-Engine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(Catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.4.0)
FetchContent_MakeAvailable(Catch2)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)