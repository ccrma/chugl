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
