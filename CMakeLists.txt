cmake_minimum_required(VERSION 3.10)

# Project name
project(cvp)

# Set the C standard
set(CMAKE_CXX_STANDARD 14)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# Find packages based on platform
if (WIN32)
    # Use vcpkg or manually specify library paths for Windows
    find_package(SDL2 REQUIRED)
    find_package(FFmpeg REQUIRED)

    include_directories(${SDL2_INCLUDE_DIRS} ${FFmpeg_INCLUDE_DIRS})
    link_directories(${SDL2_LIBRARY_DIRS} ${FFmpeg_LIBRARY_DIRS})

    set(LIBS ${SDL2_LIBRARIES} ${FFmpeg_LIBRARIES})

else()
    # Use pkg-config for Unix-like systems (Linux, macOS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED libswscale libavformat libavutil libavcodec libswresample)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(FMT REQUIRED fmt)

    include_directories(${FFMPEG_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${FMT_INCLUDE_DIRS})
    link_directories(${FFMPEG_LIBRARY_DIRS} ${SDL2_LIBRARY_DIRS} ${FMT_LIBRARY_DIRS})

    set(LIBS ${FFMPEG_LIBRARIES} ${SDL2_LIBRARIES} ${FMT_LIBRARIES} m)
endif()

# Source files
file(GLOB SRC "cpp/*.cpp")

# Add the executable
add_executable(cvp ${SRC})

# Link the executable to the libraries
target_link_libraries(cvp ${LIBS})