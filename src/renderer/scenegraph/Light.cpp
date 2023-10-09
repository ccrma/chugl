#include "scenegraph/Light.h"
#include "scenegraph/SceneGraphObject.h"
#include "Shader.h"

// ckname 
Light::CkTypeMap Light::s_CkTypeMap = {
	{LightType::Base, "GLight"},
	{LightType::Point, "GPointLight"},
	{LightType::Directional, "GDirLight"},
	{LightType::Spot, "GSpotLight"}
};

// doesn't compile on mac
// #define GLM_ENABLE_EXPERIMENTAL
// #include "glm/ext.hpp"

void PointLight::SetShaderUniforms(Shader *shader, int index)
{
    std::string lightName = "u_PointLights[" + std::to_string(index) + "].";
    // set color properties
    shader->setFloat3(lightName + "ambient", m_Params.ambient);
    shader->setFloat3(lightName + "diffuse", m_Params.diffuse);
    shader->setFloat3(lightName + "specular", m_Params.specular);

    // set range and dropoffs
    shader->setFloat(lightName + "intensity", m_Params.intensity);
    shader->setFloat(lightName + "linear", m_Params.linear);
    shader->setFloat(lightName + "quadratic", m_Params.quadratic);

    // set position
    shader->setFloat3(lightName + "position", GetWorldPosition());

    // std::cerr << "setting light position" << lightName + "position" << glm::to_string(m_Position) << std::endl;
}

void DirLight::SetShaderUniforms(Shader *shader, int index)
{
    std::string lightName = "u_DirLights[" + std::to_string(index) + "].";
    // set color properties
    shader->setFloat3(lightName + "ambient", m_Params.ambient);
    shader->setFloat3(lightName + "diffuse", m_Params.diffuse);
    shader->setFloat3(lightName + "specular", m_Params.specular);

    // set range and dropoffs
    shader->setFloat(lightName + "intensity", m_Params.intensity);

    // set direcion
    shader->setFloat3(lightName + "direction", GetForward());
}
