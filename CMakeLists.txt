cmake_minimum_required(VERSION 3.15)
project(heavenhell VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32 AND NOT MINGW)
    message(FATAL_ERROR "Windows builds are expected to use MSYS2/MinGW. Configure with -G \"MinGW Makefiles\".")
endif()

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/run/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/run/Debug")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(heavenhell ${SOURCES})

target_include_directories(heavenhell PRIVATE ${CMAKE_SOURCE_DIR}/src)

if (WIN32)
    target_link_libraries(heavenhell PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(heavenhell PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
)

if (WIN32)
    get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    add_custom_command(TARGET heavenhell POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
            "${MINGW_BIN_DIR}/libstdc++-6.dll"
            "${MINGW_BIN_DIR}/libwinpthread-1.dll"
            "${MINGW_BIN_DIR}/SDL2.dll"
            "${MINGW_BIN_DIR}/SDL2_image.dll"
            "${MINGW_BIN_DIR}/SDL2_mixer.dll"
            "${MINGW_BIN_DIR}/SDL2_ttf.dll"
            $<TARGET_FILE_DIR:heavenhell>)
endif()

add_custom_command(TARGET heavenhell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:heavenhell>/assets")

add_custom_command(TARGET heavenhell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/config_files"
        "$<TARGET_FILE_DIR:heavenhell>/config_files")
