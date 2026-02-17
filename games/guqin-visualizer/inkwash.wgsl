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


@group(1) @binding(0) var u_sampler: sampler;
@group(1) @binding(1) var simulation_map: texture_2d<f32>;   
@group(1) @binding(2) var color_map: texture_2d<f32>;   
@group(1) @binding(3) var<uniform> u_curr_mouse: vec3f;
@group(1) @binding(4) var<uniform> u_prev_mouse: vec3f;

//Recommended values between 0.03 and 0.2
//higher values simulate lower viscosity fluids (think billowing smoke)
@group(1) @binding(5) var<uniform> u_vorticity: f32; // default .11


@vertex 
fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
    var output : VertexOutput;
    output.v_uv = vec2f(f32((vertexIndex << 1u) & 2u), f32(vertexIndex & 2u));
    output.position = vec4f(output.v_uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    let t0 = u_frame;
    var uv = in.position.xy / vec2f(u_frame.resolution.xy); // interesting. fragCoord doesn't change based on viewport
    uv.y = 1.0 - uv.y;

    let UNUSED = textureSample(color_map, u_sampler, uv);

    var w = vec2f(1.0)/vec2f(u_frame.resolution.xy);
    let data = solveFluid(uv, w, u_frame.delta_time);

    // return vec4(.1 * (6.0 + velo), 0, 1)
    // return data;
    
    // fragColor = data;
    return vec4f(fract(2. * in.v_uv), 0, 1.0);
}

// #define USE_VORTICITY_CONFINEMENT

fn mag2(p: vec2f) -> f32 { return dot(p,p); }

fn solveFluid(uv: vec2f, w: vec2f, dt: f32) -> vec4f
{
	let K = 0.2;
	let v = 0.55; // viscosity

    var data = textureSample(simulation_map, u_sampler, uv);
    var tr = textureSample(simulation_map, u_sampler, uv + vec2f(w.x , 0));
    var tl = textureSample(simulation_map, u_sampler, uv - vec2f(w.x , 0));
    var tu = textureSample(simulation_map, u_sampler, uv + vec2f(0 , w.y));
    var td = textureSample(simulation_map, u_sampler, uv - vec2f(0 , w.y));
    
    var dx: vec3f = (tr.xyz - tl.xyz)*0.5;
    var dy: vec3f = (tu.xyz - td.xyz)*0.5;
    var densDif = vec2f(dx.z ,dy.z);
    
    data.z -= dt * dot(vec3(densDif, dx.x + dy.y), data.xyz); //density
    let laplacian: vec2f = tu.xy + td.xy + tr.xy + tl.xy - 4.0*data.xy;
    let viscForce: vec2f = vec2f(v)*laplacian;
    
    var advection = textureSample(simulation_map, u_sampler, uv - dt*data.xy*w).xyw; //advection
    data.x = advection.x;
    data.y = advection.y;
    data.w = advection.z; // intentionally w to z
    
    var newForce = vec2f(0.0);
    if (u_curr_mouse.z > 0. && u_prev_mouse.z > 0.)
    {
        let vv: vec2f = clamp(vec2f(u_curr_mouse.xy*w - u_prev_mouse.xy*w)*400., vec2f(-6.), vec2f(6.));
        let delta_force = .001/(mag2(uv - u_curr_mouse.xy*w)+0.001)*vv;
        newForce.x += delta_force.x;
        newForce.y += delta_force.y;
    }
    
    let delta_vel = dt*(viscForce.xy - K/dt*densDif + newForce); //update velocity
    data.x += delta_vel.x;
    data.y += delta_vel.y;
    data.x = max(0.0, abs(data.x)-1e-4)*sign(data.x); //linear velocity decay
    data.y = max(0.0, abs(data.y)-1e-4)*sign(data.y); //linear velocity decay
    
    // #ifdef USE_VORTICITY_CONFINEMENT
   	data.w = (tr.y - tl.y - tu.x + td.x);
    var vort = vec2f(abs(tu.w) - abs(td.w), abs(tl.w) - abs(tr.w));
    vort *= u_vorticity/length(vort + 1e-9)*data.w;
    data.x += vort.x;
    data.y += vort.y;
    // #endif
    
    data.y *= smoothstep(.5,.48,abs(uv.y-0.5)); //Boundaries
    
    data = clamp(data, vec4f(vec2f(-10.0), 0.5 , -10.), vec4f(vec2f(10.0), 3.0 , 10.));
    
    return data;
}