#include "Material.h"

// base Material static defines ========================

// material options
const t_CKUINT Material::POLYGON_FILL = MaterialPolygonMode::Fill;
const t_CKUINT Material::POLYGON_LINE = MaterialPolygonMode::Line;
const t_CKUINT Material::POLYGON_POINT = MaterialPolygonMode::Point;

// base material uniforms
const std::string Material::POINT_SIZE_UNAME = "u_PointSize";
const std::string Material::LINE_WIDTH_UNAME = "u_LineWidth";
const std::string Material::COLOR_UNAME = "u_Color";

// normal mat uniforms
const std::string Material::USE_LOCAL_NORMALS_UNAME = "u_UseLocalNormal";

// phont mat uniforms (TODO move out of material struct?)
const std::string Material::DIFFUSE_MAP_UNAME = "u_DiffuseMap";
const std::string Material::SPECULAR_MAP_UNAME = "u_SpecularMap";
const std::string Material::SPECULAR_COLOR_UNAME = "u_Material.specularColor";
const std::string Material::SHININESS_UNAME = "u_Material.shininess";

const t_CKUINT Material::LINE_SEGMENTS_MODE = MaterialPrimitiveMode::Lines;
const t_CKUINT Material::LINE_STRIP_MODE = MaterialPrimitiveMode::LineStrip;
const t_CKUINT Material::LINE_LOOP_MODE = MaterialPrimitiveMode::LineLoop;

const std::string Material::POINT_SIZE_ATTENUATION_UNAME = "u_PointSizeAttenuation";
const std::string Material::POINT_SPRITE_TEXTURE_UNAME = "u_PointTexture";

// env map uniforms (must match ShaderCode::ENV_MAP_UNIFORMS)
const std::string Material::SKYBOX_UNAME = "u_Skybox";
const std::string Material::ENV_MAP_ENABLED_UNAME = "u_EnvMapParams.enabled";
const std::string Material::ENV_MAP_INTENSITY_UNAME = "u_EnvMapParams.intensity";
const std::string Material::ENV_MAP_BLEND_MODE_UNAME = "u_EnvMapParams.blendMode";
const std::string Material::ENV_MAP_METHOD_UNAME = "u_EnvMapParams.method";
const std::string Material::ENV_MAP_RATIO_UNAME = "u_EnvMapParams.ratio";

// env map blend modes (must match ShaderCode::ENV_MAP_UNIFORMS)
const int Material::BLEND_MODE_ADD = 0;
const int Material::BLEND_MODE_MULTIPLY = 1;
const int Material::BLEND_MODE_MIX = 2;

// env mapping methods (must match ShaderCode::ENV_MAP_UNIFORMS)
const int Material::ENV_MAP_METHOD_REFLECTION = 0;
const int Material::ENV_MAP_METHOD_REFRACTION = 1;


// ck type name map 
Material::CkTypeMap Material::s_CkTypeMap = {
    {MaterialType::Base, "Material"},
    {MaterialType::Normal, "NormalsMaterial"},
    {MaterialType::Phong, "PhongMaterial"},
    {MaterialType::Points, "PointMaterial"},
    {MaterialType::Line, "LineMaterial"},
    {MaterialType::CustomShader, "ShaderMaterial"},
    {MaterialType::Mango, "MangoUVMaterial"},
    {MaterialType::Flat, "FlatMaterial"},
};



// points mat static defines ========================
