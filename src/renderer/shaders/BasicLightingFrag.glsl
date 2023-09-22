#version 330 core

#include include/Globals.glsl
#include include/Lighting.glsl

// material uniforms =================================================================
// TODO: add builtin material types
// http://devernay.free.fr/cours/opengl/materials.html
struct Material {
    // textures
	sampler2D diffuseMap;
	sampler2D specularMap;

    // colors
    vec3 diffuseColor;
    vec3 specularColor;

    // specular highlights
    float shininess;  // range from (0, 2^n). must be > 0. logarithmic scale.
};
uniform Material u_Material;

// varyings (interpolated and passed to frag shader) ==============================
in vec3 v_Pos;  // world space frag position
in vec3 v_Normal;
in vec2 v_TexCoord;

// output ==========================================================================
out vec4 FragColor;

// functions to calculate light contribution ======================================
vec3 CalcDirLight(  // contribution of directional light
    DirLight light, vec3 normal, vec3 viewDir,
    float shininess, vec3 diffuseCol, vec3 specularCol
) {
	vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // combine results
    vec3 ambient  = light.ambient  * diffuseCol;
    vec3 diffuse  = light.diffuse  * diff * diffuseCol;
    vec3 specular = light.specular * spec * specularCol;
    return (ambient + diffuse + specular) * light.intensity;
}

// contribution from point lights
vec3 CalcPointLight(
    PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,
    float shininess, vec3 diffuseCol, vec3 specularCol
) {
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = max(pow(max(dot(viewDir, reflectDir), 0.0), shininess), 0.0);
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = light.ambient  * diffuseCol;
    vec3 diffuse  = light.diffuse  * diff * diffuseCol;
    vec3 specular = light.specular * spec * specularCol;

    // attenuate
    return (ambient + diffuse + specular) * attenuation * light.intensity;
} 

// TODO
// vec3 CalcSpotLight() {}

// main =====================================================================================
void main()
{
    vec3 norm = normalize(v_Normal);  // fragment normal direction
    vec3 viewDir = normalize(u_ViewPos - v_Pos);  // direction from camera to this frag

    // material color properties (ignore alpha channel for now)
    vec3 diffuse = texture(u_Material.diffuseMap, v_TexCoord).xyz * u_Material.diffuseColor;
    vec3 specular = texture(u_Material.specularMap, v_TexCoord).xyz * u_Material.specularColor;

    vec3 result = vec3(0.0);

    // loop through point lights
    for (int i = 0; i < u_NumPointLights; i++) {
        PointLight light = u_PointLights[i];
        result += CalcPointLight(
            light, norm, v_Pos, viewDir, u_Material.shininess, diffuse, specular
        );
    }

    // loop through directional lights
    for (int i = 0; i < u_NumDirLights; i++) {
        DirLight light = u_DirLights[i];
        result += CalcDirLight(
            light, norm, viewDir, u_Material.shininess, diffuse, specular
        );
    }

    FragColor = vec4(result, 1.0);
}
