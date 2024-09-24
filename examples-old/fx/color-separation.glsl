#version 330 core
out vec4 FragColor;

// don't change this name!
in vec2 TexCoords;

// don't change this name!
uniform sampler2D screenTexture;

// uniforms
uniform vec2 u_Separation = vec2(0.002, 0.0);

void main()
{ 
    vec2 uv = TexCoords;
    vec4 color = texture(screenTexture, uv);

    // apply color separation on rgb channels
    color.r = texture(screenTexture, vec2(uv.x + u_Separation.x, uv.y + u_Separation.y)).r;
    color.g = texture(screenTexture, vec2(uv.x, uv.y)).g;
    color.b = texture(screenTexture, vec2(uv.x - u_Separation.x, uv.y - u_Separation.y)).b;

    FragColor = color;
}