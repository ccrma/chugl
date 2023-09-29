#version 330 core

in vec3 v_Pos;
in vec3 v_Color;
in vec2 v_TexCoord;

// output
out vec4 FragColor;

// uniforms
uniform float u_PointSize;
uniform vec3 u_PointColor;
uniform sampler2D u_PointTexture;

void main()
{
    vec4 texColor = texture(u_PointTexture, gl_PointCoord);
    // FragColor = vec4(vec3(texColor.a), 1.0);

    FragColor = vec4( u_PointColor * v_Color * texColor.rgb, texColor.a );
    // FragColor = vec4( u_Point * v_Color * texColor.rgb, 1.0 )
    // FragColor = vec4(u_PointSize, 0.0, 0.0, 1.0);
    // FragColor = vec4(v_Color, 1.0);
    // FragColor = vec4(v_TexCoord, 0.0, 1.0);
}
