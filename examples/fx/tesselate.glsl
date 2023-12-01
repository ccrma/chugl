#version 330 core
out vec4 FragColor;

// don't change this name!
in vec2 TexCoords;

// don't change this name!
uniform sampler2D screenTexture;

// uniforms
uniform ivec2 u_Tesselation = ivec2(2, 2);

void main()
{ 
    vec2 uv = fract(TexCoords * u_Tesselation);
    vec4 color = texture(screenTexture, uv);

    FragColor = color;
}