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

# chugl library
add_library(${PROJECT_NAME} MODULE ChuGL.cpp ulib_cgl.cpp window.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES 
    PREFIX ""
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX ".chug"
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
# target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# library dependencies
add_subdirectory(./vendor/glfw)  # TODO prob create a top-level CMakeLists.txt too manage all vendor deps
add_subdirectory(./vendor/glad)  # TODO prob create a top-level CMakeLists.txt too manage all vendor deps
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

install(TARGETS ${PROJECT_NAME} DESTINATION /usr/local/lib/chuck)


# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL")
endif()