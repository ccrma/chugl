#include "scenegraph/Light.h"
#include "scenegraph/SceneGraphObject.h"
#include "Shader.h"

// ckname 
Light::CkTypeMap Light::s_CkTypeMap = {
	{LightType::Base, "GLight"},
	{LightType::Point, "PointLight"},
	{LightType::Directional, "DirLight"},
	{LightType::Spot, "SpotLight"}
};

// doesn't compile on mac
// #define GLM_ENABLE_EXPERIMENTAL
// #include "glm/ext.hpp"

void PointLight::SetShaderUniforms(Shader *shader, int index)
{
    std::string lightName = "u_PointLights[" + std::to_string(index) + "].";
    // set color properties
    shader->setFloat3(lightName + "ambient", m_Ambient);
    shader->setFloat3(lightName + "diffuse", m_Diffuse);
    shader->setFloat3(lightName + "specular", m_Specular);

    // set range and dropoffs
    shader->setFloat(lightName + "intensity", m_Intensity);
    shader->setFloat(lightName + "constant", m_Constant);
    shader->setFloat(lightName + "linear", m_Linear);
    shader->setFloat(lightName + "quadratic", m_Quadratic);

    // set position
    shader->setFloat3(lightName + "position", GetWorldPosition());

    // std::cerr << "setting light position" << lightName + "position" << glm::to_string(m_Position) << std::endl;
}

void DirLight::SetShaderUniforms(Shader *shader, int index)
{
    std::string lightName = "u_DirLights[" + std::to_string(index) + "].";
    // set color properties
    shader->setFloat3(lightName + "ambient", m_Ambient);
    shader->setFloat3(lightName + "diffuse", m_Diffuse);
    shader->setFloat3(lightName + "specular", m_Specular);

    // set range and dropoffs
    shader->setFloat(lightName + "intensity", m_Intensity);

    // set direcion
    shader->setFloat3(lightName + "direction", GetForward());
}
