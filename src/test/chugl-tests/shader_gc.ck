// ------- graphics stuff -----------
// shader code string (does the GoL simulation)
"
#include FRAME_UNIFORMS
#include DRAW_UNIFORMS
#include STANDARD_VERTEX_INPUT
#include STANDARD_VERTEX_OUTPUT
#include STANDARD_VERTEX_SHADER

@fragment 
fn fs_main(in : VertexOutput, @builtin(front_facing) is_front: bool) -> @location(0) vec4f
{
    return vec4f(1.0);
}" => string shader_string;

null => Shader@ first_shader;

fun void createShader() {
    ShaderDesc shader_desc;
    shader_string => shader_desc.vertexCode;
    shader_string => shader_desc.fragmentCode;
    Shader custom_shader(shader_desc); // create shader from shader_desc
    if (first_shader == null) {
        custom_shader @=> first_shader;
    } 
    <<< "first_shader id", first_shader.id(), "| custom_shader id", custom_shader.id() >>>;
}

while (true) {
    GG.nextFrame() => now;

    // if (GWindow.keyDown(GWindow.Key_Space)) {
    //     <<< "creating shader" >>>;
    //     createShader();
    // }
    createShader();
}
