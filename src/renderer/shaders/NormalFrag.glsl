#version 330 core
// basic shader for coloring by normals

uniform int u_UseLocalNormal = 0;

in vec3 v_Normal;       // world space normal
in vec3 v_LocalNormal;  // local space normal

// output
out vec4 FragColor;

// fragment shader for light sources to always be white
void main()
{
    FragColor = (1 - u_UseLocalNormal) * vec4(normalize(v_Normal), 1.0) + 
                (u_UseLocalNormal) * vec4(normalize(v_LocalNormal), 1.0);
}
