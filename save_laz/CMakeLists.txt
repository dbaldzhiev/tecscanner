cmake_minimum_required(VERSION 3.10)
project(save_laz)
set(CMAKE_CXX_STANDARD 17)

add_executable(save_laz save_laz.cpp)

# ---------------- 3rd-party: LASzip ----------------
set(LASZIP_SRC "${CMAKE_SOURCE_DIR}/../3rd/LASzip")
add_subdirectory("${LASZIP_SRC}" "${CMAKE_BINARY_DIR}/LASzip-build")

# ---------------- 3rd-party: Livox SDK2 ----------------
set(LIVOX_SDK_SRC "${CMAKE_SOURCE_DIR}/../3rd/Livox-SDK2")
add_subdirectory("${LIVOX_SDK_SRC}" "${CMAKE_BINARY_DIR}/Livox-SDK2-build")

# If these targets exist, they already carry proper include paths
# (matching the example you sent which links laszip + livox static)
if(TARGET laszip)
  target_link_libraries(save_laz PRIVATE laszip)
elseif(TARGET LASzip)
  target_link_libraries(save_laz PRIVATE LASzip)
else()
  message(FATAL_ERROR "LASzip target not found after add_subdirectory()")
endif()

if(TARGET livox_lidar_sdk_static)
  target_link_libraries(save_laz PRIVATE livox_lidar_sdk_static)
elseif(TARGET livox_lidar_sdk)
  target_link_libraries(save_laz PRIVATE livox_lidar_sdk)
else()
  message(FATAL_ERROR "Livox SDK target not found after add_subdirectory()")
endif()

# Threads + atomic (often needed on Linux/ARM)
find_package(Threads REQUIRED)
target_link_libraries(save_laz PRIVATE Threads::Threads atomic)

# Optional: RPATH so the built binary can find Livox .so from build tree (if using shared)
# get_target_property(_LIVOX_LIB livox_lidar_sdk LOCATION)
# if(_LIVOX_LIB)
#   get_filename_component(_LIVOX_DIR "${_LIVOX_LIB}" DIRECTORY)
#   set_target_properties(save_laz PROPERTIES BUILD_RPATH "${_LIVOX_DIR}" INSTALL_RPATH "${_LIVOX_DIR}")
# endif()
