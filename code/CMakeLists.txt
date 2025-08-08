cmake_minimum_required(VERSION 3.16)
project(save_laz)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)
find_library(LASZIP_LIB NAMES laszip REQUIRED)

add_executable(save_laz main.cpp save_laz.cpp)

target_include_directories(save_laz PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(save_laz PRIVATE nlohmann_json::nlohmann_json ${LASZIP_LIB})
