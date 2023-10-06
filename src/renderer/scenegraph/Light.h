#pragma once

#include "SceneGraphObject.h"
#include "SceneGraphNode.h"
#include <glm/glm.hpp>
#include <unordered_map>

/*
struct LightInfo {  // must match include/Lighting.glsl setup!!
	// color info
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	
	// range and dropoffs
	float intensity;
	float constant;
    float linear;
    float quadratic;  

	// transform
	vec3 position;
	vec3 direction;
	
	// type enum 
	int type;
};
*/

// forward decls ========================================
class Shader;
// end forward decls ====================================

enum class LightType {  // must match include/Lighting.glsl setup!!
	Base = 0,
	Directional,
	Point,
	Spot
};

class Light : public SceneGraphObject
{
public:
	Light() {};
	virtual ~Light() {};

	virtual bool IsLight() override { return true;  }
	virtual LightType GetLightType() = 0;
	virtual void SetShaderUniforms(Shader* shader, int index) = 0;
	virtual Light* Clone() = 0;
public: // ck name
	typedef std::unordered_map<LightType, const std::string, EnumClassHash> CkTypeMap;
	static CkTypeMap s_CkTypeMap;
	static const char * CKName(LightType type) { return s_CkTypeMap[type].c_str(); }
	virtual const char * myCkName() { return CKName(GetLightType()); }
};

class PointLight : public Light
{
public:
	PointLight() : 
		m_Intensity(1.0f), 
		m_Linear(0.09f), 
		m_Quadratic(0.032f),
		m_Ambient(0.3f),   
		m_Diffuse(1.0f), 
		m_Specular(.1f)
	{}
	
	virtual LightType GetLightType() override { return LightType::Point; }
	// TODO: decouple from shader implementation
	virtual void SetShaderUniforms(Shader* shader, int index) override;
	virtual Light* Clone() override { return new PointLight(*this); }

	float m_Intensity;
	float m_Linear;
	float m_Quadratic;

	// color info
	glm::vec3 m_Ambient;
	glm::vec3 m_Diffuse;
	glm::vec3 m_Specular;
};

class DirLight : public Light
{
public:
	DirLight() : 
		m_Intensity(1.0f), // all ranges [0, 1]
		m_Ambient(0.3f),   
		m_Diffuse(1.0f), 
		m_Specular(.1f)
	{}

	virtual LightType GetLightType() override { return LightType::Directional; }
	virtual void SetShaderUniforms(Shader* shader, int index) override;
	virtual Light* Clone() override { return new DirLight(*this); }

	float m_Intensity;

	// color info
	glm::vec3 m_Ambient;
	glm::vec3 m_Diffuse;
	glm::vec3 m_Specular;
};