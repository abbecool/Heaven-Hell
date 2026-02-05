cmake_minimum_required(VERSION 3.15)
project(heavenhell VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/run")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(heavenhell ${SOURCES})

target_include_directories(heavenhell PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(heavenhell PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
)

if (WIN32)
    target_link_libraries(heavenhell PRIVATE SDL2::SDL2main)
endif()

add_custom_command(TARGET heavenhell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:heavenhell>/assets")

add_custom_command(TARGET heavenhell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/config_files"
        "$<TARGET_FILE_DIR:heavenhell>/config_files")
