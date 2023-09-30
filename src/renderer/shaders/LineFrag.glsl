#version 330 core

in vec3 v_Pos;
in vec3 v_Color;
// in vec2 v_TexCoord;

// output
out vec4 FragColor;

// uniforms
uniform float u_LineWidth;
uniform vec3 u_LineColor;

void main()
{
    FragColor = vec4( u_LineColor * v_Color, 1.0 );
}
