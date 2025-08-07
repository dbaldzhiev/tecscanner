cmake_minimum_required(VERSION 3.10)
project(save_laz)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- LASzip ---
set(LASZIP_ROOT "${CMAKE_SOURCE_DIR}/../3rd/LASzip")

find_path(LASZIP_API_INCLUDE_DIR
          NAMES laszip_api.h
          PATHS
            ${LASZIP_ROOT}/dll
            /usr/include
            /usr/include/laszip
            /usr/local/include
            /usr/local/include/laszip)

find_path(LASZIP_COMMON_INCLUDE_DIR
          NAMES laszip_common.h
          PATHS
            ${LASZIP_ROOT}/include/laszip
            /usr/include
            /usr/include/laszip
            /usr/local/include
            /usr/local/include/laszip)

find_library(LASZIP_API_LIBRARY
             NAMES laszip_api
             PATHS
               ${LASZIP_ROOT}/lib
               ${LASZIP_ROOT}/build/lib
               /usr/lib
               /usr/lib/x86_64-linux-gnu
               /usr/local/lib)

find_library(LASZIP_LIBRARY
             NAMES laszip
             PATHS
               ${LASZIP_ROOT}/lib
               ${LASZIP_ROOT}/build/lib
               /usr/lib
               /usr/lib/x86_64-linux-gnu
               /usr/local/lib)

if(NOT LASZIP_API_INCLUDE_DIR OR NOT LASZIP_COMMON_INCLUDE_DIR OR
   NOT LASZIP_API_LIBRARY OR NOT LASZIP_LIBRARY)
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

# --- Library and executable ---
add_library(save_laz_utils
  save_laz.cpp
  livox_collector.cpp
  csv_writer.cpp)

target_include_directories(save_laz_utils PUBLIC
  ${LASZIP_API_INCLUDE_DIR}
  ${LASZIP_COMMON_INCLUDE_DIR}
  ${LIVOX_SDK2_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(save_laz_utils PUBLIC
  ${LASZIP_API_LIBRARY}
  ${LASZIP_LIBRARY}
  ${LIVOX_SDK2_LIBRARY}
  pthread
  dl)

add_executable(save_laz
  main.cpp)

target_link_libraries(save_laz PRIVATE save_laz_utils)
