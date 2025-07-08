
/*
Problems:

- shader compilation error prints twice
- shader compilation error does not show name of SG_Component (because we set shaderDesc in constructor, before setting .name)

[ChuGL]: ERRORwgpu log [1]: Device::create_shader_module error:
Shader 'vertex shader 30' parsing error: expected assignment or increment/decrement, found 'return'
   ┌─ wgsl:74:9
   │
74 │         return vec4f(uv, 0, 1.0);
   │         ^^^^^^ expected assignment or increment/decrement

- very unclear error message when running a shader but not having set all the appropriate fields on the material
Caused by:
  In wgpuDeviceCreateBindGroup, label = ' @group(0)'
    Number of bindings in bind group descriptor (0) does not match the number of bindings defined in the bind group layout (14)
)

- empty compute shader (no uniforms at all) crashes with:
[ChuGL]: 10:00:09 ERROR /Users/Andrew/Google-Drive/Stanford/chugl/src/./graphics.cpp:169:Uncaptured device error: type 1 (Validation Error
Caused by:
  In wgpuComputePipelineGetBindGroupLayout
    Invalid group index 0

- StorageBuffer class is ass. need to rework. think of some cool examples that will force it to grow

- binding empty storage buffer to ccompute shader throws a crash "invalid size" during bindgroup creation
    - guessing you cannot create a bindgroup from a bindgroup entry with size 0
    - solution: initialize storageBuffer ctor to have min size
      In wgpuDeviceCreateBindGroup, label = ' @group(0)'
    Binding size 4 of Buffer with '' label is less than minimum 16
    - min is 16 bytes = 4 floats

- setting computePath AND computeCode on ShaderDesc causes it to be intepreted as a fragment shader??????



========== FEATURES ===============
- Add UV zoom on screen shader

*/

200000 => int NUM_SLIMES;
1920 => int RESOLUTION_X;
1080 => int RESOLUTION_Y;


ShaderDesc compute_shader_desc;
me.dir() + "./slime.compute" => compute_shader_desc.computePath;
Shader agent_shader(compute_shader_desc);

ShaderDesc trail_shader_desc;
"
@group(0) @binding(0) var trail_texture_read: texture_2d<f32>;
@group(0) @binding(1) var trail_texture_write: texture_storage_2d<rgba8unorm, write>;

@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) id : vec3u) {
    let dim = textureDimensions(trail_texture_read);
    if (id.x >= dim.x || id.y >= dim.y) {
        return;
    }

    let original_value = textureLoad(trail_texture_read, id.xy, 0);

    // diffuse
    var sum = vec4f(0.0);
    for (var dx = -1; dx <= 1; dx++) {
        for (var dy = -1; dy <= 1; dy++) {
            var sample = vec2i(id.xy) + vec2i(dx, dy);
            if (sample.x >= 0 && sample.x < i32(dim.x) && sample.y >= 0 && sample.y < i32(dim.y)) {
                sum += textureLoad(trail_texture_read, sample, 0);
            }
        }
    }
    var diffuse = sum / 9.0;

    // TODO add diffusion speed + lerp

    // dissolve 
    let dissolve_factor = .9; // TODO scale by dt
    var dissolved_value = diffuse * dissolve_factor;

    // dump to texture
    textureStore(trail_texture_write, id.xy, dissolved_value);
    // textureStore(trail_texture_write, id.xy, original_value);
}
" => trail_shader_desc.computeCode;
Shader trail_shader(trail_shader_desc);


" 
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

    @group(1) @binding(0) var src: texture_2d<f32>;
    @group(1) @binding(1) var texture_sampler: sampler;

    struct VertexOutput {
        @builtin(position) position : vec4<f32>,
        @location(0) v_uv : vec2<f32>,
    };

    fn rand(co: f32) -> f32 { return fract(sin(co*(91.3458)) * 47453.5453); }
    fn rand2(co: vec2f) -> f32 { return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }
    fn rand3(co: vec3f) -> f32 { return rand2(co.xy+rand(co.z)); }

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
        uv.y = 1.0 - uv.y;

        // no sampler for now
        let t = textureSample(src, texture_sampler, in.v_uv);
        let col = textureLoad(src, vec2u(in.v_uv * vec2f(textureDimensions(src))), 0).rgb;
        return vec4f(
                // col,
                // in.v_uv, 0.0,
                textureSampleLevel(src, texture_sampler, in.v_uv, 0.0).rgb,
                1.0
        );
        // return vec4f(rand2(in.v_uv));
    }
" => string screen_shader;

// TODO: improve shader construction, maybe with static factory methods
ShaderDesc desc;
screen_shader => desc.vertexCode => desc.fragmentCode;
null => desc.vertexLayout;

Shader shader(desc);
shader.name("screen shader");


float slime_init[4 * NUM_SLIMES];
for (int i; i < NUM_SLIMES; i++) {
    // set position (xy)
    .5 => slime_init[4*i + 0];
    .5 => slime_init[4*i + 1];
    // set heading (radians)
    // (i * 1.0 / NUM_SLIMES) * Math.two_pi => slime_init[4*i + 2];
    Math.random2f(0, Math.two_pi) => slime_init[4*i + 2];

    // padding (Agents struct on GPU takes up 16 bytes)
    0 => slime_init[4*i + 3];
}

// render graph
GG.rootPass() --> ComputePass compute_pass(agent_shader) --> ComputePass trail_pass(trail_shader) --> ScreenPass screen_pass(shader);

StorageBuffer slime_buffer;
slime_buffer.size(4 * NUM_SLIMES);
slime_buffer.write(slime_init);

TextureDesc trail_tex_desc;
RESOLUTION_X => trail_tex_desc.width;
RESOLUTION_Y => trail_tex_desc.height;
false => trail_tex_desc.mips;

Texture trail_tex_a(trail_tex_desc); 
Texture trail_tex_b(trail_tex_desc); 

compute_pass.storageBuffer(0, slime_buffer);
compute_pass.texture(1, trail_tex_a); // read
compute_pass.storageTexture(2, trail_tex_b); // write
compute_pass.uniformFloat(3, GG.dt());

trail_pass.texture(0, trail_tex_b); // read
trail_pass.storageTexture(1, trail_tex_a); // write

screen_pass.material().texture(0, trail_tex_a);
screen_pass.material().sampler(1, TextureSampler.linear());

compute_pass.workgroup((NUM_SLIMES / 64) + 1, 1, 1);
trail_pass.workgroup((RESOLUTION_X / 8) + 1, (RESOLUTION_Y / 8) + 1, 1);

while (1) {
    GG.nextFrame() => now;

    compute_pass.texture(1, trail_tex_b);
    compute_pass.storageTexture(2, trail_tex_a);
    compute_pass.uniformFloat(3, GG.dt());

    trail_pass.texture(0, trail_tex_a);
    trail_pass.storageTexture(1, trail_tex_b);

    screen_pass.material().texture(0, trail_tex_b);

    GG.nextFrame() => now; // swap buffers and textures -------------

    compute_pass.texture(1, trail_tex_a);
    compute_pass.storageTexture(2, trail_tex_b);
    compute_pass.uniformFloat(3, GG.dt());

    trail_pass.texture(0, trail_tex_b);
    trail_pass.storageTexture(1, trail_tex_a);

    screen_pass.material().texture(0, trail_tex_a);

}