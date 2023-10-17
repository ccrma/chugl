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

struct LightParams {
	float intensity;
	// color info
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	// falloff for point lights
	float linear;
	float quadratic;
};

class Light : public SceneGraphObject
{
public:
	Light() {
		m_Params = {
		    1.0, 			   // intensity
			glm::vec3(0.3f),   // amb
			glm::vec3(0.8f),   // diff
			glm::vec3(0.25f),   // spec
			0.09f,  		   // linear
			0.032f			   // quadratic
		};
	};
	virtual ~Light() {};

	virtual bool IsLight() override { return true;  }
	virtual LightType GetLightType() = 0;
	virtual void SetShaderUniforms(Shader* shader, int index) = 0;
public: // ck name
	typedef std::unordered_map<LightType, const std::string, EnumClassHash> CkTypeMap;
	static CkTypeMap s_CkTypeMap;
	static const char * CKName(LightType type) { return s_CkTypeMap[type].c_str(); }
	virtual const char * myCkName() { return CKName(GetLightType()); }

public: // shared params
	LightParams m_Params;
};

class PointLight : public Light
{
public:
	PointLight() {}
	
	virtual LightType GetLightType() override { return LightType::Point; }
	// TODO: decouple from shader implementation
	virtual void SetShaderUniforms(Shader* shader, int index) override;
	virtual Light* Clone() override { 
		auto* light = new PointLight(*this); 
		light->SetID(GetID());
		return light;
	}


};

class DirLight : public Light
{
public:
	DirLight() {}

	virtual LightType GetLightType() override { return LightType::Directional; }
	virtual void SetShaderUniforms(Shader* shader, int index) override;
	virtual Light* Clone() override { 
		auto* light = new DirLight(*this); 
		light->SetID(GetID());
		return light;
	}
};