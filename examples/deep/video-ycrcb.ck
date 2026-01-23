//--------------------------------------------------------------------
// name: video-ycrcb.ck
// desc: custom shader for converting decoded video planes from YCrCb 
//       to rgba 
// requires: ChuGL 0.2.10 (alpha)
// 
// (DATA) download and place this music video in the same directory: 
//   https://chuck.stanford.edu/chugl/examples/data/video/bjork.mpg
//
// find more mpeg samples here:
//   https://filesamples.com/formats/mpeg
//
// convert an mp4 to mpg with the following terminal command:
// ffmpeg -i input.mp4 -c:v mpeg1video -q:v 0 -c:a libtwolame -b:a 224k -format mpeg output.mpg
//
// author: Andrew Zhu Aday
//   date: Winter 2026
//--------------------------------------------------------------------

// Video is a UGen
Video video( me.dir() + "./bjork.mpg" ) => dac; 
video.seek(30::second);

class YCrCbMaterial extends Material {
    "
    #include FRAME_UNIFORMS
    #include DRAW_UNIFORMS
    #include STANDARD_VERTEX_INPUT

    struct VertexOutput {
        @builtin(position) position : vec4f,
        @location(1) v_uv : vec2f,
    };

    @group(1) @binding(0) var texture_sampler: sampler;
    @group(1) @binding(1) var texture_y: texture_2d<f32>;   
    @group(1) @binding(2) var texture_cr: texture_2d<f32>;   
    @group(1) @binding(3) var texture_cb: texture_2d<f32>;   
    @group(1) @binding(4) var<uniform> crop_uv: vec2f;


    @vertex 
    fn vs_main(in : VertexInput) -> VertexOutput
    {
        var out : VertexOutput;
        var u_Draw : DrawUniforms = u_draw_instances[in.instance];

        let worldpos = u_Draw.model * vec4f(in.position, 1.0f);
        out.position = (u_frame.projection * u_frame.view) * worldpos;
        out.v_uv     = in.uv;

        // crop UV because yCrCb planes are always rounded to a multiple of 16
        out.v_uv *= crop_uv;

        return out;
    }

    // YCrCb --> srgb conversion matrix
    const rec601 = mat4x4f(
        1.16438,  0.00000,  1.59603, -0.87079,
        1.16438, -0.39176, -0.81297,  0.52959,
        1.16438,  2.01723,  0.00000, -1.08139,
        0, 0, 0, 1
    );

    @fragment 
    fn fs_main(in : VertexOutput) -> @location(0) vec4f
    {   
        var y = textureSample(texture_y, texture_sampler, in.v_uv).r;
        var cb = textureSample(texture_cb, texture_sampler, in.v_uv).r;
        var cr = textureSample(texture_cr, texture_sampler, in.v_uv).r;

        let col_srgb = vec4f(y, cb, cr, 1.0) * rec601;

        // after multiplying with the conversion matrix, we are now in gamma/srgb space.
        // convert back to linear space so the final color doesn't look overly bright
        let col_linear = pow(col_srgb, vec4f(2.2));

        return col_linear;
    }
    " => static string shader_code;

    static Shader@ ycrcb_shader;
    if (ycrcb_shader == null) {
        ShaderDesc shader_desc;
        shader_code => shader_desc.vertexCode;
        shader_code => shader_desc.fragmentCode;

        new Shader(shader_desc) @=> ycrcb_shader;
    }

    // set shader
    ycrcb_shader => this.shader;

    fun @construct(Video video) {
        // set uniform defaults
        this.sampler(0, TextureSampler.linear());
        this.texture(1, video.textureY());
        this.texture(2, video.textureCr());
        this.texture(3, video.textureCb());
            // YCrCb dimensions always a multiple of 16, so set a crop window to remove empty pixels
        this.uniformFloat2(
            4, 
            @(
                video.width() $ float / video.textureY().width(),
                video.height() $ float / video.textureY().height()
            )
        ); 
    }
}

// material for rendering YCrCb planes
YCrCbMaterial ycrcb_material(video);
// material for simpler (but slower) rgba texture
FlatMaterial rgba_material;
rgba_material.colorMap(video.texture());

GMesh mesh(new PlaneGeometry, rgba_material) --> GG.scene();

// set mesh scaling
(video.width() $ float) / video.height() => float video_aspect;
mesh.scaX(3 * video_aspect);
mesh.scaY(-3);

[
    "rgba",
    "YCrCb"
] @=> string texture_modes[];
UI_Int texture_mode_idx(video.mode());

while (true)
{
    GG.nextFrame() => now;
    if (UI.listBox("video texture mode", texture_mode_idx, texture_modes)) {
        video.mode(texture_mode_idx.val());
        if (video.mode() == Video.MODE_RGBA) rgba_material => mesh.mat;
        else                               ycrcb_material => mesh.mat;
    }
}
