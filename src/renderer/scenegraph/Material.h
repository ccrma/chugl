#pragma once

#include "SceneGraphNode.h"
#include "SceneGraphObject.h"
#include "../Texture.h"
#include "../Shader.h"
#include <string>
// #include <cmath>

// builtin material type struct
enum class MaterialType {
	Base = 0,
	Normal,
	Phong,
};

// helper struct for global uniforms

// Material abstract base class
class Material : public SceneGraphNode
{
public:
	Material() : m_WireFrame(false), m_WireFrameLineWidth(1.0f) {};
	virtual ~Material() {}

	virtual MaterialType GetMaterialType() { return MaterialType::Base; }

	// TODO: need to decouple this from the scenegraph material,
	// implement entirely in RenderMaterial but to do so need way to support generic
	// uniform parameter specification.
	// e.g. specify a uniform name, value, and type maybe stored in a cpu-side buffer
	virtual void SetLocalUniforms(Shader* shader) = 0;  // for setting properties specific to the material, e.g. color
	virtual Material* Clone(bool copyID = true) = 0;

	// these two commands are for telling a material how to update itself
	// via an update command
	// TODO: is there a better way to do this in cpp that doesn't involve void * ?
	virtual void * GenUpdate() = 0;
	virtual void FreeUpdate(void* data) = 0;
	virtual void ApplyUpdate(void* data) = 0;

	inline void SetWireFrame(bool wf) { m_WireFrame = wf; }
	inline void SetWireFrameWidth(float width) { m_WireFrameLineWidth = width; }
	inline bool GetWireFrame() { return m_WireFrame; }
	inline float GetWireFrameWidth() { return m_WireFrameLineWidth; }

private:
	// TODO: wireframing
	bool m_WireFrame;
	float m_WireFrameLineWidth;
};

// material that colors using worldspace normals as rgb
class NormalMaterial : public Material
{
public:
	NormalMaterial() : m_UseLocalNormals(false) {}
	virtual MaterialType GetMaterialType() override { return MaterialType::Normal; }
	virtual void SetLocalUniforms(Shader* shader) override {
		// set uniforms
		shader->setInt("u_UseLocalNormal", m_UseLocalNormals ? 1 : 0);
	}
	virtual Material* Clone(bool copyID = true) override {
		
		NormalMaterial* normMat = new NormalMaterial();
		// id
		if (copyID) { normMat->SetID(this->GetID()); }

		// material uniforms
		if (m_UseLocalNormals)
			normMat->UseLocalNormals();
		else
			normMat->UseWorldNormals();

		return normMat;
	}
	virtual void * GenUpdate() override {
		return new bool{ m_UseLocalNormals };
	}

	virtual void FreeUpdate(void* data) override {
		delete (bool*)data;
	}


	virtual void ApplyUpdate(void* data) override {
		assert(data && "normal material update data is null!");
		m_UseLocalNormals = *(bool*)data;
	}
	void UseLocalNormals() { m_UseLocalNormals = true; }
	void UseWorldNormals() { m_UseLocalNormals = false; }
private:
	bool m_UseLocalNormals;
	// none, no textures!
};

// match the materials definition in the frag shader!
struct PhongMatUniforms {
    // textures (TODO change to shared ptr)
	// can't use references here because textures can be unitialized, and they can be reassigned
	Texture * diffuseMap, * specularMap; 
    // colors
    glm::vec3 diffuseColor, specularColor;
    // specular highlights
    float logShininess;  // LOG of shininess. e.g. logShininess = 5 ==> shininess = 2^5 = 32.
};
// phone lighting (ambient + diffuse + specular)
class PhongMaterial : public Material
{
public:
	// TODO: use a locator class or something better to get the shader paths 
	PhongMaterial(
		Texture* diffuseMap = &Texture::DefaultWhiteTexture, 
		Texture* specularMap = &Texture::DefaultWhiteTexture,
		glm::vec3 diffuseColor = glm::vec3(1.0f),
		glm::vec3 specularColor = glm::vec3(1.0f),
		float logShininess = 5  // ==> 32 shininess
	) :
		m_Uniforms({ diffuseMap, specularMap, diffuseColor, specularColor, logShininess })
	{

	}
	virtual MaterialType GetMaterialType() override { return MaterialType::Phong; }
	virtual void SetLocalUniforms(Shader* shader) override;
private:
	PhongMatUniforms m_Uniforms;
};
