cmake_minimum_required(VERSION 3.10)
project(save_laz)
set(CMAKE_CXX_STANDARD 17)

add_executable(save_laz save_laz.cpp)

# --- Livox SDK ---
target_include_directories(save_laz PRIVATE ../3rd/Livox-SDK2/include)
find_library(LIVOX_SDK_LIB livox_lidar_sdk PATHS ../3rd/Livox-SDK2/build/sdk_core)
if(LIVOX_SDK_LIB)
  target_link_libraries(save_laz PRIVATE ${LIVOX_SDK_LIB})
endif()

# --- LASzip (set root, allow override) ---
# Change this if LASzip lives elsewhere:
set(LASZIP_ROOT "$ENV{HOME}/LAStools/LASzip" CACHE PATH "Path to LASzip root")

# Headers: we need both the 'include' (where laszip_api.h is) and 'src' (for laszip_common.h)
target_include_directories(save_laz PRIVATE
  "${LASZIP_ROOT}/include"
  "${LASZIP_ROOT}/src"
)

# Library (built locally, not system-wide)
set(LASZIP_LIBRARY "${LASZIP_ROOT}/build/src/liblaszip.a")
if(EXISTS "${LASZIP_LIBRARY}")
  target_link_libraries(save_laz PRIVATE "${LASZIP_LIBRARY}")
else()
  message(WARNING "LASzip library not found at ${LASZIP_LIBRARY}. Build it with: 
    cd ${LASZIP_ROOT} && mkdir -p build && cd build && cmake .. && make -j")
endif()

# Threads
target_link_libraries(save_laz PRIVATE pthread)