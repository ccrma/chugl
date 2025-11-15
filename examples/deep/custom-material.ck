//--------------------------------------------------------------------
// name: custom-material.ck
// desc: boilerplate for creating a material with custom vertex and 
//       fragment shaders
// requires: ChuGL 0.2.8 (alpha) + chuck-1.5.5.6 or higher
// 
// author: Andrew Zhu Aday
//   date: Fall 2025
//--------------------------------------------------------------------
"
#include FRAME_UNIFORMS
// for now you always need this in your chugl shader
// it includes the following (set by the chugl renderer every frame):
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
    //     mouse: vec2f,           // normalized mouse coords (range 0-1, (0,0) is bottom left)
    //     mouse_click: vec2i,     // mouse click state
    //     sample_rate: f32        // chuck VM sound sample rate (e.g. 44100)
    // };
    // @group(0) @binding(0) var<uniform> u_frame: FrameUniforms;

#include DRAW_UNIFORMS
// includes the following (an array of data for every GMesh that uses this shader)
    // struct DrawUniforms {
    //     model: mat4x4f,
    //     normal: mat4x4f,
    //     id: i32,
    //     receives_shadow: i32,
    // };
    // @group(2) @binding(0) var<storage> u_draw_instances: array<DrawUniforms>;

#include STANDARD_VERTEX_INPUT
// includes the following 
// these vertex attributes are passed by all the builtin chugl geometries
// you can set your own vertex attributes via ShaderDesc.vertexLayout(...)
    // struct VertexInput {
    //     @location(0) position : vec3f,
    //     @location(1) normal : vec3f,
    //     @location(2) uv : vec2f,
    //     @builtin(instance_index) instance : u32,
    // };

struct VertexOutput {
    @builtin(position) position : vec4f,
    @location(0) v_worldpos : vec3f,
    @location(1) v_normal : vec3f,
    @location(2) v_uv : vec2f,
    @location(3) @interpolate(flat) receives_shadow: i32,
};

// YOUR MATERIAL UNIFORMS HERE
@group(1) @binding(0) var<uniform> u_color : vec3f;

@vertex 
fn vs_main(in : VertexInput) -> VertexOutput
{
    /*
    YOUR VERTEX SHADER CODE GOES HERE
    (below is a basic vertex shader that performs 3d projection)
    */

    var out : VertexOutput;
    var u_Draw : DrawUniforms = u_draw_instances[in.instance];

    let worldpos = u_Draw.model * vec4f(in.position, 1.0f);
    out.position = (u_frame.projection * u_frame.view) * worldpos;
    out.v_worldpos = worldpos.xyz;
    out.v_normal = (u_Draw.normal * vec4f(in.normal, 0.0)).xyz;
    out.v_uv     = in.uv;

    return out;
}

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{   
    /*
    YOUR FRAGMENT SHADER CODE HERE

    NOTE If you see an error such as:
        [ChuGL]: ERROR Uncaptured device error: type 1 (Validation Error
            Caused by:
            In wgpuDeviceCreateBindGroup, label = ' @group(0)'
                Number of bindings in bind group descriptor (1) does not match the number of bindings defined in the bind group layout (0)
            )
    This means that the gpu shader compiler optimized out one of your uniform variables
    and the bindgroup layout of this shader no longer aligns with what chugl expects.
    To work around that, simply add a dummy reference to the unused uniform, like so:

        let UNUSED = u_frame;

    */
    return vec4f(u_color * (in.v_normal * 0.5 + .5), 1.0);
}
" => string shader_code;

ShaderDesc shader_desc;
shader_code => shader_desc.vertexCode;
shader_code => shader_desc.fragmentCode;

// create shader from shader_desc
Shader custom_shader(shader_desc); 

// create a material that uses this shader
Material material;
custom_shader => material.shader; // connect shader to material

// remember to initialize all uniforms in your material
material.uniformFloat3(0, Color.WHITE);

SuzanneGeometry geo;
GMesh mesh(geo, material) --> GG.scene();

// render loop
while (true)
{
    GG.nextFrame() => now;
    GG.dt() => mesh.rotateY;

    // change colors randomly
    material.uniformFloat3(0, @(Math.sin(now/second), Math.sin(3 * (now/second)), Math.sin(2.3 * (now/second))));
}
