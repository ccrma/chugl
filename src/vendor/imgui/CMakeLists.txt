file(GLOB imgui_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_library(
    imgui
    ${imgui_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
)

target_link_libraries(imgui
    PUBLIC glfw glad 
)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_IMPL_OPENGL_LOADER_GLAD2")

target_include_directories(
    imgui 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)