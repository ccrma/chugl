#include "Material.h"
#include "Texture.h"

void PhongMaterial::SetLocalUniforms(Shader* shader) {
	// set textures TODO support actual textures,hardcoding pixel for now
	// m_Uniforms.diffuseMap->Bind(0);
	// m_Uniforms.specularMap->Bind(1);
	
	Texture::GetDefaultWhiteTexture()->Bind(0);
	Texture::GetDefaultWhiteTexture()->Bind(1);
	shader->setInt("u_Material.diffuseMap", 0);
	shader->setInt("u_Material.specularMap", 1);

	// set uniforms
	shader->setFloat3("u_Material.diffuseColor", m_Uniforms.diffuseColor);
	shader->setFloat3("u_Material.specularColor", m_Uniforms.specularColor);
	shader->setFloat("u_Material.shininess", std::powf(2.0, m_Uniforms.logShininess));
}
