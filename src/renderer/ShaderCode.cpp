#include "ShaderCode.h"

// #define REGISTER_CODE(var) \
//     do { \
//         ShaderCode::s_CodeMap[#var] = var; \
//     } while (false)

// #define REGISTER_CODE(var) ShaderCode::s_CodeMap[#var] = var;

// ====================================================================================================
//  ShaderCode strings
// ====================================================================================================

// https://github.com/stef-levesque/vscode-shader/pull/45
// Highlighting shadercode inside c++ raw strings

const std::string ShaderCode::BASIC_VERT = "GG_BASIC_VERT";
const std::string ShaderCode::NORMAL_FRAG = "GG_NORMAL_FRAG";
const std::string ShaderCode::FLAT_FRAG = "GG_FLAT_FRAG";

const std::string ShaderCode::SKYBOX_VERT_CODE = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 a_Pos;
    
    out vec3 v_TexCoord;

    uniform mat4 u_Projection;
    uniform mat4 u_View;

    void main()
    {
        v_TexCoord = a_Pos;
        vec4 pos = u_Projection * u_View * vec4(a_Pos, 1.0);
        gl_Position = pos.xyww;  // force z to be 1.0 after perspective division
    }  
)glsl";

const std::string ShaderCode::SKYBOX_FRAG_CODE = R"glsl(
    #version 330 core
    out vec4 FragColor;

    in vec3 v_TexCoord;

    uniform samplerCube skybox;

    void main()
    {    
        FragColor = texture(skybox, v_TexCoord);
        // debug UV
        // FragColor = vec4(v_TexCoord, 1.0);
        // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }

)glsl";

ShaderCode::ShaderMap ShaderCode::s_CodeMap = {

{"SHADER_VERSION", "#version 330 core\n"},
{
"FRAG_OUT_COLOR_DEF", 
R"glsl(
out vec4 FragColor;
vec4 result = vec4(0.0);
)glsl"
},
{"FRAG_OUTPUT", "FragColor = result;\n"},
{
"DEFAULT_ATTRIBUTES",
R"glsl(
    // attributes (must match vertex attrib array)
    layout (location = 0) in vec3 a_Pos;
    layout (location = 1) in vec3 a_Normal;
    layout (location = 2) in vec4 a_Color;
    layout (location = 3) in vec2 a_TexCoord;
)glsl"
},
{
"TRANSFORM_UNIFORMS",
R"glsl(
    uniform mat4 u_Model;
    uniform mat4 u_View;
    uniform mat4 u_Projection;

    // normals
    uniform mat4 u_Normal;

    // camera
    uniform vec3 u_ViewPos;

    // color
    uniform vec4 u_Color;
)glsl"
},
{
"LIGHTING_UNIFORMS",
R"glsl( 
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
)glsl"
},
{
"POINT_UNIFORMS",
R"glsl(
    uniform float u_PointSize;
    // whether or not to adjust size based on distance to camera
    uniform bool u_PointSizeAttenuation;
)glsl"
},
{
"TEXTURE_UNIFORMS",
R"glsl(
    // textures
    uniform sampler2D u_DiffuseMap;
    uniform sampler2D u_SpecularMap;
    // TODO add others

    // skybox
    // TODO: maybe this should go in its own section?
    uniform samplerCube u_Skybox;
)glsl"
},
{
"FOG_UNIFORMS",
R"glsl(
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
)glsl"
},
{
"FOG_BODY",
R"glsl(
    if (u_FogParams.enabled) {
        float fogCoordinate = abs(v_EyePos.z / v_EyePos.w);  // distance in eyespace
        // float fogCoordinate = length(v_Pos - u_ViewPos);  // TODO: this is wrong (should be in eye space
        result = mix(result, vec4(u_FogParams.color, 1.0), getFogFactor(u_FogParams, fogCoordinate));
    }
)glsl"},
{
"ENV_MAP_UNIFORMS",
R"glsl(
    const int ENV_MAP_BLEND_MODE_ADDITIVE = 0;
    const int ENV_MAP_BLEND_MODE_MULTIPLICATIVE = 1;
    const int ENV_MAP_BLEND_MODE_MIX = 2;

    const int ENV_MAP_METHOD_REFLECTION = 0;
    const int ENV_MAP_METHOD_REFRACTION = 1;

    struct EnvMapParams {
        // samplerCube envMap;
        // defaulting to skybox for now. 
        // can add support for per-object envmaps later

        bool enabled;  // whether to use envmap at all

        // coefficient to scale envmap contribution
        // also used as mix factor for mix blend mode
        float intensity;  

        int blendMode;  // how to blend envmap with lighting
        int method;  // reflection or refraction
        
        float ratio;  // ratio of refractive indices
    };

    uniform EnvMapParams u_EnvMapParams;        
)glsl"
},
{
"EYE_POS_BODY",
R"glsl(
        v_EyePos = u_View * u_Model * vec4(a_Pos, 1.0);  // eye space position
)glsl"
},
{
"POINTS_BODY",
R"glsl(
        gl_PointSize = mix(u_PointSize, u_PointSize / gl_Position.w, u_PointSizeAttenuation); // scale point size by distance to camera
)glsl"
},
{
"BASIC_VERT_VARYINGS",
    R"glsl(
    // varyings (interpolated and passed to frag shader)
    out vec3 v_Pos;
    out vec4 v_EyePos;
    out vec3 v_Normal;		 // world space normal
    out vec3 v_LocalNormal;  // local space normal
    out vec4 v_Color;
    out vec2 v_TexCoord;
)glsl"
},
{
"BASIC_FRAG_VARYINGS",
R"glsl(
    // varyings (interpolated and passed to frag shader)
    in vec3 v_Pos;
    in vec4 v_EyePos;
    in vec3 v_Normal;		 // world space normal
    in vec3 v_LocalNormal;  // local space normal
    in vec4 v_Color;
    in vec2 v_TexCoord;
)glsl"
},
{
BASIC_VERT,
R"glsl(
    #include TRANSFORM_UNIFORMS
    #include POINT_UNIFORMS
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
        #include POINTS_BODY
    }

)glsl"
},
{
NORMAL_FRAG,
R"glsl(
    #include BASIC_FRAG_VARYINGS

    uniform int u_UseLocalNormal = 0;

    // fragment shader for light sources to always be white
    void main()
    {
        result = (1 - u_UseLocalNormal) * vec4(normalize(v_Normal), 1.0) + 
                    (u_UseLocalNormal) * vec4(normalize(v_LocalNormal), 1.0);
)glsl"
},
{
FLAT_FRAG,
R"glsl(
    #include BASIC_FRAG_VARYINGS
    #include TEXTURE_UNIFORMS

    void main()
    {
        vec4 diffuseTex = texture(u_DiffuseMap, v_TexCoord);
        result = v_Color * diffuseTex;
)glsl"
},
{
"PHONG_FRAG",
R"glsl(
    #include TRANSFORM_UNIFORMS
    #include LIGHTING_UNIFORMS
    #include BASIC_FRAG_VARYINGS
    #include TEXTURE_UNIFORMS
    #include ENV_MAP_UNIFORMS

    struct Material {
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

    // calculate envmap contribution
    vec3 CalcEnvMapContribution(vec3 viewDir, vec3 norm)
    {
        // TODO: envmap can be optimized by moving some calculations into vertex shader
        vec3 envMapSampleDir = vec3(0.0);
        if (u_EnvMapParams.method == ENV_MAP_METHOD_REFLECTION) {
            envMapSampleDir = reflect(-viewDir, norm);
        }
        else if (u_EnvMapParams.method == ENV_MAP_METHOD_REFRACTION) {
            envMapSampleDir = refract(-viewDir, norm, u_EnvMapParams.ratio);
        }
        return texture(u_Skybox, envMapSampleDir).rgb;
    }

    // main =====================================================================================
    void main()
    {
        vec3 norm = normalize(v_Normal);  // fragment normal direction
        // TODO: flip this?
        vec3 viewDir = normalize(u_ViewPos - v_Pos);  // direction from camera to this frag

        // material color properties (ignore alpha channel for now)
        vec4 diffuseTex = texture(u_DiffuseMap, v_TexCoord);
        vec4 specularTex = texture(u_SpecularMap, v_TexCoord);
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

        // calculate envmap contribution
        if (u_EnvMapParams.enabled) {
            vec3 envMapContrib = CalcEnvMapContribution(viewDir, norm);

            // blending
            if (u_EnvMapParams.blendMode == ENV_MAP_BLEND_MODE_ADDITIVE) {
                lighting += (u_EnvMapParams.intensity * envMapContrib);
            }
            else if (u_EnvMapParams.blendMode == ENV_MAP_BLEND_MODE_MULTIPLICATIVE) {
                lighting *= (u_EnvMapParams.intensity * envMapContrib);
            }
            else if (u_EnvMapParams.blendMode == ENV_MAP_BLEND_MODE_MIX) {
                lighting = mix(lighting, envMapContrib, u_EnvMapParams.intensity);
            }
        }

        result = vec4(
            lighting, 
            diffuse.a * specular.a  // TODO this is not correct way to composite alpha
        );
)glsl"
},
{
"POINTS_VERT",
R"glsl(
    #include TRANSFORM_UNIFORMS
    #include POINT_UNIFORMS
    #include DEFAULT_ATTRIBUTES


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
        #include POINTS_BODY
    }
)glsl"
},
{
"POINTS_FRAG",
R"glsl(
    // in vec3 v_Pos;
    // in vec4 v_EyePos;
    // in vec4 v_Color;
    // in vec2 v_TexCoord;
    #include BASIC_FRAG_VARYINGS

    // output

    // uniforms
    uniform sampler2D u_PointTexture;

    void main()
    {
        result = v_Color * texture(u_PointTexture, gl_PointCoord);
)glsl"
},
{
"LINES_VERT",
R"glsl(
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
)glsl"
},
{ 
"LINES_FRAG",
R"glsl(
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
)glsl"
},
{
"MANGO_FRAG",
R"glsl(
    #include BASIC_FRAG_VARYINGS

    // output

    // uniforms
    uniform float u_Time;

    void main()
    {
        result = vec4(
            v_TexCoord, 0.0, 1.0
        );
)glsl"
}
};  // end ShaderCode::s_CodeMap

// ====================================================================================================
// Post Process Shaders
// ====================================================================================================

const std::string ShaderCode::PP_VERT = R"glsl(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoords;

    out vec2 TexCoords;

    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
        TexCoords = aTexCoords;
    }  
)glsl";

const std::string ShaderCode::PP_PASS_THROUGH = R"glsl(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoords;

    uniform sampler2D screenTexture;

    void main()
    { 
        // normal render
        FragColor = texture(screenTexture, TexCoords);
    }
)glsl";

const std::string ShaderCode::PP_OUTPUT = R"glsl(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoords;

    // Uniforms ==================================================================
    uniform sampler2D screenTexture;
    uniform float u_Gamma = 2.2;
    

    // Main ======================================================================
    void main()
    { 
        // normal render
        vec4 color = texture(screenTexture, TexCoords);

        // apply gamma correction
        color.rgb = pow(color.rgb, vec3(1.0/u_Gamma));

        FragColor = color;
    }
)glsl";

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