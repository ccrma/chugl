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
    // skybox
    // TODO: maybe this should go in its own section?
    uniform samplerCube u_Skybox;

    // textures
    uniform sampler2D u_DiffuseMap;
    uniform sampler2D u_SpecularMap;
    // TODO add others

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

const std::string ShaderCode::PP_MONOCHROME = R"glsl(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoords;

    uniform sampler2D screenTexture;
    uniform float u_Mix = 1.0;
    uniform vec3 u_MonoColor = vec3(1.0);

    float grayscale(vec3 rgb) {
        return dot(rgb, vec3(0.2126, 0.7152, 0.0722));
    }

    void main()
    { 
        vec4 srcColor = texture(screenTexture, TexCoords);

        float gray = grayscale(srcColor.rgb);
        vec3 monoColor = u_MonoColor * gray;

        FragColor = vec4(mix(srcColor.rgb, monoColor, u_Mix), srcColor.a);
    }
)glsl";

const std::string ShaderCode::PP_INVERT = R"glsl(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoords;

    uniform sampler2D screenTexture;
    uniform float u_Mix = 1.0;

    void main()
    { 
        vec4 srcColor = texture(screenTexture, TexCoords);
        vec3 inverseColor = vec3(1.0) - srcColor.rgb;

        FragColor = vec4(mix(srcColor.rgb, inverseColor, u_Mix), srcColor.a);
    }
)glsl";

const std::string ShaderCode::PP_OUTPUT = R"glsl(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoords;

    // Constants ==================================================================
    const int TONEMAP_NONE = 0;
    const int TONEMAP_LINEAR = 1;
    const int TONEMAP_REINHARD = 2;
    const int TONEMAP_CINEON = 3;
    const int TONEMAP_ACES = 4;
    const int TONEMAP_UNCHARTED = 5;

    // Uniforms ==================================================================
    uniform sampler2D screenTexture;
    uniform float u_Gamma = 2.2;
    uniform float u_Exposure = 1.0;
    uniform int u_Tonemap = TONEMAP_REINHARD;
    
    // Helper ====================================================================
    vec3 Uncharted2Tonemap(vec3 x)
    {
        float A = 0.15;
        float B = 0.50;
        float C = 0.10;
        float D = 0.20;
        float E = 0.02;
        float F = 0.30;
        return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
    }

    // source: https://github.com/selfshadow/ltc_code/blob/master/webgl/shaders/ltc/ltc_blit.fs
    vec3 rrt_odt_fit(vec3 v)
    {
        vec3 a = v*(         v + 0.0245786) - 0.000090537;
        vec3 b = v*(0.983729*v + 0.4329510) + 0.238081;
        return a/b;
    }

    mat3 mat3_from_rows(vec3 c0, vec3 c1, vec3 c2)
    {
        mat3 m = mat3(c0, c1, c2);
        m = transpose(m);

        return m;
    }


    // Main ======================================================================
    void main()
    { 
        // normal render
        vec4 hdrColor = texture(screenTexture, TexCoords);
        vec3 color = hdrColor.rgb;

        // apply exposure 
        if (u_Tonemap != TONEMAP_NONE) {
            color *= u_Exposure;
        }

        // apply tonemapping
        // source: http://filmicworlds.com/blog/filmic-tonemapping-operators/
        switch (u_Tonemap)
        {
        case TONEMAP_LINEAR:
            color = clamp(color, 0.0, 1.0);
            break;
        case TONEMAP_REINHARD:
            color = hdrColor.rgb / (hdrColor.rgb + vec3(1.0));
            break;
        case TONEMAP_CINEON:
            vec3 x = max(vec3(0), color-0.004);
            color = (x*(6.2*x+.5))/(x*(6.2*x+1.7)+0.06);
            // cancel out gamma!!
            color = pow(color, vec3(u_Gamma));
            break;
        case TONEMAP_ACES:
            // source: https://github.com/selfshadow/ltc_code/blob/master/webgl/shaders/ltc/ltc_blit.fs
            // source: https://github.com/selfshadow/ltc_code/blob/master/webgl/shaders/ltc/ltc_blit.fs

            // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
            mat3 ACES_INPUT_MAT = mat3_from_rows(
                vec3( 0.59719, 0.35458, 0.04823),
                vec3( 0.07600, 0.90834, 0.01566),
                vec3( 0.02840, 0.13383, 0.83777)
            );

            // ODT_SAT => XYZ => D60_2_D65 => sRGB
            mat3 ACES_OUTPUT_MAT = mat3_from_rows(
                vec3( 1.60475,-0.53108,-0.07367),
                vec3(-0.10208, 1.10813,-0.00605),
                vec3(-0.00327,-0.07276, 1.07602)
            );

            // scale color for exposure
            color = color / 0.6;

            color = ACES_INPUT_MAT * color;

            // Apply RRT and ODT
            color = rrt_odt_fit(color);

            color = ACES_OUTPUT_MAT * color;

            // clamp
            color = clamp(color, 0.0, 1.0);

            break;
        case TONEMAP_UNCHARTED:
            float ExposureBias = 2.0f;
            vec3 curr = Uncharted2Tonemap(ExposureBias*color);

            // apply white balance
            float W = 11.2;
            vec3 whiteScale = vec3(1.0/Uncharted2Tonemap(vec3(W)));
            color = curr*whiteScale;
            break;
        default:
            break;
        }

        // apply gamma correction
        color = pow(color, vec3(1.0/u_Gamma));

        FragColor = vec4(color, 1.0);
    }
)glsl";

// Karis average: https://github.com/github-linguist/linguist/blob/master/samples/HLSL/bloom.cginc
const std::string ShaderCode::PP_BLOOM_DOWNSAMPLE = R"glsl(
    #version 330 core

    // This shader performs downsampling on a texture,
    // as taken from Call Of Duty method, presented at ACM Siggraph 2014.
    // This particular method was customly designed to eliminate
    // "pulsating artifacts and temporal stability issues".

    // Remember to add bilinear minification filter for this texture!
    // Remember to use a floating-point texture format (for HDR)!
    // Remember to use edge clamping for this texture!

    // output ====================================================================
    layout (location = 0) out vec4 outputColor;

    // input varyings ============================================================
    in vec2 TexCoords;

    // uniforms ==================================================================
    uniform sampler2D srcTexture;
    uniform vec2 u_SrcResolution;
    uniform int u_MipLevel;  // current level of the downsample chain
    uniform float u_Threshold;      // range: [0, inf]
    uniform float u_ThresholdKnee;  // range: [0, 1]

    uniform bool u_KarisEnabled = false;

    // helpers ===================================================================

    float max3 (vec3 v) {
        return max (max (v.x, v.y), v.z);
    }
    vec3 PowVec3(vec3 v, float p)
    {
        return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
    }

    const float invGamma = 1.0 / 2.2;
    vec3 ToSRGB(vec3 v) { return PowVec3(v, invGamma); }

    float RGBToLuminance(vec3 col)
    {
        return dot(col, vec3(0.2126f, 0.7152f, 0.0722f));
    }

    float KarisAverageLuma(vec3 col)
    {
        // Formula is 1 / (1 + luma)
        float luma = RGBToLuminance(ToSRGB(col));
        return 1.0f / (1.0f + luma);
    }

    float KarisAverageBrightness(vec3 col)
    {
        // Formula is 1 / (1 + brightness)
        float brightness = max3(col);
        return 1.0f / (1.0f + brightness);
    }


    void main()
    {
        vec2 srcTexelSize = 1.0 / u_SrcResolution;
        float x = srcTexelSize.x;
        float y = srcTexelSize.y;
        float alpha = texture(srcTexture, TexCoords).a;

        // Take 13 samples around current texel:
        // a - b - c
        // - j - k -
        // d - e - f
        // - l - m -
        // g - h - i
        // === ('e' is the current texel) ===
        vec3 a = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y + 2*y)).rgb;
        vec3 b = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y + 2*y)).rgb;
        vec3 c = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y + 2*y)).rgb;

        vec3 d = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y)).rgb;
        vec3 e = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y)).rgb;
        vec3 f = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y)).rgb;

        vec3 g = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y - 2*y)).rgb;
        vec3 h = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y - 2*y)).rgb;
        vec3 i = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y - 2*y)).rgb;

        vec3 j = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
        vec3 k = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;
        vec3 l = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
        vec3 m = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;

        // Apply weighted distribution:
        // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
        // a,b,d,e * 0.125
        // b,c,e,f * 0.125
        // d,e,g,h * 0.125
        // e,f,h,i * 0.125
        // j,k,l,m * 0.5
        // This shows 5 square areas that are being sampled. But some of them overlap,
        // so to have an energy preserving downsample we need to make some adjustments.
        // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
        // contribute 0.5 to the final color output. The code below is written
        // to effectively yield this sum. We get:
        // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
        vec3 downsample = vec3(0.0);

        // apply thresholding
        vec3 groups[5];
        switch (u_MipLevel) {
        case 0:
            // karis average
            if (u_KarisEnabled) {
                groups[0] = (a+b+d+e) * 0.25f;
                groups[1] = (b+c+e+f) * 0.25f;
                groups[2] = (d+e+g+h) * 0.25f;
                groups[3] = (e+f+h+i) * 0.25f;
                groups[4] = (j+k+l+m) * 0.25f;

                float kw0 = KarisAverageLuma(groups[0]);
                float kw1 = KarisAverageLuma(groups[1]);
                float kw2 = KarisAverageLuma(groups[2]);
                float kw3 = KarisAverageLuma(groups[3]);
                float kw4 = KarisAverageLuma(groups[4]);
                downsample = (kw0 * groups[0] + kw1* groups[1] + kw2 * groups[2] + kw3* groups[3] + kw4 * groups[4]) / (kw0 + kw1 + kw2 + kw3 + kw4);
            }
            else {
                downsample = e*0.125;
                downsample += (a+c+g+i)*0.03125;
                downsample += (b+d+f+h)*0.0625;
                downsample += (j+k+l+m)*0.125;
            }
            // thresholding
            float brightness = max3(e);
            float contribution = max(0, brightness - u_Threshold) / max (brightness, 0.00001);
            downsample *= contribution;
            break;
        default:
            downsample = e*0.125;
            downsample += (a+c+g+i)*0.03125;
            downsample += (b+d+f+h)*0.0625;
            downsample += (j+k+l+m)*0.125;
            break;
        }

        outputColor = vec4(downsample, alpha);
    }

)glsl";

const std::string ShaderCode::PP_BLOOM_UPSAMPLE = R"glsl(
    #version 330 core
    // This shader performs upsampling on a texture,
    // as taken from Call Of Duty method, presented at ACM Siggraph 2014.

    // Remember to add bilinear minification filter for this texture!
    // Remember to use a floating-point texture format (for HDR)!
    // Remember to use edge clamping for this texture!  

    // output ====================================================================
    layout (location = 0) out vec4 bloomOutput;

    // input varyings ============================================================
    in vec2 TexCoords;

    // uniforms ==================================================================
    uniform sampler2D srcTexture;
    uniform float u_FilterRadius = 0.1f;

    void main()
    {
        // The filter kernel is applied with a radius, specified in texture
        // coordinates, so that the radius will vary across mip resolutions.
        float x = u_FilterRadius;
        float y = u_FilterRadius;
        
        float alpha = texture(srcTexture, TexCoords).a;

        // Take 9 samples around current texel:
        // a - b - c
        // d - e - f
        // g - h - i
        // === ('e' is the current texel) ===
        vec3 a = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
        vec3 b = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y + y)).rgb;
        vec3 c = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;

        vec3 d = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y)).rgb;
        vec3 e = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y)).rgb;
        vec3 f = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y)).rgb;

        vec3 g = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
        vec3 h = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y - y)).rgb;
        vec3 i = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;

        // Apply weighted distribution, by using a 3x3 tent filter:
        //  1   | 1 2 1 |
        // -- * | 2 4 2 |
        // 16   | 1 2 1 |
        vec3 upsample = e*4.0;
        upsample += (b+d+f+h)*2.0;
        upsample += (a+c+g+i);
        upsample *= 1.0 / 16.0;

        bloomOutput = vec4(upsample, alpha);
    }
)glsl";

const std::string ShaderCode::PP_BLOOM_BLEND = R"glsl(
    #version 330 core

    // Output ====================================================================
    out vec4 FragColor;

    // Input =====================================================================
    in vec2 TexCoords;

    // enums =====================================================================
    // blend modes must match does defined by PP:BloomEffect::BLEND_*
    const int BLEND_MODE_MIX = 0;
    const int BLEND_MODE_ADD = 1;

    // Uniforms ==================================================================
    uniform sampler2D srcTexture;
    uniform sampler2D bloomTexture;
    uniform float u_BloomStrength = 0.04f;
    uniform int u_BlendMode = 0;
    uniform int u_Levels = 1;

    void main()
    {
        vec4 hdrColor = texture(srcTexture, TexCoords);
        vec3 bloomColor = texture(bloomTexture, TexCoords).rgb;

        // normalize bloomColor by the number of levels
        bloomColor *= 1.0 / float(u_Levels);

        // TODO: switch for additive blend vs linear mix
        vec3 result = vec3(0.0);
        switch (u_BlendMode) {
        case BLEND_MODE_MIX:
            result = mix(hdrColor.rgb, bloomColor, u_BloomStrength); // linear interpolation
            break;
        case BLEND_MODE_ADD:
            result = hdrColor.rgb + bloomColor * u_BloomStrength; // additive
            break;
        default:
            break;
        }

        FragColor = vec4(result, hdrColor.a);
    }

)glsl";


// ====================================================================================================
//  Text Shaders
// ====================================================================================================

// source: https://github.com/GreenLightning/gpu-font-rendering
const std::string ShaderCode::FONT_TEXT_VERT = R"glsl(
    #version 330 core

    uniform mat4 u_Projection;
    uniform mat4 u_View;
    uniform mat4 u_Model;

    layout (location = 0) in vec2 vertexPosition;
    layout (location = 1) in vec2 vertexUV;
    layout (location = 2) in int  vertexIndex;

    out vec2 uv;
    flat out int bufferIndex;

    void main() {
        gl_Position = u_Projection * u_View * u_Model * vec4(vertexPosition, 0, 1);
        uv = vertexUV;
        bufferIndex = vertexIndex;
    }
)glsl";

// source: https://github.com/GreenLightning/gpu-font-rendering
const std::string ShaderCode::FONT_TEXT_FRAG = R"glsl(
    #version 330 core

    // Based on: http://wdobbie.com/post/gpu-text-rendering-with-vector-textures/

    struct Glyph {
        int start, count;
    };

    struct Curve {
        vec2 p0, p1, p2;
    };

    uniform isamplerBuffer u_Glyphs;
    uniform samplerBuffer u_Curves;
    uniform vec4 u_Color;


    // Controls for debugging and exploring:

    // Size of the window (in pixels) used for 1-dimensional anti-aliasing along each rays.
    //   0 - no anti-aliasing
    //   1 - normal anti-aliasing
    // >=2 - exaggerated effect 
    uniform float antiAliasingWindowSize = 1.0;

    // Enable a second ray along the y-axis to achieve 2-dimensional anti-aliasing.
    uniform bool enableSuperSamplingAntiAliasing = true;


    in vec2 uv;
    flat in int bufferIndex;

    out vec4 result;

    Glyph loadGlyph(int index) {
        Glyph result;
        ivec2 data = texelFetch(u_Glyphs, index).xy;
        result.start = data.x;
        result.count = data.y;
        return result;
    }

    Curve loadCurve(int index) {
        Curve result;
        result.p0 = texelFetch(u_Curves, 3*index+0).xy;
        result.p1 = texelFetch(u_Curves, 3*index+1).xy;
        result.p2 = texelFetch(u_Curves, 3*index+2).xy;
        return result;
    }

    float computeCoverage(float inverseDiameter, vec2 p0, vec2 p1, vec2 p2) {
        if (p0.y > 0 && p1.y > 0 && p2.y > 0) return 0.0;
        if (p0.y < 0 && p1.y < 0 && p2.y < 0) return 0.0;

        // Note: Simplified from abc formula by extracting a factor of (-2) from b.
        vec2 a = p0 - 2*p1 + p2;
        vec2 b = p0 - p1;
        vec2 c = p0;

        float t0, t1;
        if (abs(a.y) >= 1e-5) {
            // Quadratic segment, solve abc formula to find roots.
            float radicand = b.y*b.y - a.y*c.y;
            if (radicand <= 0) return 0.0;
        
            float s = sqrt(radicand);
            t0 = (b.y - s) / a.y;
            t1 = (b.y + s) / a.y;
        } else {
            // Linear segment, avoid division by a.y, which is near zero.
            // There is only one root, so we have to decide which variable to
            // assign it to based on the direction of the segment, to ensure that
            // the ray always exits the shape at t0 and enters at t1. For a
            // quadratic segment this works 'automatically', see readme.
            float t = p0.y / (p0.y - p2.y);
            if (p0.y < p2.y) {
                t0 = -1.0;
                t1 = t;
            } else {
                t0 = t;
                t1 = -1.0;
            }
        }

        float alpha = 0;
        
        if (t0 >= 0 && t0 < 1) {
            float x = (a.x*t0 - 2.0*b.x)*t0 + c.x;
            alpha += clamp(x * inverseDiameter + 0.5, 0, 1);
        }

        if (t1 >= 0 && t1 < 1) {
            float x = (a.x*t1 - 2.0*b.x)*t1 + c.x;
            alpha -= clamp(x * inverseDiameter + 0.5, 0, 1);
        }

        return alpha;
    }

    vec2 rotate(vec2 v) {
        return vec2(v.y, -v.x);
    }

    void main() {
        float alpha = 0;

        // Inverse of the diameter of a pixel in uv units for anti-aliasing.
        vec2 inverseDiameter = 1.0 / (antiAliasingWindowSize * fwidth(uv));

        Glyph glyph = loadGlyph(bufferIndex);
        for (int i = 0; i < glyph.count; i++) {
            Curve curve = loadCurve(glyph.start + i);

            vec2 p0 = curve.p0 - uv;
            vec2 p1 = curve.p1 - uv;
            vec2 p2 = curve.p2 - uv;

            alpha += computeCoverage(inverseDiameter.x, p0, p1, p2);
            if (enableSuperSamplingAntiAliasing) {
                alpha += computeCoverage(inverseDiameter.y, rotate(p0), rotate(p1), rotate(p2));
            }
        }

        if (enableSuperSamplingAntiAliasing) {
            alpha *= 0.5;
        }

        alpha = clamp(alpha, 0.0, 1.0);
        result = u_Color * alpha;

        if (result.a < 0.001) {
            discard;
        }
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