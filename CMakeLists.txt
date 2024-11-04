cmake_minimum_required(VERSION 3.10)

# Project name
project(cvp)

# Set the C standard
set(CMAKE_CXX_STANDARD 17)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# Use pkg-config for Unix-like systems (Linux, macOS)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libswscale libavformat libavutil libavcodec libswresample)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(FMT REQUIRED fmt)

include_directories(${FFMPEG_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${FMT_INCLUDE_DIRS})
link_directories(${FFMPEG_LIBRARY_DIRS} ${SDL2_LIBRARY_DIRS} ${FMT_LIBRARY_DIRS})

set(LIBS ${FFMPEG_LIBRARIES} ${SDL2_LIBRARIES} ${FMT_LIBRARIES} m)

# Source files
file(GLOB SRC "src/*.cpp" "src/*.h")

# Add the executable
add_executable(cvp ${SRC})

# Link the executable to the libraries
target_link_libraries(cvp ${LIBS})
