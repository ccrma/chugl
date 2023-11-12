cmake_minimum_required(VERSION 3.16)

set(CMAKE_VERBOSE_MAKEFILE on)

# force universal binary on osx (must go BEFORE project call)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
# specify minimum support macOS version
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    # If no deployment target has been set default to the minimum supported
    # OS version (this has to be set before the first project() call)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "iOS Deployment Target")
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10 CACHE STRING "macOS Deployment Target")
    endif()
endif()

project(
    ChuGL
    VERSION 0.1.0
    LANGUAGES CXX C
)

if(LINUX)
    add_compile_options(-fPIC)
    #set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
endif()

if(APPLE)
    add_link_options(-undefined error)
    #set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error" )
endif()


# all ulib_*.cpp files
file(GLOB ULIB_SOURCES "ulib_*.cpp")

# chugl library
add_library(
    ${PROJECT_NAME} SHARED 
    ChuGL.cpp 
    ulib_cgl.cpp 
    window.cpp
    ${ULIB_SOURCES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    PREFIX ""
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX ".chug"
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)


# library dependencies

# glfw
# -- exclude glfw docs, tests, and examples
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(./vendor/glfw)  # TODO prob create a top-level CMakeLists.txt too manage all vendor deps

add_subdirectory(./vendor/glad)
add_subdirectory(./vendor/glm)
add_subdirectory(./renderer)
add_subdirectory(./vendor/imgui)


# Tracy profiler
include(FetchContent)
# set default tracy options
set(TRACY_ENABLE OFF CACHE BOOL "")
set(TRACY_DELAYED_INIT ON CACHE BOOL "" FORCE)
set(TRACY_MANUAL_LIFETIME ON CACHE BOOL "" FORCE)
# fetch tracy
FetchContent_Declare (
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.10  # must match the profiler server version
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable (tracy)

# assimp Open Asset Import Library
# set default assimp options
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "")
set(ASSIMP_INSTALL OFF CACHE BOOL "")
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "")
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "")
set(ASSIMP_NO_EXPORT ON CACHE BOOL "")  # disable export functionality for now until we add it to the DL API
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")  # disable all importers by default
# which importers to build
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "")       # Autodesk
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")      # .gltf, .glb
set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "")   # digital asset exchange (.dae)
set(ASSIMP_BUILD_BLEND_IMPORTER ON CACHE BOOL "")     # blender
set(ASSIMP_BUILD_PLY_IMPORTER ON CACHE BOOL "")       # standard polygon library
set(ASSIMP_BUILD_STL_IMPORTER ON CACHE BOOL "")       # stereolithography
set(ASSIMP_BUILD_3DS_IMPORTER ON CACHE BOOL "")       # 3ds max
set(ASSIMP_BUILD_BVH_IMPORTER ON CACHE BOOL "")       # motion capture
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")       # wavefront obj
# fetch assimp
FetchContent_Declare (
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.3.1
)
# make available
FetchContent_MakeAvailable (assimp)

# =================================================================================
# interface library for compiler flags
# =================================================================================
add_library(chugl_compiler_flags INTERFACE)

# specify the C++ standard
target_compile_features(chugl_compiler_flags INTERFACE cxx_std_11)

# add compiler warning flags just when building this project via
# the BUILD_INTERFACE genex
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(chugl_compiler_flags INTERFACE
  "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Werror;>>"
  "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
)
# =================================================================================

# =================================================================================
# interface library for precompiled headers
# =================================================================================
add_library(chugl_pch INTERFACE)

# add precompiled headers to interface
target_precompile_headers(chugl_pch INTERFACE chugl_pch.h)

# add libraries to interface
target_link_libraries(chugl_pch INTERFACE
    TracyClient glm assimp
)

# set interface include directories
target_include_directories(chugl_pch INTERFACE
    .
    ./chuck/include
    ./vendor
)
# =================================================================================


# link libraries to chugl.chug
target_link_libraries(${PROJECT_NAME} 
    PRIVATE glfw glad imgui
    PRIVATE chugl_compiler_flags chugl_pch
    PRIVATE renderer
)

# header includes
target_include_directories(${PROJECT_NAME} 
    PRIVATE ./chuck/include
    PRIVATE ./vendor
    PRIVATE ./vendor/glad/include
    PRIVATE ./vendor/glfw/include
    PRIVATE ./vendor/imgui
)

# set install destination by platform
if (APPLE)
    set(INSTALL_DESTINATION "~/.chuck/lib")
    message(STATUS "chugl.chug Install destination: ${INSTALL_DESTINATION}")
    install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_DESTINATION})
endif()


# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL")
endif()

# windows debugging setup
if ( MSVC )
    set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME} )
    set(CHUCK_EXECUTABLE_DEBUG "${CMAKE_SOURCE_DIR}/chuck-debug.exe")
    set(CHUCK_EXECUTABLE_RELEASE "${CMAKE_SOURCE_DIR}/chuck-release.exe")
    set(CHUGIN_PATH_DEBUG "--chugin-path:${CMAKE_TARGET_DIR}./build/Debug")
    set(CHUGIN_PATH_RELEASE "--chugin-path:${CMAKE_TARGET_DIR}./build/Release")
    message(STATUS "Setting chuck debug executable to ${CHUCK_EXECUTABLE_DEBUG}")
    message(STATUS "Setting chuck release executable to ${CHUCK_EXECUTABLE_RELEASE}")
    message(STATUS "Setting chuck debug chugin path to ${CHUGIN_PATH_DEBUG}")
    message(STATUS "Setting chuck release chugin path to ${CHUGIN_PATH_RELEASE}")
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        # VS_DEBUGGER_COMMAND "${CHUCK_EXECUTABLE_DEBUG}"
        VS_DEBUGGER_COMMAND $<$<CONFIG:Debug>:${CHUCK_EXECUTABLE_DEBUG}>$<$<CONFIG:Release>:${CHUCK_EXECUTABLE_RELEASE}>
        VS_DEBUGGER_COMMAND_ARGUMENTS $<$<CONFIG:Debug>:${CHUGIN_PATH_DEBUG}>$<$<CONFIG:Release>:${CHUGIN_PATH_RELEASE}>
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()
