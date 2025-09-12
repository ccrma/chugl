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
@group(1) @binding(2) var<uniform> viewport_height_pixels: i32;
@group(1) @binding(3) var<uniform> center_pixels: vec2i;
@group(1) @binding(4) var<uniform> u_grid: i32;
@group(1) @binding(5) var<uniform> u_brush_color: vec4f;

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
    var uv = in.position.xy / vec2f(u_frame.resolution.xy); // interesting. fragCoord doesn't change based on viewport
    // uv.y = 1.0 - uv.y;
    uv -= 0.5;

    let tex_dim = textureDimensions(u_texture).xy;
    let aspect = f32(u_frame.resolution.x) / f32(u_frame.resolution.y);

    var flipped_mouse = u_frame.mouse; 
    flipped_mouse.y = 1.0 - flipped_mouse.y;
    let mouse = ((flipped_mouse - 0.5) * vec2f(aspect, 1.0) * f32(viewport_height_pixels) + vec2f(center_pixels));

    let pixel_fx = (uv.x * aspect * f32(viewport_height_pixels)) + f32(center_pixels.x);
    let pixel_fy = (uv.y * f32(viewport_height_pixels)) + f32(center_pixels.y);
    let pixel_f = vec2(pixel_fx, pixel_fy);

    let pixel_x = i32(pixel_fx);
    let pixel_y = i32(pixel_fy);

    let pixel_u = pixel_fx / (f32(tex_dim.x));
    let pixel_v = pixel_fy / (f32(tex_dim.y));
    let pixel_uv = vec2f(pixel_u, pixel_v);


    var col : vec4f;

    if (all(vec2i(mouse) == vec2i(pixel_x, pixel_y))) {
    // if (abs(pixel_fx - mouse.x) < 0.5 && abs(pixel_fy - mouse.y) < 0.5) {
        col = u_brush_color;
    } else {
        if (any(pixel_uv > vec2f(1.0)) || any(pixel_uv < vec2f(0.0))) {
            col = vec4f(0.0);
        } else {
            col = textureSample(u_texture, u_sampler, vec2(pixel_u, pixel_v));
        }
    }

    let pixel_width_cell_space = f32(viewport_height_pixels) / f32(u_frame.resolution.y);
    var grid : f32 = 1.0;
    if (bool(u_grid) && any(fract(pixel_f) < vec2f(pixel_width_cell_space))) {
        grid = 0.0;
    } else {
        grid = 1.0;
    }

    return col * grid;
}
