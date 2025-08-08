cmake_minimum_required(VERSION 3.14)
project(save_laz)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- nlohmann_json ---
include(FetchContent)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

# --- LASzip ---
# Get the parent of the current source dir
get_filename_component(PARENT_SOURCE_DIR "${CMAKE_SOURCE_DIR}" DIRECTORY)

# Set the path to LASzip source (outside current CMake source tree)
set(LASZIP_SRC_DIR "${PARENT_SOURCE_DIR}/3rd/LASzip")

# Create a separate binary dir for LASzip to avoid conflicts
set(LASZIP_BIN_DIR "${CMAKE_BINARY_DIR}/_laszip_build")

# Add LASzip as a subdirectory with its own binary directory.
# This makes the laszip and laszip_api targets available for linking.
add_subdirectory("${LASZIP_SRC_DIR}" "${LASZIP_BIN_DIR}")

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
  imu_writer.cpp)

target_include_directories(save_laz_utils PUBLIC
  "${LASZIP_SRC_DIR}/include"
  "${LASZIP_BIN_DIR}/include"
  ${LIVOX_SDK2_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(save_laz_utils PUBLIC
  nlohmann_json::nlohmann_json
  laszip_api
  laszip
  ${LIVOX_SDK2_LIBRARY}
  pthread
  dl)

add_executable(save_laz
  main.cpp)

target_link_libraries(save_laz PRIVATE save_laz_utils)
