add_library(renderer # TODO this whole structure is a mess I realize....can build it better to be a library (with public includes/ dir)
    IndexBuffer.cpp
    Renderer.cpp
    RendererState.cpp
    Shader.cpp
    ShaderCode.cpp
    Texture.cpp
    VertexArray.cpp
    VertexBuffer.cpp

    # scenegraph -------
    scenegraph/Camera.cpp
    scenegraph/Command.cpp
    scenegraph/Geometry.cpp
    scenegraph/Light.cpp
    scenegraph/Material.cpp
    scenegraph/Mesh.cpp
    scenegraph/Scene.cpp
    scenegraph/SceneGraphNode.cpp
    scenegraph/SceneGraphObject.cpp
    scenegraph/CGL_Texture.cpp
)

target_link_libraries(renderer
    PUBLIC glfw glad glm chugl_compiler_flags
    PUBLIC TracyClient
)

target_include_directories(renderer 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/chuck/include/
)

message(cmake_current_source_dir=${CMAKE_CURRENT_SOURCE_DIR})

# TODO: add install interface? https://stackoverflow.com/a/74313654
