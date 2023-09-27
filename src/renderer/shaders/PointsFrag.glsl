#version 330 core

in vec3 v_Pos;
in vec3 v_Color;
in vec2 v_TexCoord;

// output
out vec4 FragColor;

// uniforms
uniform float u_PointSize;

void main()
{
    // FragColor = vec4( v_Color, 1.0 );
    // FragColor = vec4(u_PointSize, 0.0, 0.0, 1.0);
    // FragColor = vec4(v_Color, 1.0);
    FragColor = vec4(v_TexCoord, 0.0, 1.0);
}
