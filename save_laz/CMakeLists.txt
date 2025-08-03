cmake_minimum_required(VERSION 3.10)
project(save_laz)

set(CMAKE_CXX_STANDARD 17)

add_executable(save_laz save_laz.cpp)

target_include_directories(save_laz PRIVATE ../3rd/Livox-SDK2/include)

find_library(LIVOX_SDK_LIB livox_lidar_sdk PATHS ../3rd/Livox-SDK2/build/sdk_core)
find_library(LASZIP_LIB laszip)

if(LIVOX_SDK_LIB)
  target_link_libraries(save_laz PRIVATE ${LIVOX_SDK_LIB})
endif()
if(LASZIP_LIB)
  target_link_libraries(save_laz PRIVATE ${LASZIP_LIB})
endif()

target_link_libraries(save_laz PRIVATE pthread)
