/*
This test makes sure chugl does not crash when setting an empty array to a storage buffer binding.
Previously, it would crash with "invalid size", because wgsl expected the WGPUBuffer to have 
min size = 1 element of the type.
e.g.  var<storage> array<f32> requires min size(f32) = 4 bytes
      var<storage> array<vec4f> requires min 4 * size(f32) = 16 bytes

because we currently don't have wgsl reflection, we can't know what array type is being bound.
So we default to the largest minimum of 16 bytes...
binding an empty chuck array will now create a storage buffer of min size = 16bytes to satisfy
the largest possible array<vec4f> binding.

see `CQ_PushCommand_MaterialSetStorageBuffer` for the change.

*/

MeshLines line --> GG.scene();
GG.nextFrame() => now; GG.nextFrame() => now;

// implementation
public class MeshLines extends GMesh
{	
    "
    #include FRAME_UNIFORMS
#include DRAW_UNIFORMS

struct VertexOutput {
    @builtin(position) position: vec4f,
};

@group(1) @binding(0) var<storage> a_position : array<f32>;
@group(1) @binding(1) var<storage> a_color : array<vec4f>; 
@group(1) @binding(2) var<storage> a_width : array<vec3f>;

@vertex
fn vs_main(
    @builtin(instance_index) instance_idx : u32,    // carry over from everything being indexed...
    @builtin(vertex_index) vertex_idx : u32,        // used to determine which polygon we are drawing
) -> VertexOutput {
    var out : VertexOutput;
    var u_Draw : DrawUniforms = u_draw_instances[instance_idx];
    let resolution = vec2f(u_frame.resolution.xy);

    let line_num_vertices = i32(arrayLength(&a_position) / 3);
    let line_num_colors = i32(arrayLength(&a_color));
    let line_num_widths = i32(arrayLength(&a_width));

    out.position = vec4f(1.0);
    return out;
}

@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4f { return vec4f(1.0); }
    " => string shader_code;

	// material shader, shared by all MeshLines instances
	static ShaderDesc shader_desc;
	static Shader@ shader;

	static float empty_float_arr[0];
	static float white_float_arr[4];

    // local params
	Material line_material;
	Geometry line_geo; // just used to set vertex count
    int n_positions; // #line vertices

    if (shader == null) {
        shader_code => shader_desc.vertexCode;
        shader_code => shader_desc.fragmentCode;
        null @=> shader_desc.vertexLayout;
        new Shader(shader_desc) @=> shader;
    }

    // init material shader
    line_material.shader(shader);

    // init storage buffers
    line_material.storageBuffer(0, empty_float_arr);
    line_material.storageBuffer(1, empty_float_arr);
    line_material.storageBuffer(2, white_float_arr);
}

