#version 330 core

in vec3 v_Pos;
in vec2 v_TexCoord;

// output
out vec4 FragColor;

// uniforms
uniform float u_Time;

void main()
{
    FragColor = vec4(
        v_TexCoord, sin(u_Time), 1.0
    );
}
