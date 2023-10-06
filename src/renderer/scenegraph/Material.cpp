#include "Material.h"

// base Material static defines ========================

// material options
const MaterialPolygonMode Material::POLYGON_FILL = MaterialPolygonMode::Fill;
const MaterialPolygonMode Material::POLYGON_LINE = MaterialPolygonMode::Line;
const MaterialPolygonMode Material::POLYGON_POINT = MaterialPolygonMode::Point;

// material uniforms
const std::string Material::POINT_SIZE_UNAME = "u_PointSize";
const std::string Material::LINE_WIDTH_UNAME = "u_LineWidth";

// ck type name map 
Material::CkTypeMap Material::s_CkTypeMap = {
    {MaterialType::Base, "Material"},
    {MaterialType::Normal, "NormMat"},
    {MaterialType::Phong, "PhongMat"},
    {MaterialType::Points, "PointsMat"},
    {MaterialType::Line, "LineMat"},
    {MaterialType::CustomShader, "ShaderMat"},
    {MaterialType::Mango, "MangoMat"},
};

const char * Material::CKName(MaterialType type) {
    return s_CkTypeMap[type].c_str();
}

// normMat static defines ========================
const std::string NormalMaterial::USE_LOCAL_NORMALS_UNAME = "u_UseLocalNormal";

// phong static defines ========================
const std::string PhongMaterial::DIFFUSE_MAP_UNAME = "u_Material.diffuseMap";
const std::string PhongMaterial::SPECULAR_MAP_UNAME = "u_Material.specularMap";
const std::string PhongMaterial::DIFFUSE_COLOR_UNAME = "u_Material.diffuseColor";
const std::string PhongMaterial::SPECULAR_COLOR_UNAME = "u_Material.specularColor";
const std::string PhongMaterial::SHININESS_UNAME = "u_Material.shininess";

// points mat static defines ========================
const std::string PointsMaterial::POINT_SIZE_ATTENUATION_UNAME = "u_PointSizeAttenuation";
const std::string PointsMaterial::POINT_COLOR_UNAME = "u_PointColor";
const std::string PointsMaterial::POINT_SPRITE_TEXTURE_UNAME = "u_PointTexture";

// line mat static defines ========================
const std::string LineMaterial::LINE_COLOR_UNAME = "u_LineColor";
const unsigned int LineMaterial::LINE_SEGMENTS_MODE = MaterialPrimitiveMode::Lines;
const unsigned int LineMaterial::LINE_STRIP_MODE = MaterialPrimitiveMode::LineStrip;
const unsigned int LineMaterial::LINE_LOOP_MODE = MaterialPrimitiveMode::LineLoop;