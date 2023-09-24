#include "Material.h"

// base Material static defines ========================

// material options
const MaterialOptionParam Material::WIREFRAME = MaterialOptionParam::WireFrame;
const MaterialOptionParam Material::WIREFRAME_WIDTH = MaterialOptionParam::WireFrameWidth;



// normMat static defines ========================
const std::string NormalMaterial::USE_LOCAL_NORMALS_UNAME = "u_UseLocalNormal";

// phong static defines ========================
const std::string PhongMaterial::DIFFUSE_MAP_UNAME = "u_Material.diffuseMap";
const std::string PhongMaterial::SPECULAR_MAP_UNAME = "u_Material.specularMap";
const std::string PhongMaterial::DIFFUSE_COLOR_UNAME = "u_Material.diffuseColor";
const std::string PhongMaterial::SPECULAR_COLOR_UNAME = "u_Material.specularColor";
const std::string PhongMaterial::SHININESS_UNAME = "u_Material.shininess";