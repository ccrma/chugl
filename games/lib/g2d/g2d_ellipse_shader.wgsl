#include FRAME_UNIFORMS
#include DRAW_UNIFORMS

const a = 1.01f;
var<private> QUAD_VERTICES : array<vec2f, 6> = array(
    vec2f(-a, -a), // bottom left
    vec2f(a, -a),  // bottom right
    vec2f(-a, a),  // top left
    vec2f(a, -a),  // bottom right
    vec2f(a, a),   // top right
    vec2f(-a, a)   // top left
);

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) v_local_pos: vec2f, // local position in bounding box, between [0, 1]
    @location(1) v_ab: vec2f,
    @location(2) @interpolate(flat) v_thickness: f32,
    @location(3) v_color: vec4f,
};

// each ellipse contains the following:
// [ vec2 center, vec2 ab, f32 thickness, f32 layer, vec4 color]
// 10 floats per ellipse
@group(1) @binding(0) var<storage> u_data : array<f32>;
// @group(1) @binding(0) var<storage> u_center_radius_thickness : array<vec4f>; // .xy is center, .z is radius, .w is thickness
// @group(1) @binding(1) var<storage> u_color : array<vec4f>;
// @group(1) @binding(2) var<storage> u_layer : array<f32>;
@group(1) @binding(1) var<uniform> u_antialias : i32;

@vertex
fn vs_main(
    @builtin(instance_index) instance_idx : u32,    // carry over from everything being indexed...
    @builtin(vertex_index) vertex_idx : u32,        // used to determine which polygon we are drawing
) -> VertexOutput {
    var out : VertexOutput;

    // compute which polygon we are drawing in this batch
    let shape_idx =  vertex_idx / 6u;
    let quad_vertex : vec2f = QUAD_VERTICES[vertex_idx % 6u]; 

    // let center_radius_thickness = u_center_radius_thickness[circle_idx];
    let center = vec2f(u_data[10*shape_idx + 0], u_data[10*shape_idx + 1]);
    let ab = vec2f(u_data[10*shape_idx + 2], u_data[10*shape_idx + 3]);
    let thickness = u_data[10*shape_idx + 4];
    let layer = u_data[10*shape_idx + 5];
    let color = vec4f(
        u_data[10*shape_idx + 6],
        u_data[10*shape_idx + 7],
        u_data[10*shape_idx + 8],
        u_data[10*shape_idx + 9]
    );

    let p = ab * quad_vertex + center; // transform quad to world space
    var u_Draw : DrawUniforms = u_draw_instances[instance_idx];
    out.position = (u_frame.projection * u_frame.view) * u_Draw.model * vec4f(p, layer, 1.0f);

    out.v_local_pos = ab * quad_vertex;
    out.v_ab = ab;
    out.v_thickness = thickness;
    out.v_color = color;

    return out;
}

// https://www.shadertoy.com/view/4lsXDN
// Computes the distance to an ellipse by using a Newton solver instead of an analytical solution
// there is possibly a faster impl here: (needs to be profiled https://www.shadertoy.com/view/tt3yz7)
//    but this one is numerically unstable for a or b = 0
fn sdEllipse( point: vec2f, ab: vec2f ) -> f32
{
    // symmetry
	let p = abs( point );

    // find root with Newton solver
    let q : vec2f = ab*(p-ab);
	var w : f32 = 0.0;
    if (q.x<q.y) { w = 1.570796327; }  
    for( var i : i32=0; i<5; i++ )
    {
        let cs : vec2f = vec2(cos(w),sin(w));
        let u : vec2f = ab*vec2( cs.x,cs.y);
        let v : vec2f = ab*vec2(-cs.y,cs.x);
        w = w + dot(p-u,v)/(dot(p-u,u)+dot(v,v));
    }
    
    // compute final point and distance
    let d : f32 = length(p-ab*vec2(cos(w),sin(w)));
    
    // return signed distance
    if (dot(p/ab,p/ab)>1.0) {
        return d;
    } else {
        return -d;
    }
}

// begin fragment shader ----------------------------
// derivative anti-aliasing from http://www.numb3r23.net/2015/08/17/using-fwidth-for-distance-based-anti-aliasing/
@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    let sdf = sdEllipse(in.v_local_pos, in.v_ab);

    var alpha = 1.0;
    let aaf = fwidth(sdf); // anti alias field
    if (bool(u_antialias)) {
        alpha = smoothstep(-in.v_thickness - aaf, -in.v_thickness, sdf) - 
                    smoothstep(0.0, aaf, sdf);
    } else {
        alpha = step(-in.v_thickness - aaf, sdf) - step(0.0, sdf);
    }
    
    if (alpha < .01) { discard; }
    return vec4f(in.v_color.rgb, alpha * in.v_color.a);

    // return vec4f(in.v_color);
}