/*
Test GC of G_Cache GPU resources (renderpipeline, bg layout) when freeing shaders 
*/

Material custom_material;

"
#include FRAME_UNIFORMS
#include DRAW_UNIFORMS
#include STANDARD_VERTEX_INPUT

// our custom material uniforms
@group(1) @binding(0) var<uniform> u_color: vec3f;

struct VertexOutput {
    @builtin(position) position : vec4f,
};


@vertex 
fn vs_main(in : VertexInput) -> VertexOutput
{
    var out : VertexOutput;
    var u_Draw : DrawUniforms = u_draw_instances[in.instance];
    let worldpos = u_Draw.model * vec4f(in.position, 1.0f);
    out.position = (u_frame.projection * u_frame.view) * worldpos;

    return out;
}

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    return vec4f(u_color, 1.0);
}" => string shader_string;

ShaderDesc shader_desc;
shader_string => shader_desc.vertexCode;
shader_string => shader_desc.fragmentCode;

Shader custom_shader(shader_desc); // create shader from shader_desc
custom_shader => custom_material.shader; // connect shader to material
GMesh mesh(new PlaneGeometry, custom_material) --> GG.scene();

while (1) {
    now/second => float t;
    // test changing the shader and material every frame
    Material mat(new Shader(shader_desc));
    mesh.material(mat);

    mesh.material().uniformFloat3(0, @(Math.sin(t*1.13), Math.cos(t*1.07), Math.sin(t*.9)));


    GG.nextFrame() => now;
}