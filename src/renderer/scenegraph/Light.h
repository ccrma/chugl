#pragma once

#include "SceneGraphObject.h"
#include <glm/glm.hpp>

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

class Light : public SceneGraphObject
{
public:

	virtual bool IsLight() override { return true;  }
};
