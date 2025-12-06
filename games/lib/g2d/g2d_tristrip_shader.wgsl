#include FRAME_UNIFORMS
// struct FrameUniforms {
//     projection: mat4x4f,
//     view: mat4x4f,
//     projection_view_inverse_no_translation: mat4x4f,
//     camera_pos: vec3f,
//     time: f32,
//     ambient_light: vec3f,
//     num_lights: i32,
//     background_color: vec4f,
// };
// @group(0) @binding(0) var<uniform> u_frame: FrameUniforms;

#include DRAW_UNIFORMS
// struct DrawUniforms {
//     model: mat4x4f,
//     id: u32
// };
// @group(2) @binding(0) var<storage> u_draw_instances: array<DrawUniforms>;

struct VertexInput {
    @location(0) position : vec3f,
    @location(1) color : vec4f,     // per-vertex color
};


struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) v_color: vec4f,
};

@vertex
fn vs_main(
    in : VertexInput,
    @builtin(instance_index) instance_idx : u32,    // carry over from everything being indexed...
) -> VertexOutput {
    var out : VertexOutput;

    let p = in.position;
    
    var u_Draw : DrawUniforms = u_draw_instances[instance_idx];
    out.position = (u_frame.projection * u_frame.view) * u_Draw.model * vec4f(p, 1.0f);
    out.v_color = in.color;

    return out;
}

// begin fragment shader ----------------------------

@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    return in.v_color;
    // return vec4f(1, 1, 0, .1);
    // return vec4f(1.0);
}