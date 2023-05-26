cmake_minimum_required(VERSION 3.1)
project(RTPS VERSION 0.5)
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")

add_subdirectory(3rdParty/colmap)
file(GLOB SOURCES "RTPS/*.cpp" "RTPS/*.h")


set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
add_executable(RTPS ${SOURCES})
target_link_libraries(RTPS colmap)