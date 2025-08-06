cmake_minimum_required(VERSION 3.10)
project(save_laz)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# LASzip paths relative to root (because we're now called from root)
set(LASZIP_ROOT_DIR "${CMAKE_SOURCE_DIR}/3rd/LASzip")
set(LASZIP_INCLUDE_DIR "${LASZIP_ROOT_DIR}/include") # if headers are in dll/
set(LASZIP_LIBRARY_DIR "${LASZIP_ROOT_DIR}/lib")


# Livox SDK2
find_path(LIVOX_SDK2_INCLUDE_DIR
  NAMES livox_lidar_api.h livox_lidar_def.h
  PATHS /usr/local/include /usr/include
  REQUIRED)

find_library(LIVOX_SDK2_LIBRARY
  NAMES livox_lidar_sdk_shared livox_lidar_sdk
  PATHS /usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu
  REQUIRED)

# Executable
add_executable(save_laz save_laz.cpp)

# Includes
target_include_directories(save_laz PRIVATE
  ${LASZIP_INCLUDE_DIR}
  ${LIVOX_SDK2_INCLUDE_DIR}
)

# Link libs
target_link_libraries(save_laz PRIVATE
  ${LASZIP_LIBRARY}
  ${LIVOX_SDK2_LIBRARY}
  pthread
  dl
)
