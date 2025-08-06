cmake_minimum_required(VERSION 3.10)
project(save_laz)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- LASzip ---
find_path(LASZIP_INCLUDE_DIR
          NAMES laszip/laszip_api.h
          PATHS
            ${CMAKE_SOURCE_DIR}/../3rd/LASzip/include
            /usr/include
            /usr/local/include)

find_library(LASZIP_API_LIBRARY
             NAMES laszip_api
             PATHS
               ${CMAKE_SOURCE_DIR}/../3rd/LASzip/lib
               /usr/lib
               /usr/lib/x86_64-linux-gnu
               /usr/local/lib)

if(NOT LASZIP_INCLUDE_DIR OR NOT LASZIP_API_LIBRARY)
  message(FATAL_ERROR "LASzip library not found")
endif()

# --- Livox SDK2 ---
find_path(LIVOX_SDK2_INCLUDE_DIR
          NAMES livox_lidar_api.h livox_lidar_def.h
          PATHS
            ${CMAKE_SOURCE_DIR}/../3rd/Livox-SDK2/include
            /usr/local/include
            /usr/include)

find_library(LIVOX_SDK2_LIBRARY
             NAMES livox_lidar_sdk_shared livox_lidar_sdk
             PATHS
               ${CMAKE_SOURCE_DIR}/../3rd/Livox-SDK2/lib
               /usr/local/lib
               /usr/lib
               /usr/lib/x86_64-linux-gnu)

if(NOT LIVOX_SDK2_INCLUDE_DIR OR NOT LIVOX_SDK2_LIBRARY)
  message(FATAL_ERROR "Livox SDK2 not found")
endif()

# --- Executable ---
add_executable(save_laz save_laz.cpp)

target_include_directories(save_laz PRIVATE
  ${LASZIP_INCLUDE_DIR}
  ${LIVOX_SDK2_INCLUDE_DIR})

target_link_libraries(save_laz PRIVATE
  ${LASZIP_API_LIBRARY}
  ${LIVOX_SDK2_LIBRARY}
  pthread
  dl)

