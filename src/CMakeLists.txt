cmake_minimum_required(VERSION 3.15)

# force universal binary on osx (must go BEFORE project call)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
project(
    chugl
    VERSION 0.0.1
    LANGUAGES CXX C
)

# global compiler flags interface
# specify the C++ standard
add_library(chugl_compiler_flags INTERFACE)
target_compile_features(chugl_compiler_flags INTERFACE cxx_std_11)

# add compiler warning flags just when building this project via
# the BUILD_INTERFACE genex
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(chugl_compiler_flags INTERFACE
  "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Werror;>>"
  "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
)

# find chuck program
find_program(CHUCK_EXECUTABLE NAMES chuck)
if (NOT CHUCK_EXECUTABLE)
    message(FATAL_ERROR "Could not find chuck executable")
endif()

# chugl library
add_library(${PROJECT_NAME} SHARED ChuGL.cpp ulib_cgl.cpp window.cpp)
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
target_link_libraries(${PROJECT_NAME} 
    PRIVATE glfw glad glm
    PRIVATE chugl_compiler_flags
    PRIVATE renderer
)

# header includes
target_include_directories(${PROJECT_NAME} 
    PRIVATE ./chuck/include
    PRIVATE ./vendor
    PRIVATE ./vendor/glad/include
    PRIVATE ./vendor/glfw/include
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
    set(CHUCK_EXECUTABLE "${CMAKE_SOURCE_DIR}/chuck.exe")
    message(STATUS "Setting chuck executable to ${CHUCK_EXECUTABLE}")
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_COMMAND "${CHUCK_EXECUTABLE}"
        VS_DEBUGGER_COMMAND_ARGUMENTS "--chugin-path:${CMAKE_TARGET_DIR}./build/Debug"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()