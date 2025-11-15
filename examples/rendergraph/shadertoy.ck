//--------------------------------------------------------------------
// name: shadertoy.ck
// desc: boilerplate for creating a full-screen screenpass shader
// requires: ChuGL 0.2.8 (alpha) + chuck-1.5.5.6 or higher
// 
// author: Andrew Zhu Aday
//   date: Fall 2025
//--------------------------------------------------------------------

"
#include FRAME_UNIFORMS
// what's included:
// struct FrameUniforms {
//     // scene params (only set in ScenePass, otherwise 0)
//     projection: mat4x4f,
//     view: mat4x4f,
//     projection_view_inverse_no_translation: mat4x4f,
//     camera_pos: vec3f,
//     ambient_light: vec3f,
//     num_lights: i32,
//     background_color: vec4f,

//     // general params (include in all passes except ComputePass)
//     resolution: vec3i,      // window viewport resolution 
//     time: f32,              // time in seconds since the graphics window was opened
//     delta_time: f32,        // time since last frame (in seconds)
//     frame_count: i32,       // frames since window was opened
//     mouse: vec2f,           // normalized mouse coords (range 0-1)
//     mouse_click: vec2i,     // mouse click state
//     sample_rate: f32        // chuck VM sound sample rate (e.g. 44100)
// };
// @group(0) @binding(0) var<uniform> u_frame: FrameUniforms;

struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) v_uv : vec2<f32>,
};

// SHADER UNIFORMS (YOUR CODE GOES HERE)
@group(1) @binding(0) var<uniform> u_color: f32;


// VERTEX SHADER DON'T CHANGE! This draws the full-screen quad
@vertex 
fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
    var output : VertexOutput;

    // a triangle which covers the screen
    output.v_uv = vec2f(f32((vertexIndex << 1u) & 2u), f32(vertexIndex & 2u));
    output.position = vec4f(output.v_uv * 2.0 - 1.0, 0.0, 1.0);
    
    return output;
}

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    /*
    NOTE If you see an error such as:
        [ChuGL]: ERROR Uncaptured device error: type 1 (Validation Error
            Caused by:
            In wgpuDeviceCreateBindGroup, label = ' @group(0)'
                Number of bindings in bind group descriptor (1) does not match the number of bindings defined in the bind group layout (0)
            )
    
    This means that one or more uniform variables was unused, and therefore 
    optimized out by the gpu shader compiler.  This causes the layout of the
    shader to misalign with what chugl expects.
    Workaround: simply add a dummy reference to the unused uniform, like so
    */
    let UNUSED = u_frame; 

    // get uv
    var uv = in.position.xy / vec2f(u_frame.resolution.xy);
    uv.y = 1.0 - uv.y;

    /*
    YOUR CODE GOES HERE
    */

    return vec4f(fract(2. * in.v_uv), u_color, 1.0);
}
" => string shader_code;

ShaderDesc desc;
shader_code => desc.vertexCode;
shader_code => desc.fragmentCode;
null => desc.vertexLayout;

Shader shader(desc);
shader.name("screen shader");

// render graph
GG.rootPass() --> ScreenPass screen_pass(shader);

// set uniforms
// (be sure to initialize all uniforms before the first call to GG.nextFrame())
screen_pass.material().uniformFloat(0, 0);

while (1) {
    GG.nextFrame() => now;
    screen_pass.material().uniformFloat(0, .5 + .5 * Math.sin(now/second));
}