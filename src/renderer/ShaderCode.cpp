#include "ShaderCode.h"

// #define REGISTER_CODE(var) \
//     do { \
//         ShaderCode::s_CodeMap[#var] = var; \
//     } while (false)

// #define REGISTER_CODE(var) ShaderCode::s_CodeMap[#var] = var;

// ====================================================================================================
//  ShaderCode strings
// ====================================================================================================

ShaderCode::ShaderMap ShaderCode::s_CodeMap = {
    {"SHADER_VERSION", "#version 330 core\n"},
    {"TRANSFORM_UNIFORMS",
     R"(
        uniform mat4 u_Model;
        uniform mat4 u_View;
        uniform mat4 u_Projection;

        // normals
        uniform mat4 u_Normal;

        // camera
        uniform vec3 u_ViewPos;

    )"},
    {"LIGHTING_UNIFORMS",
     R"( 
        // Include this to access all the scene lighting info
        struct PointLight {
            // color info
            vec3 ambient;
            vec3 diffuse;
            vec3 specular;
            
            // range and dropoffs
            float intensity;
            float constant;
            float linear;
            float quadratic;  

            // transform
            vec3 position;
        };

        struct DirLight {
            vec3 direction;

            float intensity;

            // color info
            vec3 ambient;
            vec3 diffuse;
            vec3 specular;
        };

        const int MAX_LIGHTS = 12;
        uniform int u_NumPointLights = 0;
        uniform int u_NumDirLights = 0;
        uniform PointLight u_PointLights[MAX_LIGHTS];
        uniform DirLight u_DirLights[MAX_LIGHTS];
    )"},
    {"BASIC_VERTEX_SHADER",
     R"(
        #include TRANSFORM_UNIFORMS

        layout (location = 0) in vec3 a_Pos;
        layout (location = 1) in vec3 a_Normal;
        layout (location = 2) in vec3 a_Color;
        layout (location = 3) in vec2 a_TexCoord;

        // varyings (interpolated and passed to frag shader)
        out vec3 v_Pos;
        out vec3 v_Normal;		 // world space normal
        out vec3 v_LocalNormal;  // local space normal
        out vec3 v_Color;
        out vec2 v_TexCoord;

        void main()
        {
            v_Pos = vec3(u_Model * vec4(a_Pos, 1.0));  // world space position
            v_TexCoord = a_TexCoord;

            v_LocalNormal = a_Normal;  
            v_Normal = vec3(u_Normal * vec4(a_Normal, 0.0));

            v_Color = a_Color;

            gl_Position = u_Projection * u_View * vec4(v_Pos, 1.0);
        }
    )"}

};



// ====================================================================================================
//  ShaderCode impl
// ====================================================================================================

// populate the map with raw code strings
// ShaderCode::ShaderCode()
// {
//     REGISTER_CODE(SHADER_VERSION);
// }

std::string ShaderCode::GenShaderSource(const std::string& name) {
    std::string source = s_CodeMap[name];
    size_t pos = source.find("#include");
    while (pos != std::string::npos) {
        size_t start = source.find_first_of(' ', pos);
        size_t end = source.find_first_of('\n', start + 1);
        std::string includeName = source.substr(start + 1, end - start - 1);
        std::string includeSource = s_CodeMap[includeName];
        source.replace(pos, end - pos + 1, includeSource);
        pos = source.find("#include");
    }
    return source;
}