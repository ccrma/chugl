#include FRAME_UNIFORMS
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

@group(1) @binding(0) var u_texture: texture_2d<f32>;
@group(1) @binding(1) var u_sampler: sampler;
@group(1) @binding(2) var<uniform> u_world_height: f32; // used to go from uv space to world space

// x,y: pos
// z: radius
// w: intensity
@group(1) @binding(3) var<storage, read> u_lights: array<vec4f>;


struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) v_uv : vec2<f32>,
};


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
    let t0 = u_frame;
    let aspect = f32(u_frame.resolution.x) / f32(u_frame.resolution.y);
    var uv = in.position.xy / vec2f(u_frame.resolution.xy); // interesting. fragCoord doesn't change based on viewport
    // uv.y = 1.0 - uv.y;

    let screen_size_world_space = vec2f(u_world_height * aspect, u_world_height);
    let frag_pos_world = (uv - vec2(0.5, 0.5)) * screen_size_world_space;

    let game_pixel = textureSample(u_texture, u_sampler, uv);

    let num_lights: u32 = arrayLength(&u_lights);
    var mask = 0.0;
    for (var i: u32 = 0; i < num_lights; i++) {
        if (distance(u_lights[i].xy, frag_pos_world) < u_lights[i].z) {
            mask = 1.0;
        }
    }

    return mask * game_pixel;
    // return vec4f(frag_pos_world, 0.0, 1.0);
}
