cmake_minimum_required(VERSION 3.10)

set(VCPKG_ROOT "vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(cvp)

set(CMAKE_CXX_STANDARD 17)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
find_package(sdl2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)

file(GLOB SRC "src/*.cpp" "src/*.h")

add_executable(cvp ${SRC})

target_link_libraries(cvp)

target_include_directories(cvp PRIVATE ${CURSES_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS})
target_compile_options(cvp PRIVATE ${CURSES_CFLAGS})
target_link_directories(cvp PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(cvp
    PRIVATE
    ${FFMPEG_LIBRARIES}
    fmt::fmt
    ${CURSES_LIBRARIES}
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
