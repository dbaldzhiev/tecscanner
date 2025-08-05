cmake_minimum_required(VERSION 3.12)
project(save_laz CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- LASzip (C API) ---
find_library(LASZIP_API_LIBRARY
  NAMES laszip_api
  PATHS /usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu
  REQUIRED)
find_path(LASZIP_INCLUDE_DIR
  NAMES laszip_api.h laszip_common.h
  PATHS /usr/local/include /usr/include
  PATH_SUFFIXES laszip
  REQUIRED)

# --- Livox SDK2 ---
# Headers: livox_lidar_api.h, livox_lidar_def.h
find_path(LIVOX_SDK2_INCLUDE_DIR
  NAMES livox_lidar_api.h livox_lidar_def.h
  PATHS /usr/local/include /usr/include
  REQUIRED)

# Library: liblivox_lidar_sdk_shared.so (preferred)
find_library(LIVOX_SDK2_LIBRARY
  NAMES livox_lidar_sdk_shared livox_lidar_sdk
  PATHS /usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu
  REQUIRED)

message(STATUS "Found LASzip: ${LASZIP_API_LIBRARY}")
message(STATUS "Found LASzip include dir: ${LASZIP_INCLUDE_DIR}")
message(STATUS "Found Livox SDK2: ${LIVOX_SDK2_LIBRARY}")
message(STATUS "Found Livox SDK2 include dir: ${LIVOX_SDK2_INCLUDE_DIR}")

add_executable(save_laz save_laz.cpp)

target_include_directories(save_laz PRIVATE
  ${LASZIP_INCLUDE_DIR}
  ${LIVOX_SDK2_INCLUDE_DIR})

# pthread and dl are commonly required on Linux
target_link_libraries(save_laz PRIVATE
  ${LASZIP_API_LIBRARY}
  ${LIVOX_SDK2_LIBRARY}
  pthread
  dl)

# Ensure the built binary can locate /usr/local/lib at runtime
set_target_properties(save_laz PROPERTIES
  BUILD_RPATH "/usr/local/lib"
  INSTALL_RPATH "/usr/local/lib")
