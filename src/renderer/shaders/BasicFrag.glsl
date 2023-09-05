#version 330 core

// uniforms
// TODO: should textures go into a universal uniform buffer object?
// Note: uniforms not used in code will be optimized away during compilation!!
uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;
// uniform vec4 u_Color;

// varyings
// in vec3 v_Color;
in vec2 v_TexCoord;

// output
out vec4 FragColor;

void main()
{
//	vec4 texColor = texture(u_Texture, v_TexCoord);
	vec4 texColor = mix(texture(u_Texture1, v_TexCoord), texture(u_Texture2, v_TexCoord), .5);
	// FragColor = texColor * u_Color;
	FragColor = vec4(texColor.rgb, 1.0);
	
	// debug UVs
	// FragColor = vec4(v_TexCoord, 0.0, 1.0);
}
