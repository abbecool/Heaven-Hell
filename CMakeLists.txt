cmake_minimum_required(VERSION 3.15)
project(heavenhell VERSION 0.1 LANGUAGES CXX)

include(CTest)

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

if (BUILD_TESTING)
    function(add_named_tests target prefix)
        foreach(test_name IN LISTS ARGN)
            add_test(NAME ${prefix}_${test_name} COMMAND ${target} ${test_name})
        endforeach()
    endfunction()

    add_executable(vec2_tests
        tests/vec2/test_vec2.cpp
        src/physics/Vec2.cpp
    )
    target_include_directories(vec2_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
    )

    add_named_tests(vec2_tests vec2
        default_constructor
        float_constructor
        integer_constructor
        mixed_constructor
        json_constructor
        json_scalar_constructor
        equality
        greater_equal
        inequality
        addition
        subtraction
        vector_multiplication
        scalar_multiplication
        vector_division
        scalar_division
        vector_modulo
        scalar_modulo
        increment
        decrement
        add_assign
        subtract_assign
        multiply_assign
        divide_assign
        distance
        is_null
        length
        absolute_value
        normalization
        zero_normalization
        scaled_normalization
        angle
        main_direction
        to_integer
        smaller
        greater
        print
        has_positive
        has_negative
        is_positive
        is_negative
        from_json
    )

    add_executable(random_array_tests
        tests/random_array/test_random_array.cpp
        src/physics/RandomArray.cpp
    )
    target_include_directories(random_array_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
    )
    add_named_tests(random_array_tests random_array
        deterministic_seed
        size_and_range
        zero_size
    )

    add_executable(sprite_definition_tests
        tests/sprite_definition/test_sprite_definition.cpp
        src/assets/SpriteDefinition.cpp
        src/physics/Vec2.cpp
    )
    target_include_directories(sprite_definition_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
    )
    add_named_tests(sprite_definition_tests sprite_definition
        texture_size_constructor
        source_region_constructor
        minimum_values
    )

    add_executable(ecs_tests
        tests/ecs/test_ecs.cpp
    )
    target_include_directories(ecs_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
    )
    add_named_tests(ecs_tests ecs
        pool_add_and_get
        pool_replace_existing
        pool_remove_compacts
        pool_move
        entity_lifecycle
        copy_and_view
        queued_removal
    )

    add_executable(render_view_tests
        tests/render/test_render_view.cpp
    )
    target_include_directories(render_view_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
    )
    add_named_tests(render_view_tests render_view
        point_conversion
        rect_conversion
        camera_zoom_origin
    )
endif()
