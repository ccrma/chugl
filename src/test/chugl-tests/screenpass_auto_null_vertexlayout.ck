"
#include FRAME_UNIFORMS

struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) v_uv : vec2<f32>,
};


// VERTEX SHADER DON'T CHANGE! This draws the full-screen quad
@vertex
fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
    var output : VertexOutput;
    let UNUSED = u_frame;

    // a triangle which covers the screen
    output.v_uv = vec2f(f32((vertexIndex << 1u) & 2u), f32(vertexIndex & 2u));
    output.position = vec4f(output.v_uv * 2.0 - 1.0, 0.0, 1.0);

    // webgpu has flipped-y
    output.v_uv.y = 1 - output.v_uv.y;

    return output;
}

@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    return vec4f(1.0);
}
" => string screen_shader;

ShaderDesc shader_desc;
screen_shader => shader_desc.vertexCode;
screen_shader => shader_desc.fragmentCode;

// annoying, but need to do this for now until
// ChuGL 0.3.1 release
// null => shader_desc.vertexLayout; 

GG.rootPass() --> GG.scenePass() --> ScreenPass screen_pass(new Shader(shader_desc));

screen_pass.material().shader().vertexLayout() @=> int layout[];
for (auto i : layout) T.assert(i == 0);

repeat(3) GG.nextFrame() => now;