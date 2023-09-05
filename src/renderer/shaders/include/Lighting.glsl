// Include this to access all the scene lighting info

const int LIGHT_TYPE_NONE  = 0;
const int LIGHT_TYPE_DIR   = 1;
const int LIGHT_TYPE_POINT = 2;
const int LIGHT_TYPE_SPOT  = 3;

struct Light {
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

const int MAX_LIGHTS = 25;
uniform int u_NumLights = 0;
uniform Light u_Lights[MAX_LIGHTS];

