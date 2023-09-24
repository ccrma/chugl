#version 330 core

in vec3 v_Pos;
in vec2 v_TexCoord;

// output
out vec4 FragColor;

// uniforms
uniform float u_Time;
uniform sampler2D u_AudioTexture; // TODO: can do this faster with Uniform Buffer Objects
uniform int u_AmplitudeIndex;  // where we are currently writing to in amp history graph


// helper fns at: https://github.com/mrdoob/three.js/blob/dev/src/renderers/shaders/ShaderChunk/common.glsl.js
 
// Created by inigo quilez - iq/2013
// https://www.youtube.com/c/InigoQuilez
// https://iquilezles.org/
// source: https://www.shadertoy.com/view/Xds3Rr

void main()
{
    vec2 uv = v_TexCoord;

    // the sound texture is 512x2
    int tx = int(uv.x*512.0);
    
	// first chan is frequency data (48Khz/4 in 512 texels, meaning 23 Hz per texel)
	float fft  = texelFetch( u_AudioTexture, ivec2(tx,0), 0 ).x; 

    // second channel is the sound wave, one texel is one mono sample
    float wave = texelFetch( u_AudioTexture, ivec2(tx,0), 0 ).y;

    // third channel is the amplitude history. first / leftmost texel is most recent RMS
    float amp = texelFetch( u_AudioTexture, ivec2(tx,0), 0 ).z;
	
	// convert frequency to colors
	vec3 col = vec3( fft, 4.0*fft*(1.0-fft), 1.0-fft ) * fft * step(.5, uv.y);

    // add wave form on top	
	col += 1.0 -  smoothstep( 0.0, 0.005, abs(wave - uv.y) );

    // add amplitude bargraph
    col += step(uv.y, 0.5) * step(uv.y, amp) * vec3(1.0);

    // add redline at 0.5
    col += step(uv.y, 0.5) * (1.0 - smoothstep(0.0, 0.005, abs(uv.x - u_AmplitudeIndex/512.0))) * vec3(1.0,0.0,0.0);
	
	// output final color
	FragColor = vec4(col,1.0);
}
 