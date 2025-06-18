" struct VertexOutput {
        @builtin(position) position : vec4<f32>,
        @location(0) v_uv : vec2<f32>,
    };

    @group(0) @binding(0) var<uniform> u_time: f32;

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
        let t = u_time;
        // return vec4f(in.v_uv, .5 * sin(u_time) + .5, 1.0);
        // return vec4f(in.v_uv, 0, 1.0);
        let col = vec3f(.5, .5, .5);
        return vec4f(col, 1.0);
    }
" => string screen_shader;
GG.rootPass() --> ScreenPass screen_pass;
// screen_pass.clear(false);
ShaderDesc desc;
screen_shader => desc.vertexCode;
screen_shader => desc.fragmentCode;
null => desc.vertexLayout;

Shader shader(desc);
shader.name("screen shader");
screen_pass.shader(shader);

while (1) {
    screen_pass.material().uniformFloat(0, now/second);
    GG.nextFrame() => now;
}