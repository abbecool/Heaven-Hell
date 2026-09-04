cmake_minimum_required(VERSION 3.15)
project(heavenhell VERSION 0.1 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HEAVENHELL_BUILD_GAME "Build the SDL game executable" ON)
option(HEAVENHELL_STRICT_CONVERSIONS "Enable additional conversion warnings" OFF)
option(HEAVENHELL_ENABLE_SANITIZERS "Enable address and undefined-behavior sanitizers" OFF)

if (WIN32 AND NOT MINGW)
    message(FATAL_ERROR "Windows builds are expected to use MSYS2/MinGW. Configure with -G \"MinGW Makefiles\".")
endif()

function(heavenhell_target_defaults target)
    target_compile_features(${target} PUBLIC cxx_std_20)

    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if (HEAVENHELL_STRICT_CONVERSIONS)
            target_compile_options(${target} PRIVATE -Wconversion -Wsign-conversion)
        endif()
    endif()

    if (HEAVENHELL_ENABLE_SANITIZERS)
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32)
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        else()
            message(FATAL_ERROR "HEAVENHELL_ENABLE_SANITIZERS requires GCC or Clang on a non-Windows build")
        endif()
    endif()
endfunction()

add_library(heavenhell_core STATIC
    src/assets/SpriteDefinition.cpp
    src/debug/EntityInspector.cpp
    src/physics/Quadtree.cpp
    src/physics/RandomArray.cpp
    src/physics/Vec2.cpp
    src/story/StoryManager.cpp
    src/world/EntityCatalog.cpp
    src/world/WorldLayout.cpp
)
target_include_directories(heavenhell_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(heavenhell_core PUBLIC
    HEAVENHELL_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
heavenhell_target_defaults(heavenhell_core)

if (HEAVENHELL_BUILD_GAME)
    find_package(SDL3 CONFIG REQUIRED)
    find_package(SDL3_image CONFIG REQUIRED)
    find_package(SDL3_mixer CONFIG REQUIRED)
    find_package(SDL3_ttf CONFIG REQUIRED)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/run/Release")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/run/Debug")

    set_source_files_properties(src/external/glad/src/glad.c PROPERTIES LANGUAGE CXX)
    add_library(glad STATIC src/external/glad/src/glad.c)
    target_include_directories(glad PUBLIC src/external/glad/include)

    add_library(heavenhell_runtime STATIC
        src/assets/Assets.cpp
        src/core/Action.cpp
        src/core/Game.cpp
        src/core/SDLPlatform.cpp
        src/physics/Camera.cpp
        src/physics/CollisionManager.cpp
        src/physics/Level_Loader.cpp
        src/physics/Physics.cpp
        src/physics/damageTypes.cpp
        src/render/opengl/OpenGLGlyphAtlas.cpp
        src/render/opengl/OpenGLRenderBackend.cpp
        src/render/opengl/OpenGLSpriteBatch.cpp
        src/render/sdl/SDLRenderBackend.cpp
        src/scenes/Scene.cpp
        src/scenes/Scene_Editor.cpp
        src/scenes/Scene_Finish.cpp
        src/scenes/Scene_GameOver.cpp
        src/scenes/Scene_Inventory.cpp
        src/scenes/Scene_Menu.cpp
        src/scenes/Scene_Pause.cpp
        src/scenes/Scene_Play.cpp
    )
    target_include_directories(heavenhell_runtime PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(heavenhell_runtime
        PUBLIC heavenhell_core
        PRIVATE
            SDL3::SDL3
            SDL3_image::SDL3_image
            SDL3_mixer::SDL3_mixer
            SDL3_ttf::SDL3_ttf
            glad
    )
    heavenhell_target_defaults(heavenhell_runtime)

    add_executable(heavenhell src/main.cpp)
    target_link_libraries(heavenhell PRIVATE heavenhell_runtime)
    heavenhell_target_defaults(heavenhell)

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

    add_custom_target(sync_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:heavenhell>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/config_files"
            "$<TARGET_FILE_DIR:heavenhell>/config_files"
        COMMENT "Syncing assets and config files to the run directory")
    add_dependencies(sync_assets heavenhell)
endif()

if (BUILD_TESTING)
    function(add_heavenhell_test target source)
        add_executable(${target} ${source})
        target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/tests)
        target_link_libraries(${target} PRIVATE heavenhell_core)
        heavenhell_target_defaults(${target})
        add_test(NAME ${target} COMMAND ${target})
        set_tests_properties(${target} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endfunction()

    add_heavenhell_test(vec2_tests tests/vec2/test_vec2.cpp)
    add_heavenhell_test(random_array_tests tests/random_array/test_random_array.cpp)
    add_heavenhell_test(sprite_definition_tests tests/sprite_definition/test_sprite_definition.cpp)
    add_heavenhell_test(ecs_tests tests/ecs/test_ecs.cpp)
    add_heavenhell_test(render_view_tests tests/render/test_render_view.cpp)
    add_heavenhell_test(story_manager_tests tests/story/test_story_manager.cpp)
    add_heavenhell_test(world_layout_tests tests/world/test_world_layout.cpp)
    add_heavenhell_test(entity_catalog_tests tests/world/test_entity_catalog.cpp)
endif()
