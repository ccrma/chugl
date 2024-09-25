#include FRAME_UNIFORMS
#include DRAW_UNIFORMS
#include STANDARD_VERTEX_INPUT

/*
Ideas 
- compute heightmap-adjusted normals in vertex shader, apply lighting and texture in fragment shader
*/

struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) v_height : f32,
};

// our custom material uniforms
// @group(1) @binding(0) var u_sampler : sampler;
@group(1) @binding(0) var u_height_map : texture_2d<f32>;
@group(1) @binding(1) var<uniform> u_playhead : i32;
@group(1) @binding(2) var<uniform> u_color : vec3f;

@vertex 
fn vs_main(in : VertexInput) -> VertexOutput
{
    var out : VertexOutput;
    let u_draw : DrawUniforms = u_draw_instances[in.instance];

    let heightmap_dim = textureDimensions(u_height_map);

    var mirrored_uv = in.uv;
    // mirrored_uv.x = fract(in.uv.x * 2.0);
    // mirrored_uv.x = abs(mirrored_uv.x * 2.0 - 1.0);
    var sample_coords = vec2i(mirrored_uv * vec2f(heightmap_dim));
    sample_coords.y = u_playhead - sample_coords.y; // scroll the heightmap
    if (sample_coords.y < 0) {
        sample_coords.y += i32(heightmap_dim.y);
    }
    if (sample_coords.y >= i32(heightmap_dim.y)) {
        sample_coords.y = sample_coords.y % i32(heightmap_dim.y);
    }

    // mirror the uv.x

    let heightmap = textureLoad(u_height_map, sample_coords, 0).r;
    let heightmap_scaled_pos = in.position + (heightmap * in.normal);
    let worldpos = u_draw.model * vec4f(heightmap_scaled_pos, 1.0f);

    out.v_height = heightmap;

    // let worldpos = u_draw.model * vec4f(in.position, 1.0f);

    out.position = (u_frame.projection * u_frame.view) * worldpos;
    // out.v_worldpos = worldpos.xyz;

    return out;
}

// don't actually need normals/tangents
@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    let color_scale = pow((in.v_height / 8.0), .5) + .05;

    let alpha = clamp(color_scale, 0.0, 1.0);
    return vec4f(vec3f(u_color * color_scale), alpha);
}