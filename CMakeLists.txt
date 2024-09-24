include(FetchContent)

cmake_minimum_required(VERSION 3.16)

# set(CMAKE_VERBOSE_MAKEFILE ON)

# force universal binary on osx (must go BEFORE project call)
# TODO: universal binary not working with webgpu-distribution
# set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
# # specify minimum support macOS version
# if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
#     # If no deployment target has been set default to the minimum supported
#     # OS version (this has to be set before the first project() call)
#     if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
#         set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "iOS Deployment Target")
#     else()
#         set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10 CACHE STRING "macOS Deployment Target")
#     endif()
# endif()

project(
    ChuGL
    VERSION 0.1.0
    LANGUAGES CXX C
)

if(LINUX OR EMSCRIPTEN)
    add_compile_options(-fPIC)  # needed for emcripten
    #set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
endif()


set(
    CORE 
    src/core/log.c
    src/core/hashmap.c
    src/core/memory.cpp
)

set(
    VENDOR
    src/implementations.cpp
    vendor/mikktspace/mikktspace.c

    # cimgui (no freetype )
    vendor/cimgui/cimgui.cpp
    vendor/cimgui/imgui/imgui.cpp
    vendor/cimgui/imgui/imgui_draw.cpp
    vendor/cimgui/imgui/imgui_demo.cpp
    vendor/cimgui/imgui/imgui_widgets.cpp
	vendor/cimgui/imgui/imgui_tables.cpp
    vendor/cimgui/imgui/misc/cpp/imgui_stdlib.cpp
    vendor/cimgui/imgui/backends/imgui_impl_glfw.cpp
    vendor/cimgui/imgui/backends/imgui_impl_wgpu.cpp
)

# chugl library
if (EMSCRIPTEN)
    # emcmake can't handle shared libraries. SIDE_LOAD executable is the only option
    add_executable(
        ${PROJECT_NAME} 
        src/chugl.cpp 
        ${CORE}
        ${VENDOR}
    )
else()
    add_library(
        ${PROJECT_NAME} SHARED 
        src/chugl.cpp 
        # src/ulib_imgui.cpp

        ${CORE}
        ${VENDOR}
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
    VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
    POSITION_INDEPENDENT_CODE ON
    PREFIX ""
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX ".chug"
)


if(XCODE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
        XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

# =================================================================================
# interface library for CMake properties
# =================================================================================
add_library(chugl_shared_properties INTERFACE)

# Defines
target_compile_definitions(chugl_shared_properties INTERFACE 
    $<$<CONFIG:Debug>:CHUGL_DEBUG>
    $<$<CONFIG:Release>:CHUGL_RELEASE>
    GLM_FORCE_DEPTH_ZERO_TO_ONE # glm force depth range to 0-1
    GLM_ENABLE_EXPERIMENTAL
    LOG_USE_COLOR
)

# increasing warning levels
if (MSVC)
    target_compile_options(chugl_shared_properties INTERFACE /W4)
else()
    target_compile_options(chugl_shared_properties INTERFACE -Wall -Wextra -pedantic)

    # disable warnings
    target_compile_options(chugl_shared_properties INTERFACE 
        -Wno-unused-parameter
        # in release mode only, allowed unused params (to get around ASSERT)
        $<$<CONFIG:Release>:-Wno-unused-variable>
        $<$<CONFIG:Release>:-Wno-unused-function>

        -Wno-deprecated-declarations
        -Wno-compound-token-split-by-macro
        -Wno-sign-compare
        -Wno-unused-but-set-variable
    )
endif()

# Ignore Windows warnings
if (MSVC)
	target_compile_options(
        chugl_shared_properties INTERFACE
        # Ignore a warning that GLM requires to bypass
        # Disable warning C4201: nonstandard extension used: nameless struct/union
        /wd4201
        # Required for GLM lookat()
        # Disable warning C4127: conditional expression is constant
        /wd4127
        # Disable warning C4305: truncation from 'int' to 'bool' in 'if' condition
        /wd4305
        # Ignore a warning that stb_image requires to bypass
        # Disable warning C4244: conversion from 'int' to 'short', possible loss of data
        /wd4244
        # ignore fopen warning for cgltf
        /wd4996
        # ignore warning for using 'bitwise and' to check if flag is set
        /wd26813
        # ignore hiding of previous local declaration
        /wd4456
        # ignore unused local variables
        /wd4189
        # ignore uninitialized member variables
        /wd26495 
    )
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
        # allow unused variables for the ChuGin DLL Query
        /wd4100
    )
endif (MSVC)


if ( MSVC )
    # Set the startup project to be the actual project instead of ALL_BUILD 
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

    set(CHUCK_EXECUTABLE_DEBUG "${CMAKE_SOURCE_DIR}/chuck-debug.exe")
    set(CHUCK_EXECUTABLE_RELEASE "${CMAKE_SOURCE_DIR}/chuck-release.exe")
    set(CHUGIN_PATH_DEBUG "--chugin-path:${CMAKE_TARGET_DIR}./build/Debug")
    set(CHUGIN_PATH_RELEASE "--chugin-path:${CMAKE_TARGET_DIR}./build/Release")
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        # VS_DEBUGGER_COMMAND $<$<CONFIG:Debug>:${CHUCK_EXECUTABLE_DEBUG}>$<$<CONFIG:Release>:${CHUCK_EXECUTABLE_RELEASE}>
        VS_DEBUGGER_COMMAND "C:/Program Files/ChucK/chuck.exe"

        VS_DEBUGGER_COMMAND_ARGUMENTS "$<$<CONFIG:Debug>:${CHUGIN_PATH_DEBUG}>$<$<CONFIG:Release>:${CHUGIN_PATH_RELEASE}> --chugin-probe"

        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # static linking with CRT
endif()

# add include path
target_include_directories(chugl_shared_properties 
    INTERFACE src
    INTERFACE vendor
    INTERFACE vendor/cimgui
    INTERFACE vendor/cimgui/imgui
    INTERFACE vendor/cimgui/imgui
    INTERFACE vendor/box2d/include
)

# vendor dependencies ==========================================================

if (NOT EMSCRIPTEN)
    add_subdirectory(vendor/glfw)
endif()
add_subdirectory(vendor/glfw3webgpu)

# host our own distribution of WebGPU on CCRMA
# switching to static library instead of dynami b/c
# chuck cannot find build/Debug/webgpu_native.dll.
# throws Error [126]: The specified module could not be found.
# i.e. ChuGL DLL needs wgpu DLL which cannot be not found.
# will need to fix this to implement ChuGLins
FetchContent_Declare(
  webgpu
  GIT_REPOSITORY https://github.com/AndrewAday/webgpu-distribution
  GIT_TAG        main
)
FetchContent_MakeAvailable(webgpu)

# FreeType font rendering
# set default freetype options
set(FT_DISABLE_BROTLI   ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB     ON  CACHE BOOL "" FORCE)
set(FT_ENABLE_ERROR_STRINGS ON CACHE BOOL "" FORCE)
add_subdirectory(vendor/freetype)
# # fetch freetype
# FetchContent_Declare (
#     freetype 
#     GIT_REPOSITORY https://github.com/freetype/freetype.git
#     GIT_TAG VER-2-13-2
# )
# make available
# FetchContent_MakeAvailable (freetype)

# box2d v3.0
# FetchContent_Declare(
#   box2d
#   GIT_REPOSITORY https://github.com/erincatto/box2c
#   GIT_TAG        main
# )
# FetchContent_MakeAvailable(box2d)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
	option(BOX2D_AVX2 "Enable AVX2 (faster)" ON)
endif()

# Needed for samples.exe to find box2d.dll
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# The Box2D library uses simde https://github.com/simd-everywhere/simde
add_subdirectory(vendor/simde)
add_subdirectory(vendor/box2d/src)

# linking
# target_link_libraries(${PROJECT_NAME} PRIVATE webgpu glfw glfw3webgpu)
target_link_libraries(chugl_shared_properties INTERFACE webgpu glfw glfw3webgpu box2d freetype)
target_link_libraries(${PROJECT_NAME} PRIVATE chugl_shared_properties)

# emscripten specific options =================================================
if (EMSCRIPTEN)
    # know to rebuild when assets/ directory changes
    # set(SHELL_FILE shell_minimal.html)
    # set_property(
    #     TARGET ${PROJECT_NAME}
    #     PROPERTY LINK_DEPENDS
    #     ${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}
    # )
    # set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
    target_compile_options(${PROJECT_NAME} PRIVATE -pthread)

    target_link_options(${PROJECT_NAME} PRIVATE -sENVIRONMENT=worker)
    target_link_options(${PROJECT_NAME} PRIVATE -pthread)

    # Add Emscripten-specific link options
    target_link_options(${PROJECT_NAME} PRIVATE
        -sUSE_GLFW=3 # Use Emscripten-provided GLFW
        -sUSE_WEBGPU # Handle WebGPU symbols
        -sALLOW_MEMORY_GROWTH # allow memory to grow dynamically
        -sSIDE_MODULE=1  # needed for chuck to dynamically load the chugin

        # add thread support
        # -pthread

        # removing because ASYNCIFY not supported in side module
        # using infinite loop instead
        # -sASYNCIFY # Required for emscripten_sleep in the async wgpu adapter and device request

        # -sDISABLE_EXCEPTION_CATCHING=0

        # uncomment to include local files in the build
        # https://emscripten.org/docs/porting/files/packaging_files.html
        # https://emscripten.org/docs/getting_started/Tutorial.html#tutorial-files
        # To include files outside of the current working directory
        # use the `--preload-file srcpath@dstpath` syntax to explicitly 
        # specify the target location
        # --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@./assets

        # Can specify custom html template
        # https://github.com/emscripten-core/emscripten/blob/main/src/shell_minimal.html
        # --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".wasm")

    # emscripten only generates .html properly if target is a MAIN_MODULE
    # set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
endif (EMSCRIPTEN)
