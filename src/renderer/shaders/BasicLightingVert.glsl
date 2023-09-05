#version 330 core

#include "include/Globals.glsl"

// attributes
layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;

// varyings (interpolated and passed to frag shader)
out vec3 v_Pos;
out vec3 v_Normal;		 // world space normal
out vec3 v_LocalNormal;  // local space normal
out vec2 v_TexCoord;

void main()
{
   v_Pos = vec3(u_Model * vec4(a_Pos, 1.0));  // world space position
   v_TexCoord = a_TexCoord;

   v_LocalNormal = a_Normal;  
   v_Normal = vec3(u_Normal * vec4(a_Normal, 0.0));

   gl_Position = u_Projection * u_View * vec4(v_Pos, 1.0);
};
