// Include this to access all the scene lighting info
struct PointLight {
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
};

struct DirLight {
	vec3 direction;

	float intensity;

	// color info
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

const int MAX_LIGHTS = 12;
uniform int u_NumPointLights = 0;
uniform int u_NumDirLights = 0;
uniform PointLight u_PointLights[MAX_LIGHTS];
uniform DirLight u_DirLights[MAX_LIGHTS];

