cmake_minimum_required(VERSION 3.15)
project(n_body_problem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_GENERATOR "Ninja")


set(SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

file(GLOB_RECURSE ADDITIONAL_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderers/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/simulators/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/systems/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

list(APPEND SOURCES ${ADDITIONAL_SOURCES})
list(FILTER SOURCES EXCLUDE REGEX ".*CMakeFiles.*")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)
pkg_check_modules(SIGCPP REQUIRED sigc++-3.0)
pkg_check_modules(GLIBMM REQUIRED glibmm-2.68)
pkg_check_modules(CAIROMM REQUIRED cairomm-1.16)
pkg_check_modules(PANGOMM REQUIRED pangomm-2.48)
pkg_check_modules(FFMPEG REQUIRED
    libavformat
    libavcodec
    libavutil
    libswscale
)
pkg_check_modules(FFTW3 REQUIRED fftw3)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${GTKMM_INCLUDE_DIRS})
include_directories(${SIGCPP_INCLUDE_DIRS})
include_directories(${GTK_INCLUDE_DIRS})
include_directories(${GLIBMM_INCLUDE_DIRS})
include_directories(${FFMPEG_INCLUDE_DIRS})
include_directories(${FFTW3_INCLUDE_DIRS})

link_directories(${GTKMM_LIBRARY_DIRS})
link_directories(${FFMPEG_LIBRARY_DIRS})
link_directories(${FFTW3_LIBRARY_DIRS})

add_executable(n_body_sim ${SOURCES})
target_link_libraries(n_body_sim ${GTKMM_LIBRARIES} ${FFMPEG_LIBRARIES} ${FFTW3_LIBRARIES})

add_custom_command(TARGET n_body_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:n_body_sim>
    COMMENT "Copying data files to build directory"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()
