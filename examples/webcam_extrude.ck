// Video video(me.dir() + "./bjork-all-is-full-of-love.mpg") => dac; 

GOrbitCamera camera --> GG.scene();
camera.clip(.01, 1000);
GG.scene().camera(camera);

// audio graph
adc => Gain g => OnePole p => blackhole;
// square the input
adc => g;
// multiply
3 => g.op;

// set filter pole position (between 0 and 1)
// NOTE: this controls how smooth the output is
// closer to 1 == smoother but less responsive
// closer to 0 == more jumpy but also more responsive
0.995 => p.pole;

"
#include FRAME_UNIFORMS
#include DRAW_UNIFORMS
#include STANDARD_VERTEX_INPUT

struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) v_height : f32,
    @location(1) v_uv : vec2<f32>,
    @location(2) v_color : vec3<f32>,
};

// our custom material uniforms
@group(1) @binding(0) var u_webcam : texture_2d<f32>;
@group(1) @binding(1) var<uniform> u_height_scale : f32;
// @group(1) @binding(1) var texture_sampler : sampler;

fn grayscale(in : vec4f) -> f32
{
    return dot(in.rgb, vec3f(0.299f, 0.587f, 0.114f));
}

@vertex 
fn vs_main(in : VertexInput) -> VertexOutput
{
    var out : VertexOutput;
    let u_draw : DrawUniforms = u_draw_instances[in.instance];

    let webcam_dim = textureDimensions(u_webcam);

    var sample_coords = vec2i(in.uv * vec2f(webcam_dim));
    let webcam_color = textureLoad(u_webcam, sample_coords, 0);
    let heightmap = grayscale(webcam_color);
    let heightmap_scaled_pos = in.position + u_height_scale * pow(heightmap, 2.0) * in.normal;
    let worldpos = u_draw.model * vec4f(heightmap_scaled_pos, 1.0f);

    out.v_height = heightmap;
    out.v_uv = in.uv;
    out.v_color = webcam_color.rgb;
    out.position = (u_frame.projection * u_frame.view) * worldpos;

    return out;
}

// don't actually need normals/tangents
@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    // let color_scale = pow((in.v_height / 8.0), 1.5) + .05;
    let color_scale = in.v_height;
    let alpha = clamp(color_scale, 0.0, 1.0);

    // return vec4f(vec3f(color_scale), 1.0);
    // return textureSample(u_webcam, texture_sampler, in.v_uv);
    // var tangentNormal : vec3f = textureSample(normalMap, texture_sampler, inUV).rgb * 2.0 - 1.0;
    return vec4f(in.v_color, alpha);
    // return vec4f(in.v_height);
}
" @=> string webcam_shader_string;

ShaderDesc shader_desc;
webcam_shader_string => shader_desc.vertexCode;
webcam_shader_string => shader_desc.fragmentCode;
Shader webcam_shader(shader_desc); // create shader from shader_desc


Webcam webcam;
<<< "webcam width: ", webcam.width() >>>;
<<< "webcam height: ", webcam.height() >>>;
<<< "webcam fps: ", webcam.fps() >>>;

Material webcam_material;
webcam_shader => webcam_material.shader;
webcam_material.texture(0, webcam.texture()); // bind webcam texture to slot 0
// webcam_material.texture(0, video.texture()); // bind webcam texture to slot 0
webcam_material.uniformFloat(1, 1); // bind webcam texture to slot 0
// webcam_material.sampler(1, new TextureSampler); // create a default sampler for slot 1

// create plane geo with as many subdivisions as the webcam texture has pixels
PlaneGeometry plane_geo(
    10,  // width
    10,  // height
    webcam.width() / 1,
    webcam.height() / 1
);
// SphereGeometry plane_geo(2, webcam.width() / 4, webcam.height() / 4, 0, Math.TWO_PI, 0, Math.PI);
// TorusGeometry plane_geo(
//     2,
//     1,
//     webcam.width() / 2, // width segments
//     webcam.height() / 2, // height segments
//     Math.TWO_PI
// );
webcam_material.topology(Material.Topology_LineStrip);

GMesh plane(plane_geo, webcam_material) --> GG.scene();

// plane.scaX(-3 * webcam.aspect());
plane.scaX(-3);
plane.scaY(-3); // flipping to match webcam orientation
plane.scaZ(3);

UI_Float height_scale(1.0);

while (true) {
    GG.nextFrame() => now;
    // GG.dt() => plane.rotateY;

    height_scale.val(webcam_material.uniformFloat(1));

    if (UI.begin("Webcam")) {
        if (UI.slider("Height Scale", height_scale, 0.0, 10.0)) {
            webcam_material.uniformFloat(1, height_scale.val());
        }
        UI.end();
    }

    webcam_material.uniformFloat(1, 5 * Math.pow(p.last(), .2));
}