#version 330 core

// interpolated varyings from vertex shader
in vec3 v_Pos;
in vec2 v_TexCoord;

// output color
out vec4 FragColor;

// uniforms (these are set by chuck!)
uniform float u_Time;
uniform sampler2D u_AudioTexture; // texture that contains waveform, fft, and amplitude audio data
uniform int u_AmplitudeIndex;  // where we are currently writing to in amp history graph

// Modified from a shader by Inigo Quilez
// reference: https://www.shadertoy.com/view/Xds3Rr

void main()
{
    // shorthand for texture coordinates
    vec2 uv = v_TexCoord; 

    // the sound texture is 512x1, so we can use the uv x-coordinate as a lookup index
    int tx = int(uv.x*512.0);
    
	// first chan is frequency data (48Khz/4 in 512 texels, meaning 23 Hz per texel)
    // note: we don't use glsl texture() function for this, because texture() 
    // applies filtering and takes in normalized lookup coordinates in [0,1]
    // 
    // texelFetch() is meant for accessing raw data, and performs lookup with 
    // unnormalized coordinates
	float fft  = texelFetch( u_AudioTexture, ivec2(tx,0), 0 ).x; 

    // second channel is the sound wave, one texel is one mono sample
    float wave = texelFetch( u_AudioTexture, ivec2(tx,0), 0 ).y;

    // third channel is the amplitude history
    float amp = texelFetch( u_AudioTexture, ivec2(tx,0), 0 ).z;
	
	// convert frequency to colors
	vec3 col = vec3( fft, 4.0*fft*(1.0-fft), 1.0-fft ) * fft * step(.5, uv.y);

    // add wave form on top	
	col += 1.0 -  smoothstep( 0.0, 0.005, abs(wave - uv.y) );

    // add amplitude bargraph
    col += step(uv.y, 0.5) * step(uv.y, amp) * vec3(1.0);

    // add redline tracking where most recent amplitude RMS values are being added
    col += step(uv.y, 0.5) * (1.0 - smoothstep(0.0, 0.005, abs(uv.x - u_AmplitudeIndex/512.0))) * vec3(1.0,0.0,0.0);
	
	// output final color
	FragColor = vec4(col,1.0);
}
 