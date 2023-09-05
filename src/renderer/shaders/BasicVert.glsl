#version 330 core

// TODO: move transform matrices into Uniform Buffer Object
/*
With a Uniform Buffer Object (UBO), 
you can upload data once and have it accessible in all 
your shaders. 
E.g. no longer send Projection and View matrices to every shader. 
- camera position for lighting, 
- time value for animations, 
- screen width and height 
- etc
*/
// uniforms
uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

// attributes

// TODO: maybe auto-assign layout location using glcalls? allows having different vertex formats
layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TexCoord;
// layout (location = 2) in vec3 a_Col;

// varyings (interpolated and passed to frag shader)
// out vec3 v_Color;
out vec2 v_TexCoord;

void main()
{
   gl_Position = u_Projection * u_View * u_Model * vec4(a_Pos, 1.0);
   v_TexCoord = a_TexCoord;

   /*
   Note: openGL assume gl_Position is in clip space
   It then performs perspective division to transform into NDC space, which in openGL is from -1.0 --> 1.0 on all axis (xyz)
   It then uses the window params from glViewPort to map NDCs to screen coordinates in pixels e.g. between 0 and 800 (this process is called the viewport transform)


   */
};
