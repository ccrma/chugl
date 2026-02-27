//--------------------------------------------------------------------
// name: skybox-shader.ck
// desc: using a custom skybox shader for atmospheric sky rendering
// requires: ChuGL 0.2.10 (alpha) + chuck-1.5.5.7 or higher
//
// Shader code adapted from: https://github.com/shff/opengl_sky/
// 
// To learn more about sky rendering, also check out:
// https://github.com/wwwtyro/glsl-atmosphere/
// https://fabiensanglard.net/sunset/
// https://jannikboysen.de/reaching-for-the-stars/
// 
// author: Andrew Zhu Aday
//   date: Winter 2026
//--------------------------------------------------------------------

// orbit camera (mouse controlled)
GOrbitCamera cam => GG.scene().camera;

// custom atmospheric sky shader
"
#include FRAME_UNIFORMS
#include ENVIRONMENT_MAP_UNIFORMS
// will include:
// @group(0) @binding(2) var u_envmap: texture_cube<f32>;

@group(1) @binding(0) var<uniform> uSunPos: vec3f;
@group(1) @binding(1) var<uniform> Br: f32; // Rayleigh coefficient
@group(1) @binding(2) var<uniform> Bm: f32; // Mie coefficient
@group(1) @binding(3) var<uniform> g: f32;  // Mie scattering direction. Should be ALMOST 1.0f
@group(1) @binding(4) var<uniform> floor_refraction_ratio: f32;
@group(1) @binding(5) var<uniform> floor_color: vec3f;
@group(1) @binding(6) var<uniform> nitrogen: vec3f;

fn atmosphere(r: vec3f, sun_dir: vec3f) -> vec3f {
    // constants
    let Kr = Br / pow(nitrogen, vec3f(4.0));
    let Km = Bm / pow(nitrogen, vec3f(0.84));

    // Calculate the Rayleigh and Mie phases.
    var mu: f32 = dot(r, sun_dir);
    var rayleigh: f32 = 3.0 / (8.0 * 3.14) * (1.0 + mu * mu);
    var mie: vec3f = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm);

    var day_extinction: vec3f = exp(-exp(-((r.y + sun_dir.y * 4.0) * (exp(-r.y * 16.0) + 0.1) / 80.0) / Br) * (exp(-r.y * 16.0) + 0.1) * Kr / Br) * exp(-r.y * exp(-r.y * 8.0 ) * 4.0) * exp(-r.y * 2.0) * 4.0;
    var night_extinction: vec3f = vec3(1.0 - exp(sun_dir.y)) * 0.2;
    var extinction: vec3f = mix(day_extinction, night_extinction, -sun_dir.y * 0.2 + 0.5);
    return rayleigh * mie * extinction;
}

struct VSOutput {
    @builtin(position) position: vec4f,
    @location(0) r: vec4f,
};

// full-screen triangle
var<private> r : array<vec2f, 3> = array(
    vec2f(-1, 3),
    vec2f(-1,-1),
    vec2f( 3,-1),
);

@vertex 
fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VSOutput {
    var output : VSOutput;
    output.position = vec4f(r[vertexIndex], 1, 1);
    output.r = output.position;

    return output;
}

@fragment
fn fs_main(vsOut: VSOutput) -> @location(0) vec4f {
    let t = u_frame.projection_view_inverse_no_translation * vsOut.r;
    var r = (t.xyz / t.w) * vec3f(1, 1, -1); // skybox dir

    var reflected: bool = false;
    if (r.y < 0) { 
        r.y *= -1.0;
        reflected = true;
        r = refract(r, vec3f(0, -1, 0), floor_refraction_ratio);
    }

    var color: vec3f = atmosphere(
        normalize(r), // normalized ray direction
        normalize(uSunPos),                
    );

    // color the plane reflection
    if (reflected) { color *= floor_color; }

    // Apply exposure and gamma correct
    color = pow(1.0 - exp(-1.3 * color), vec3(1.3));

    return vec4f(color, 1);
}

" => string shader_string;

ShaderDesc shader_desc;
null => shader_desc.vertexLayout; // this shader does not expect vertex buffers
shader_string => shader_desc.vertexCode;
shader_string => shader_desc.fragmentCode;

// create shader from shader_desc
Shader skybox_shader(shader_desc); 

// create a material that uses this shader
Material skybox_material(skybox_shader);

// tell gscene to use our custom skybox shader
GG.scene().skybox(skybox_material);

// disable tonemapping and gamma correction, messes with skybox colors
GG.outputPass().tonemap(OutputPass.TONEMAP_NONE);
GG.outputPass().gamma(false);

// simulation params
UI_Float Br(0.0025); // Rayleigh coefficient
UI_Float Bm(0.0003); // Mie coefficient
UI_Float g(0.9800); // Mie scattering direction. Should be ALMOST 1.0f
[0.0, .1, 1] @=> float sun_dir[];
UI_Float ground_reflection_ratio(.5);
UI_Float3 ground_color(Color.GRAY);
UI_Float3 nitrogen(0.650, 0.570, 0.475); // nitrogen color
UI_Bool automate_sun(true);

float t;
while (true) {
    if (automate_sun.val()) {
        GG.dt() * .05 +=> t;
        Math.sin(t) => sun_dir[1];
        Math.cos(t) => sun_dir[2];
    }

    // apply shader uniforms
    skybox_material.uniformFloat3(0, @(sun_dir[0], sun_dir[1], sun_dir[2]));
    skybox_material.uniformFloat(1, Br.val());
    skybox_material.uniformFloat(2, Bm.val());
    skybox_material.uniformFloat(3, g.val());
    skybox_material.uniformFloat(4, ground_reflection_ratio.val());
    skybox_material.uniformFloat3(5, ground_color.val());
    skybox_material.uniformFloat3(6, nitrogen.val());

    GG.nextFrame() => now;

    // UI params
    UI.checkbox("Automate sun", automate_sun);
    UI.slider("Rayleigh coefficient", Br, 0, .005, "%.4f", 0);
    UI.slider("Mie coefficient", Bm, 0, .01, "%.4f", 0);
    UI.slider("Mie scattering", g, .9, 1, "%.4f", 0);
    UI.slider("Sun dir", sun_dir, -1, 1);
    UI.slider("Ground Reflection Ratio", ground_reflection_ratio, 0, 1);
    UI.colorEdit("Ground color", ground_color);
    UI.colorEdit("Nitrogen color", nitrogen);
}