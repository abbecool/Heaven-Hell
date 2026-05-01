cmake_minimum_required(VERSION 3.21)
project(heavenhell VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32 AND MINGW)
    if (DEFINED ENV{MSYSTEM_PREFIX})
        list(APPEND CMAKE_PREFIX_PATH "$ENV{MSYSTEM_PREFIX}")
    elseif (EXISTS "C:/msys64/ucrt64")
        list(APPEND CMAKE_PREFIX_PATH "C:/msys64/ucrt64")
    endif()
elseif (WIN32 AND MSVC)
    set(HEAVENHELL_SDL_ROOT "${CMAKE_SOURCE_DIR}/../SDL2" CACHE PATH
        "Folder containing SDL2 Visual C++ development packages on Windows")

    if (EXISTS "${HEAVENHELL_SDL_ROOT}")
        list(APPEND CMAKE_PREFIX_PATH
            "${HEAVENHELL_SDL_ROOT}/SDL2-2.32.8"
            "${HEAVENHELL_SDL_ROOT}/SDL2_image-2.8.8"
            "${HEAVENHELL_SDL_ROOT}/SDL2_mixer-2.8.1"
            "${HEAVENHELL_SDL_ROOT}/SDL2_ttf-2.24.0"
        )
    endif()
endif()

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(heavenhell ${SOURCES})

target_include_directories(heavenhell PRIVATE ${CMAKE_SOURCE_DIR}/src)

set_target_properties(heavenhell PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/run/$<CONFIG>"
)

if (WIN32 AND TARGET SDL2::SDL2main)
    target_link_libraries(heavenhell PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(heavenhell PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
)

add_custom_command(TARGET heavenhell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:heavenhell>/assets")

add_custom_command(TARGET heavenhell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/config_files"
        "$<TARGET_FILE_DIR:heavenhell>/config_files")

if (WIN32)
    add_custom_command(TARGET heavenhell POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:heavenhell>
            $<TARGET_FILE_DIR:heavenhell>
        COMMAND_EXPAND_LISTS)
endif()
