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


// VERTEX SHADER DON'T CHANGE
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
    // unused variables get excluded from shader compilation which messes up
    // bindgroup structure...annoying. You'll need to assign like this 
    // for all your unused shader uniforms so it compiles correctly
    let t0 = u_frame; 

    // get uv
    var uv = in.position.xy / vec2f(u_frame.resolution.xy);
    uv.y = 1.0 - uv.y;

    /*
    YOUR CODE GOES HERE
    */

    return vec4f(fract(2. * in.v_uv), u_color, 1.0);
}