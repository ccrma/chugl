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
	CustomShader
};

enum class UniformType {
	Float = 0,
	Float2,
	Float3,
	Float4,
	Mat4,
	Int,
	Int2,
	Int3,
	Int4,
	Bool,
	Texture
};

struct MaterialUniform {
	UniformType type;
	std::string name;
	union {
		float f;
		float f2[2];
		float f3[3];
		float f4[4];
		int i;
		int i2[2];
		int i3[3];
		int i4[4];
		bool b;
		size_t texID;  // needs to be the ID so that when material is cloned into the renderer scenegraph it doesn't hold a chuck-side pointer reference
		// TODO: don't need to support Mat4 until matrices added to chuck
		// also at that point might want to stop using a union to be more space efficient
		// but that also would make memory access less efficient
	};

	// constructors  (because prior c++20 you can only initialize the first type in a union??)
	// also why doesn't cpp support C-style designated initializers?
	static MaterialUniform Create(std::string s, float f0) {
		MaterialUniform m; m.type = UniformType::Float; m.name = s; m.f = f0; return m;
	}
	static MaterialUniform Create(std::string s, float f0, float f1) {
		MaterialUniform m; m.type = UniformType::Float2; m.name = s; m.f2[0] = f0; m.f2[1] = f1; return m;
	}
	static MaterialUniform Create(std::string s, float f0, float f1, float f2) {
		MaterialUniform m; m.type = UniformType::Float3; m.name = s; m.f3[0] = f0; m.f3[1] = f1; m.f3[2] = f2; return m;
	}
	static MaterialUniform Create(std::string s, glm::vec3 v) {
		MaterialUniform m; m.type = UniformType::Float3; m.name = s; m.f3[0] = v.x; m.f3[1] = v.y; m.f3[2] = v.z; return m;
	}
	static MaterialUniform Create(std::string s, float f0, float f1, float f2, float f3) {
		MaterialUniform m; m.type = UniformType::Float4; m.name = s; m.f4[0] = f0; m.f4[1] = f1; m.f4[2] = f2; m.f4[3] = f3; return m;
	}
	static MaterialUniform Create(std::string s, glm::vec4 v) {
		MaterialUniform m; m.type = UniformType::Float4; m.name = s; m.f4[0] = v.x; m.f4[1] = v.y; m.f4[2] = v.z; m.f4[3] = v.w; return m;
	}
	static MaterialUniform Create(std::string s, int i0) {
		MaterialUniform m; m.type = UniformType::Int; m.name = s; m.i = i0; return m;
	}
	static MaterialUniform Create(std::string s, int i0, int i1) {
		MaterialUniform m; m.type = UniformType::Int2; m.name = s; m.i2[0] = i0; m.i2[1] = i1; return m;
	}
	static MaterialUniform Create(std::string s, int i0, int i1, int i2) {
		MaterialUniform m; m.type = UniformType::Int3; m.name = s; m.i3[0] = i0; m.i3[1] = i1; m.i3[2] = i2; return m;
	}
	static MaterialUniform Create(std::string s, int i0, int i1, int i2, int i3) {
		MaterialUniform m; m.type = UniformType::Int4; m.name = s; m.i4[0] = i0; m.i4[1] = i1; m.i4[2] = i2; m.i4[3] = i3; return m;
	}
	static MaterialUniform Create(std::string s, bool b0) {
		MaterialUniform m; m.type = UniformType::Bool; m.name = s; m.b = b0; return m;
	}
	static MaterialUniform Create(std::string s, size_t texID) {
		MaterialUniform m; m.type = UniformType::Texture; m.name = s; m.texID = texID; return m;
	}
};


// Material abstract base class
class Material : public SceneGraphNode
{
public:
	Material() : m_WireFrame(false), m_WireFrameLineWidth(1.0f) {};
	virtual ~Material() {}

	virtual MaterialType GetMaterialType() { return MaterialType::Base; }
	virtual std::vector<MaterialUniform>& GetLocalUniforms() { return m_Uniforms;  }  // for setting properties specific to the material, e.g. color

	virtual Material* Clone() = 0;

	// commands for telling a material how to update itself
	virtual std::vector<MaterialUniform>* GenUpdate() { return new std::vector<MaterialUniform>(m_Uniforms); }
	virtual void ApplyUpdate(std::vector<MaterialUniform>* uniform_data) { m_Uniforms = *uniform_data; }

	inline void SetWireFrame(bool wf) { m_WireFrame = wf; }
	inline void SetWireFrameWidth(float width) { m_WireFrameLineWidth = width; }
	inline bool GetWireFrame() { return m_WireFrame; }
	inline float GetWireFrameWidth() { return m_WireFrameLineWidth; }

	int AddUniform(MaterialUniform uniform) {
		m_Uniforms.push_back(uniform);
		return m_Uniforms.size() - 1;
	}

	// Note: keep these public so they are accessible by default copy constructor
	bool m_WireFrame;
	float m_WireFrameLineWidth;

	// uniform list
	std::vector<MaterialUniform> m_Uniforms;
};

// material that colors using worldspace normals as rgb
class NormalMaterial : public Material
{
public:
	NormalMaterial() {
		AddUniform(MaterialUniform::Create("u_UseLocalNormals", false));
	}

	virtual MaterialType GetMaterialType() override { return MaterialType::Normal; }
	virtual Material* Clone() override { 
		auto* mat = new NormalMaterial(*this);
		mat->SetID(GetID());
		return mat;
	}

	void UseLocalNormals() { m_Uniforms[0].b = true; }
	void UseWorldNormals() { m_Uniforms[0].b = false; }
	bool GetUseLocalNormals() { return m_Uniforms[0].b; }
};

// phong lighting (ambient + diffuse + specular)
class PhongMaterial : public Material
{
public:
	PhongMaterial(
		size_t diffuseMapID = 0,
		size_t specularMapID = 0,
		glm::vec3 diffuseColor = glm::vec3(1.0f),
		glm::vec3 specularColor = glm::vec3(1.0f),
		float logShininess = 5  // ==> 32 shininess
	) {
		AddUniform(MaterialUniform::Create("u_Material.diffuseMap", diffuseMapID));
		AddUniform(MaterialUniform::Create("u_Material.specularMap", specularMapID));
		AddUniform(MaterialUniform::Create("u_Material.diffuseColor", diffuseColor));
		AddUniform(MaterialUniform::Create("u_Material.specularColor", specularColor));
		AddUniform(MaterialUniform::Create("u_Material.shininess", std::pow(2.0f, logShininess)));
	}

	virtual MaterialType GetMaterialType() override { return MaterialType::Phong; }
	virtual Material* Clone() override { 
		auto* mat = new PhongMaterial(*this);
		mat->SetID(GetID());
		return mat;
	}

	// uniform getters
	size_t GetDiffuseMapID() { return m_Uniforms[0].texID; }
	size_t GetSpecularMapID() { return m_Uniforms[1].texID; }
	glm::vec3 GetDiffuseColor() { return glm::vec3(m_Uniforms[2].f3[0], m_Uniforms[2].f3[1], m_Uniforms[2].f3[2]); }
	glm::vec3 GetSpecularColor() { return glm::vec3(m_Uniforms[3].f3[0], m_Uniforms[3].f3[1], m_Uniforms[3].f3[2]); }
	float GetLogShininess() { return m_Uniforms[4].f; }

	// uniform setters
	void SetDiffuseMap(CGL_Texture* texture) { m_Uniforms[0].texID = texture->GetID(); }
	void SetSpecularMap(CGL_Texture* texture) { m_Uniforms[1].texID = texture->GetID(); }
	void SetDiffuseColor(float r, float g, float b) { m_Uniforms[2].f3[0] = r; m_Uniforms[2].f3[1] = g; m_Uniforms[2].f3[2] = b; }
	void SetSpecularColor(float r, float g, float b) { m_Uniforms[3].f3[0] = r; m_Uniforms[3].f3[1] = g; m_Uniforms[3].f3[2] = b; }
	void SetLogShininess(float logShininess) { m_Uniforms[4].f = std::pow(2.0f, logShininess); }
};
