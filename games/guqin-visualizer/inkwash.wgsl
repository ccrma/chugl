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

// @fragment 
// fn fs_main(in : VertexOutput) -> @location(0) vec4f {
//     let t0 = u_frame;
//     var uv = in.position.xy / vec2f(u_frame.resolution.xy); // interesting. fragCoord doesn't change based on viewport
//     uv.y = 1.0 - uv.y;

//     return vec4f(fract(2. * in.v_uv), 0, 1.0);
// }

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f {

    let dim : vec2u = textureDimensions(src);
    let uv = in.v_uv;
    let w: vec2f = 1.0/dim; // w = delta x, delta y in uv space

    // update velocity field
    // TODO run this 3 times per frame
    vec4 data = solveFluid(
        iChannel0, 
        in.v_uv, 
        w,  // width of each cell 
        u_frame.time, 
        vec3f(0.0), vec3f(0.0) // mouse and last_mouse
    );

    { // compute color
        vec2 velo = textureLod(iChannel0, uv, 0.).xy; // channel0 is velocity
        
        // channel1 holds colors from past frame
        vec4 col = textureLod(
            iChannel1, 
            uv - dt * velo * w * 3. // x3 because the shader runs 3 steps per frame?
            , 0.
        ); //advection
    
        vec2 brush_pos = vec2(.12, .5);
        col += (
            .0025 / (.0005 + pow(length(uv - brush_pos),1.75) ) // pick position
            
            * dt   // scale amount of ink by dt
            * 0.12 // scale down color 
            * vec4(1, 1, 1, 1) // pick color
        );
        
        col = clamp(col, 0.,5.);
        col = max(.998 * col - .0001, 0.); //decay
        
        fragColor = col;
    }
    

    return vec4f(in.v_uv, 0.0, 1.0);
}

//Chimera's Breath
//by nimitz 2018 (twitter: @stormoid)

/*
	The main interest here is the addition of vorticity confinement with the curl stored in
	the alpha channel of the simulation texture (which was not used in the paper)
	this in turns allows for believable simulation of much lower viscosity fluids.
	Without vorticity confinement, the fluids that can be simulated are much more akin to
	thick oil.
	
	Base Simulation based on the 2011 paper: "Simple and fast fluids"
	(Martin Guay, Fabrice Colin, Richard Egli)
	(https://hal.inria.fr/inria-00596050/document)

	The actual simulation only requires one pass, Buffer A, B and C	are just copies 
	of each other to increase the simulation speed (3 simulation passes per frame)
	and Buffer D is drawing colors on the simulated fluid 
	(could be using particles instead in a real scenario)
*/

const dt = 0.15;
//Recommended values between 0.03 and 0.2
//higher values simulate lower viscosity fluids (think billowing smoke)
const VORTICITY_AMOUNT = 0.2;

fn mag2(p: vec2f) -> f32 {
    return dot(p,p);
}

// force function
fn point1(t: f32) -> vec2f {
    t *= 0.62;
    return vec2f(0.12, 0.5 + sin(t) * 0.2);
}

// force function
fn point2(t: f32) -> vec2f {
    t *= 0.62;
    return vec2f(0.88,0.5 + cos(t + 1.5708)*0.2);
}

fn solveFluid(
    // sampler2D smp, 
    uv: vec2f, w: vec2f, time: f32, mouse: vec3f, lastMouse: vec3f
    // TODO refactor assuming w = 1 texel
) -> vec4f {
	const K = 0.2;
	const v = 0.55;

    // TODO uv needs to be convverted to integer texture coordinates
    var data: vec4f = textureLoad(smp, uv, 0);
    var tr: vec4f = textureLoad(smp, uv + vec2f(w.x , 0), 0); // right
    var tl: vec4f = textureLoad(smp, uv - vec2f(w.x , 0), 0); // left
    var tu: vec4f = textureLoad(smp, uv + vec2f(0 , w.y), 0); // up
    var td: vec4f = textureLoad(smp, uv - vec2f(0 , w.y), 0); // down
    
    var dx: vec3f = (tr.xyz - tl.xyz)*0.5;
    var dy: vec3f = (tu.xyz - td.xyz)*0.5;
    var densDif: vec2f = vec2f(dx.z ,dy.z);
    
    data.z -= dt*dot(vec3f(densDif, dx.x + dy.y) ,data.xyz); //density
    var laplacian: vec2f = tu.xy + td.xy + tr.xy + tl.xy - 4.0*data.xy;
    var viscForce: vec2f = vec2f(v)*laplacian;
    data.xyw = textureLoad(smp, uv - dt*data.xy*w, 0).xyw; //advection
    
    var newForce: vec2f = vec2f(0);

    // auto force vectors
    newForce.xy += 0.75*vec2f(.0003, 0.00015)/(mag2(uv-point1(time))+0.0001);
    newForce.xy -= 0.75*vec2f(.0003, 0.00015)/(mag2(uv-point2(time))+0.0001);

    // add force at mouse point
    if (mouse.z > 1. && lastMouse.z > 1.)
    {
        var vv: vec2f = clamp(vec2f(mouse.xy*w - lastMouse.xy*w)*400., -6., 6.);
        newForce.xy += .001/(mag2(uv - mouse.xy*w)+0.001)*vv;
    }
    
    // TODO: can velocity decay be played with?
    data.xy += dt*(viscForce.xy - K/dt*densDif + newForce);  // update velocity
    data.xy = max(vec2f(0), abs(data.xy) - 1e-4 * sign(data.xy)); // linear velocity decay
    
   	data.w = (tr.y - tl.y - tu.x + td.x);
    var vort: vec2f = vec2f(abs(tu.w) - abs(td.w), abs(tl.w) - abs(tr.w));
    vort *= VORTICITY_AMOUNT/length(vort + 1e-9)*data.w;
    data.xy += vort;
    
    data.y *= smoothstep(.5,.48,abs(uv.y-0.5)); //Boundaries
    
    data = clamp(data, vec4f(vec2f(-10), 0.5, -10.), vec4f(vec2f(10), 3.0, 10.));
    
    return data;
}