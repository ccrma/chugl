#include "ShaderCode.h"
#include <iostream>

// #define REGISTER_CODE(var) \
//     do { \
//         ShaderCode::s_CodeMap[#var] = var; \
//     } while (false)

// #define REGISTER_CODE(var) ShaderCode::s_CodeMap[#var] = var;

// ====================================================================================================
//  ShaderCode strings
// ====================================================================================================

const std::string ShaderCode::BASIC_VERT = "GG_BASIC_VERT";
const std::string ShaderCode::NORMAL_FRAG = "GG_NORMAL_FRAG";

ShaderCode::ShaderMap ShaderCode::s_CodeMap = {
    {"SHADER_VERSION", "#version 330 core\n"},
    {"FRAG_OUT_COLOR_DEF", 
    R"(
    out vec4 FragColor;
    vec4 result = vec4(0.0);
    )"},
    {"FRAG_OUTPUT", "FragColor = result;\n"},
    {"DEFAULT_ATTRIBUTES",
    R"(
        // attributes (must match vertex attrib array)
        layout (location = 0) in vec3 a_Pos;
        layout (location = 1) in vec3 a_Normal;
        layout (location = 2) in vec4 a_Color;
        layout (location = 3) in vec2 a_TexCoord;
    )"},
    {"TRANSFORM_UNIFORMS",
     R"(
        uniform mat4 u_Model;
        uniform mat4 u_View;
        uniform mat4 u_Projection;

        // normals
        uniform mat4 u_Normal;

        // camera
        uniform vec3 u_ViewPos;

        // color
        uniform vec4 u_Color;
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
    {
    "FOG_UNIFORMS",
    R"(
        struct FogParameters
        {
            vec3 color;
            float density;
            
            int type;  // exp fog or exp^2
            bool enabled;
        };

        float getFogFactor(FogParameters params, float fogCoordinate)
        {
            float fogFactor = 0.0;

            if(params.type == 0) {
                fogFactor = exp(-params.density * fogCoordinate);
            }
            else if(params.type == 1) {
                fogFactor = exp(-pow(params.density * fogCoordinate, 2.0));
            }
            
            return 1.0 - clamp(fogFactor, 0.0, 1.0);
        }

        uniform FogParameters u_FogParams;
    )"},
    {
    "FOG_BODY",
    R"(
        if (u_FogParams.enabled) {
            float fogCoordinate = abs(v_EyePos.z / v_EyePos.w);  // distance in eyespace
            // float fogCoordinate = length(v_Pos - u_ViewPos);  // TODO: this is wrong (should be in eye space
            result = mix(result, vec4(u_FogParams.color, 1.0), getFogFactor(u_FogParams, fogCoordinate));
        }
    )"},
    {
    "EYE_POS_BODY",
    R"(
            v_EyePos = u_View * u_Model * vec4(a_Pos, 1.0);  // eye space position
    )"},
    {"BASIC_VERT_VARYINGS",
     R"(
        // varyings (interpolated and passed to frag shader)
        out vec3 v_Pos;
        out vec4 v_EyePos;
        out vec3 v_Normal;		 // world space normal
        out vec3 v_LocalNormal;  // local space normal
        out vec4 v_Color;
        out vec2 v_TexCoord;
    )"},
    {"BASIC_FRAG_VARYINGS",
     R"(
        // varyings (interpolated and passed to frag shader)
        in vec3 v_Pos;
        in vec4 v_EyePos;
        in vec3 v_Normal;		 // world space normal
        in vec3 v_LocalNormal;  // local space normal
        in vec4 v_Color;
        in vec2 v_TexCoord;
    )"},
    {BASIC_VERT,
     R"(
        #include TRANSFORM_UNIFORMS
        #include DEFAULT_ATTRIBUTES
        #include BASIC_VERT_VARYINGS

        void main()
        {
            v_Pos = vec3(u_Model * vec4(a_Pos, 1.0));  // world space position
            #include EYE_POS_BODY
            gl_Position = u_Projection * v_EyePos;  // clip space position

            v_TexCoord = a_TexCoord;

            v_LocalNormal = a_Normal;  
            v_Normal = vec3(u_Normal * vec4(a_Normal, 0.0));

            v_Color = a_Color * u_Color;
        }

    )"},
    {NORMAL_FRAG,
     R"(
        #include BASIC_FRAG_VARYINGS

        uniform int u_UseLocalNormal = 0;

        // fragment shader for light sources to always be white
        void main()
        {
            result = (1 - u_UseLocalNormal) * vec4(normalize(v_Normal), 1.0) + 
                        (u_UseLocalNormal) * vec4(normalize(v_LocalNormal), 1.0);
    )"},
    {"PHONG_FRAG",
     R"(
        #include TRANSFORM_UNIFORMS
        #include LIGHTING_UNIFORMS
        #include BASIC_FRAG_VARYINGS

        struct Material {
            // textures
            sampler2D diffuseMap;
            sampler2D specularMap;

            // colors
            vec4 specularColor;

            // specular highlights
            float shininess;  // range from (0, 2^n). must be > 0. logarithmic scale.
        };
        uniform Material u_Material;

        // output ==========================================================================

        // functions to calculate light contribution ======================================
        vec3 CalcDirLight(  // contribution of directional light
            DirLight light, vec3 normal, vec3 viewDir,
            float shininess, vec3 diffuseCol, vec3 specularCol
        ) {
            vec3 lightDir = normalize(-light.direction);
            // diffuse shading
            float diff = max(dot(normal, lightDir), 0.0);
            // specular shading
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            // combine results
            vec3 ambient  = light.ambient  * diffuseCol;
            vec3 diffuse  = light.diffuse  * diff * diffuseCol;
            vec3 specular = light.specular * spec * specularCol;
            return (ambient + diffuse + specular) * light.intensity;
        }

        // contribution from point lights
        vec3 CalcPointLight(
            PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,
            float shininess, vec3 diffuseCol, vec3 specularCol
        ) {
            vec3 lightDir = normalize(light.position - fragPos);
            // diffuse shading
            float diff = max(dot(normal, lightDir), 0.0);
            // specular shading
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = max(pow(max(dot(viewDir, reflectDir), 0.0), shininess), 0.0);
            // attenuation
            float distance    = length(light.position - fragPos);
            float attenuation = 1.0 / (1.0 + light.linear * distance + 
                        light.quadratic * (distance * distance));    
            // combine results
            vec3 ambient  = light.ambient  * diffuseCol;
            vec3 diffuse  = light.diffuse  * diff * diffuseCol;
            vec3 specular = light.specular * spec * specularCol;

            // attenuate
            return (ambient + diffuse + specular) * attenuation * light.intensity;
        } 

        // TODO
        // vec3 CalcSpotLight() {}

        // main =====================================================================================
        void main()
        {
            vec3 norm = normalize(v_Normal);  // fragment normal direction
            vec3 viewDir = normalize(u_ViewPos - v_Pos);  // direction from camera to this frag

            // material color properties (ignore alpha channel for now)
            vec4 diffuseTex = texture(u_Material.diffuseMap, v_TexCoord);
            vec4 specularTex = texture(u_Material.specularMap, v_TexCoord);
            vec4 diffuse = diffuseTex * v_Color;
            vec4 specular = specularTex * u_Material.specularColor;

            vec3 lighting = vec3(0.0);

            // loop through point lights
            for (int i = 0; i < u_NumPointLights; i++) {
                PointLight light = u_PointLights[i];
                lighting += CalcPointLight(
                    light, norm, v_Pos, viewDir, u_Material.shininess, diffuse.xyz, specular.xyz
                );
            }

            // loop through directional lights
            for (int i = 0; i < u_NumDirLights; i++) {
                DirLight light = u_DirLights[i];
                lighting += CalcDirLight(
                    light, norm, viewDir, u_Material.shininess, diffuse.xyz, specular.xyz
                );
            }

            result = vec4(
                lighting, 
                diffuse.a * specular.a
            );
    )"},
    {"POINTS_VERT",
     R"(
        #include TRANSFORM_UNIFORMS
        #include DEFAULT_ATTRIBUTES

        // uniforms (passed in from program)
        uniform float u_PointSize;

        // whether or not to adjust size based on distance to camera
        uniform bool u_PointSizeAttenuation;

        // varyings (interpolated and passed to frag shader)
        #include BASIC_VERT_VARYINGS

        // out vec3 v_Pos;
        // out vec4 v_EyePos;
        // out vec4 v_Color;
        // out vec2 v_TexCoord;

        void main()
        {
            v_Pos = vec3(u_Model * vec4(a_Pos, 1.0));  // world space position
            #include EYE_POS_BODY
            v_Color = a_Color * u_Color;
            v_TexCoord = a_TexCoord;

            gl_Position = u_Projection * u_View * vec4(v_Pos, 1.0);
            gl_PointSize = mix(u_PointSize, u_PointSize / gl_Position.w, u_PointSizeAttenuation); // scale point size by distance to camera
        }
    )"},
    {"POINTS_FRAG",
     R"(
        // in vec3 v_Pos;
        // in vec4 v_EyePos;
        // in vec4 v_Color;
        // in vec2 v_TexCoord;
        #include BASIC_FRAG_VARYINGS

        // output

        // uniforms
        uniform float u_PointSize;
        uniform sampler2D u_PointTexture;

        void main()
        {
            result = v_Color * texture(u_PointTexture, gl_PointCoord);
    )"},
    {"LINES_VERT",
     R"(
        #include TRANSFORM_UNIFORMS
        #include DEFAULT_ATTRIBUTES

        uniform float u_LineWidth;

        // varyings (interpolated and passed to frag shader)
        #include BASIC_VERT_VARYINGS
        // out vec3 v_Pos;
        // out vec4 v_EyePos;
        // out vec3 v_Color;

        void main()
        {
            v_Pos = vec3(u_Model * vec4(a_Pos, 1.0));  // world space position
            #include EYE_POS_BODY
            v_Color = a_Color * u_Color;

            gl_Position = u_Projection * u_View * vec4(v_Pos, 1.0);
        }
    )"},
    {"LINES_FRAG",
     R"(
        // in vec3 v_Pos;
        // in vec4 v_EyePos;
        // in vec3 v_Color;
        #include BASIC_FRAG_VARYINGS
        // output

        // uniforms
        uniform float u_LineWidth;

        void main()
        {
            result = v_Color;
    )"},
    {
    "MANGO_FRAG",
    R"(
        #include BASIC_FRAG_VARYINGS

        // output

        // uniforms
        uniform float u_Time;

        void main()
        {
            result = vec4(
                v_TexCoord, .5 * sin(u_Time) + .5, 1.0
            );
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

std::string ShaderCode::GenShaderSource(
    const std::string& name,
    ShaderType type,
    bool fog
) {
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

    if (type == ShaderType::Vertex) {
        return s_CodeMap["SHADER_VERSION"] + source;
    } else if (type == ShaderType::Fragment) {
        return (
            s_CodeMap["SHADER_VERSION"] + 
            s_CodeMap["FRAG_OUT_COLOR_DEF"] + 
            (fog ? s_CodeMap["FOG_UNIFORMS"] : "") +
            source +
            (fog ? s_CodeMap["FOG_BODY"] : "") +
            s_CodeMap["FRAG_OUTPUT"] +
            "\n}"
        );
    } else {
        throw std::runtime_error("ShaderCode::GenShaderSource: invalid shader type");
    }
}