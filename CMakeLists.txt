cmake_minimum_required(VERSION 3.23)
project(pixsim C)

set(CMAKE_C_STANDARD 99)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
add_executable(pixsim main.c)
target_link_libraries(pixsim PRIVATE SDL2::SDL2)
target_link_libraries(pixsim PRIVATE SDL2_ttf::SDL2_ttf)
