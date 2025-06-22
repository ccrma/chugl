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
        uv.y = 1.0 - uv.y;

        // return vec4f(u_frame.mouse.y);
        // return vec4f(uv, .5 * sin(u_time) + .5, 1.0);
        // return vec4f(fract(2. * uv), 0, 1.0);
        return vec4f(fract(2. * in.v_uv), 0, 1.0);
        // let col = vec3f(.5, .5, .5);
        // return vec4f(col, 1.0);
    }
" => string screen_shader;
// screen_pass.clear(false);
ShaderDesc desc;
screen_shader => desc.vertexCode;
screen_shader => desc.fragmentCode;
null => desc.vertexLayout;

Shader shader(desc);
shader.name("screen shader");

GG.rootPass() --> ScreenPass screen_pass(shader);

UI_Float4 viewport_normalized(0., 0., 1., 1.);
UI_Float4 viewport_absolute(0., 0., 200., 200.);
UI_Float4 scissor_normalized(0., 0., 1., 1.);
UI_Float4 scissor_absolute(0., 0., 200., 200.);

while (1) {
    GG.nextFrame() => now;
    if (UI.begin("")) {
        if (UI.drag("viewport (normalized)", viewport_normalized, .01)) {
            screen_pass.viewportNormalized(viewport_normalized.val());
        }
        if (UI.drag("viewport (in pixels)", viewport_absolute)) {
            screen_pass.viewport(viewport_absolute.val());
        }

        if (UI.drag("scissor (normalized)", scissor_normalized, .01)) {
            screen_pass.scissorNormalized(scissor_normalized.val());
        }
        if (UI.drag("scissor (in pixels)", scissor_absolute)) {
            screen_pass.scissor(scissor_absolute.val());
        }
    }
    UI.end();
}