cmake_minimum_required(VERSION 3.15)
project(heavenhell VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32 AND NOT MINGW)
    message(FATAL_ERROR "Windows builds are expected to use MSYS2/MinGW. Configure with -G \"MinGW Makefiles\".")
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/run/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/run/Debug")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(heavenhell ${SOURCES})

target_include_directories(heavenhell PRIVATE ${CMAKE_SOURCE_DIR}/src)

set_source_files_properties(src/external/glad/src/glad.c PROPERTIES LANGUAGE CXX)
add_library(glad STATIC
    src/external/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    src/external/glad/include
)

target_link_libraries(heavenhell PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
    SDL3_ttf::SDL3_ttf
    glad
)

if (WIN32)
    get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    add_custom_command(TARGET heavenhell POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
            "${MINGW_BIN_DIR}/libstdc++-6.dll"
            "${MINGW_BIN_DIR}/libwinpthread-1.dll"
            "${MINGW_BIN_DIR}/SDL3.dll"
            "${MINGW_BIN_DIR}/SDL3_image.dll"
            "${MINGW_BIN_DIR}/SDL3_mixer.dll"
            "${MINGW_BIN_DIR}/SDL3_ttf.dll"
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
