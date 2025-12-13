/*-----------------------------------------------------------------------------
name: meshlines.ck
desc: 3D screen-space projected lines. Supports colors, per-vertex widths, 
dashed lines, alpha-map brushstroke textures, animation, and more.
This implementation is inspired by THREE.MeshLine, with the addition of 
significant performance improvements, bug fixes, and API changes.

For testing alpha-mapping, download the brushstroke texture from:
https://chuck.stanford.edu/chugl/examples/data/textures/brush-texture.png
and uncomment the relevant lines below.

Note: this line renderer performs best on vertex data that is relatively dense
and smooth, e.g. as one would get with bezier curves or some other spline.

Prefer using GLines for lines that have sharp corners and whose vertices
are coplanar.

references:
- https://mattdesl.svbtle.com/drawing-lines-is-hard
- https://github.com/spite/THREE.MeshLine?tab=readme-ov-file
- https://threlte.xyz/docs/reference/extras/meshline-material

possible improvements:
- add depth tinting, like in noclip's gfx/helpers/DebugDraw

author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
  date: December 2025
//---------------------------------------------------------------------------*/

// scene setup
new GOrbitCamera => GG.scene().camera;
GG.scene().backgroundColor(.8 * Color.WHITE);

// generates a random vec3
fun vec3 random() { 
    return @( Math.random2f(-10, 10), Math.random2f(-10, 10), Math.random2f(-10, 10)); 
}

// color palette
[
	Color.hex(0xed6a5a), Color.hex(0xf4f1bb), Color.hex(0x9bc1bc),
	Color.hex(0x5ca4a9), Color.hex(0xe6ebe0), Color.hex(0xf0b67f),
	Color.hex(0xfe5f55), Color.hex(0xd6d1b1), Color.hex(0xc7efcf),
	Color.hex(0xeef5db), Color.hex(0x50514f), Color.hex(0xf25f5c),
	Color.hex(0xffe066), Color.hex(0x247ba0), Color.hex(0x70c1b3)
] @=> vec3 random_colors[];
fun vec3 randomColor() {
    return random_colors[Math.random2(0, random_colors.size()-1)];
}


GGen line_transform --> GG.scene();
MeshLines lines[0];
// function to create and store a MeshLines object
fun void addLine() {
    MeshLines line --> line_transform;

    random() => vec3 p0;
    random() => vec3 p1;
    random() => vec3 p2;
    random() => vec3 p3;

    bezier( p0, p1, p2, p3, 200) => line.positions;
    [0.0, 1, 0] => line.widths; // taper the line from start to end
    [randomColor(), randomColor(), randomColor()] => line.colors;

    lines << line;
}

// add the lines to our scene
repeat(50) addLine();

// disable gamma correction and tonemapping
GG.outputPass().gamma(0);
GG.outputPass().tonemap(0);

// UI Params
UI_Float width(lines[0].width());
UI_Bool attenuate_size(lines[0].sizeAttenuation());
UI_Float scale_down(lines[0].scaleDown());
UI_Float dash_offset(lines[0].dashOffset());
UI_Float dash_len(1.0);
UI_Float dash_ratio(lines[0].dashRatio());
UI_Float4 color(lines[0].color());
UI_Float visibility(lines[0].visibility());

[
    "None",
    "Segment",
    "Blend",
    "Cycle"
] @=> string color_modes[];
UI_Int color_mode_idx(lines[0].colorMode());
UI_Bool loop(lines[0].loop());

[
    "None",
    "Blend",
    "Cycle"
] @=> string width_modes[];
UI_Int width_mode(lines[0].widthMode());

UI_Bool animate(true);

// uncomment this and all lines with `alpha_map` to test applying a brush texture!
// (first you need to get a brush texture)
// Texture.load(me.dir() + "./brush-texture.png") @=> Texture brush_texture;
// UI_Bool alpha_map(false);

// render loop
while (1) {
    GG.nextFrame() => now;
    .1 * GG.dt() => line_transform.rotateY;

    // build UI widgets
    UI.slider("width", width, 0, 1);
    UI.checkbox("animate", animate);
    if (!animate.val()) {
        UI.slider("dash offset", dash_offset, 0, 20);
    }
    UI.slider("dash len", dash_len, 0, 1);
    UI.slider("dash ratio", dash_ratio, 0, 1);
    UI.colorEdit("color", color);
    UI.slider("visibility", visibility, -2, 2);
    UI.listBox("color mode", color_mode_idx, color_modes);
    UI.listBox("width mode", width_mode, width_modes);
    UI.checkbox("attenuate size", attenuate_size);
    UI.slider("scale down", scale_down, 0, .1);
    UI.checkbox("loop", loop);
    // UI.checkbox("brush texture", alpha_map);

    // update all lines
    for (auto line : lines) {
        line.width(width.val());
        line.sizeAttenuation(attenuate_size.val());
        line.scaleDown(scale_down.val());
        line.color(color.val());
        line.visibility(visibility.val());
        line.colorMode(color_mode_idx.val());
        line.widthMode(width_mode.val());
        line.loop(loop.val());

        line.dashLength(dash_len.val());
        line.dashRatio(dash_ratio.val());
        // animate via dash offset
        if (animate.val()) {
            line.dashOffset(.2*(now/second));
        } else {
            line.dashOffset(dash_offset.val());
        }

        // if (alpha_map.val()) {
        //     line.alphaMap(brush_texture);
        // } else {
        //     line.alphaMap(MeshLines.white_pixel);
        // }
    }
}

// bezier curve builder
fun vec3[] bezier(
	vec3 p0, vec3 p1, vec3 p2, vec3 p3, // control points
	int npoints
) {
    vec3 points[0];

	1.0 / npoints => float inc;
	for( float t; t <= 1; inc +=> t ) {
        1 - t => float k;
	    points << (
            k * k * k * p0
            + 3 * k * k * t * p1
            + 3 * k * t * t * p2
	        + t * t * t * p3
        );
    }

    return points;
}

// implementation
public class MeshLines extends GMesh
{	
    "
    #include FRAME_UNIFORMS
#include DRAW_UNIFORMS

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) v_dash_counter: f32, // ratio of vertex along line segment, from [0,1]. vertex N/2 has value .5
    @location(1) v_visibility_counter: f32, // ratio of vertex along line segment, from [0,1]. vertex N/2 has value .5
    @location(2) v_color: vec4f,
    @location(3) v_uv: vec2f,
};


const COLOR_MODE_NONE = 0; // ignore the a_color array entirely
const COLOR_MODE_SEGMENT = 1; // distribute a_colors evenly over entire line, with no blending
const COLOR_MODE_BLEND = 2; // distribute a_colors evenly over entire line, with linear blending
const COLOR_MODE_CYCLE = 3; // a_color[idx % a_color.size] cycle color every line/vertex segment

const WIDTH_MODE_NONE = 0; // ignore width attribute
const WIDTH_MODE_BLEND = 1; // distribute a_widths evenly over line with linear blending
const WIDTH_MODE_CYCLE = 2; // cycle width for every line/vertex segment

@group(1) @binding(0) var<storage> a_position : array<f32>;  // f32 instead of vec3f because of bullshit byte alignment 
@group(1) @binding(1) var<storage> a_color : array<vec4f>; 
@group(1) @binding(2) var<storage> a_width : array<f32>;  // per-vertex width modifier

@group(1) @binding(3) var<uniform> u_color : vec4f;
@group(1) @binding(4) var<uniform> u_color_mode : i32;
@group(1) @binding(5) var<uniform> u_thickness : f32;
@group(1) @binding(6) var<uniform> u_size_attenuation : i32;
@group(1) @binding(7) var<uniform> u_scale_down : f32;
@group(1) @binding(8) var<uniform> u_dash : vec3f; // @(offset, len, ratio)
@group(1) @binding(9) var<uniform> u_visibility : f32; // % of line that is shown
@group(1) @binding(10) var<uniform> u_loop : i32;
@group(1) @binding(11) var<uniform> u_width_mode : i32;

// alpha params
@group(1) @binding(12) var alpha_map: texture_2d<f32>;
@group(1) @binding(13) var texture_sampler: sampler;
@group(1) @binding(14) var<uniform> u_alpha_cutoff : f32;
@group(1) @binding(15) var<uniform> u_texture_scale : vec2f;

fn intoScreen(i: vec4f) -> vec2f {
    return vec2f(u_frame.resolution.xy) * (0.5 * i.xy / i.w + 0.5);
}

@vertex
fn vs_main(
    @builtin(instance_index) instance_idx : u32,    // carry over from everything being indexed...
    @builtin(vertex_index) vertex_idx : u32,        // used to determine which polygon we are drawing
) -> VertexOutput {
    var out : VertexOutput;
    var u_Draw : DrawUniforms = u_draw_instances[instance_idx];
    let proj_view_model = u_frame.projection * u_frame.view * u_Draw.model;
    let resolution = vec2f(u_frame.resolution.xy);

    // @TODO make param
    let line_num_vertices = i32(arrayLength(&a_position) / 3);
    let line_num_colors = i32(arrayLength(&a_color));
    let line_num_widths = i32(arrayLength(&a_width));
    let orientation = f32((i32(vertex_idx) % 2) * 2 - 1); // -1 if vertex_idx is even, else 1

    // compute which 
    let segment_idx = i32(vertex_idx) / 2;
    var curr_idx : i32 = 0;
    var prev_idx : i32 = 0;
    var next_idx : i32 = 0;
    if (u_loop > 0) {
        curr_idx = (i32(vertex_idx) / 2) % line_num_vertices;
        prev_idx = select(curr_idx - 1, line_num_vertices - 1, curr_idx == 0);
        next_idx = select(curr_idx + 1, 0, curr_idx == line_num_vertices - 1);
    } else {
        curr_idx = i32(vertex_idx) / 2;
        prev_idx = max(curr_idx - 1, 0);
        next_idx = min(curr_idx + 1, line_num_vertices - 1);
    }

    var actual_thickness = u_thickness;
    if (line_num_widths > 0) {
        if (u_width_mode == WIDTH_MODE_BLEND) {
            let width_idx = (f32(curr_idx) / f32(line_num_vertices-1) * f32(line_num_widths-1));
            actual_thickness *= mix(
                a_width[i32(width_idx) % line_num_widths], 
                a_width[(i32(width_idx) + 1) % line_num_widths], 
                fract(width_idx)
            );
        }
        else if (u_width_mode == WIDTH_MODE_CYCLE) {
            actual_thickness *= a_width[segment_idx % line_num_widths];
        }
    }


    let curr_pos = vec3f(
        a_position[curr_idx * 3 + 0],
        a_position[curr_idx * 3 + 1],
        a_position[curr_idx * 3 + 2],
    );
    let curr_proj = proj_view_model * vec4f(curr_pos, 1.0);
    let curr_normed = curr_proj / curr_proj.w;
    let curr_screen = intoScreen(curr_normed);

    let next_pos = vec3f(
        a_position[next_idx * 3 + 0],
        a_position[next_idx * 3 + 1],
        a_position[next_idx * 3 + 2],
    );
    let next_proj = proj_view_model * vec4f(next_pos, 1.0);
    let next_normed = next_proj / next_proj.w;
    let next_screen = intoScreen(next_normed);

    let prev_pos = vec3f(
        a_position[prev_idx * 3 + 0],
        a_position[prev_idx * 3 + 1],
        a_position[prev_idx * 3 + 2],
    );
    let prev_proj = proj_view_model * vec4f(prev_pos, 1.0);
    let prev_normed = prev_proj / prev_proj.w;
    let prev_screen = intoScreen(prev_normed);

    var dir: vec2f = vec2f(0.0);
    if (all(prev_screen == curr_screen)) { // first point
        dir = normalize(next_screen - curr_screen);
    } 
    else if (all(next_screen == curr_screen)) { // last point
        dir = normalize(curr_screen - prev_screen);
    } else { // middle point
        let inDir = curr_screen - prev_screen;
        let  outDir = next_screen - curr_screen;
        let  fullDir = next_screen - prev_screen;

        if(length(fullDir) > 0.0) {
            dir = normalize(fullDir);
        } else if(length(inDir) > 0.0){
            dir = normalize(inDir);
        } else {
            dir = normalize(outDir);
        }

        // old approach
        // let dir1 = normalize( curr_screen - prev_screen );
        // let dir2 = normalize( next_screen - prev_screen );
        // dir = normalize( dir1 + dir2 );
        // vec2 perp = vec2( -dir1.y, dir1.x );
        // vec2 miter = vec2( -dir.y, dir.x );
        //w = clamp( w / dot( miter, perp ), 0., 4. * lineWidth * width );
    }

    var normal = vec2(-dir.y, dir.x);

    if(u_size_attenuation > 0) {
        normal /= curr_proj.w;
        normal *= min(resolution.x, resolution.y);
    }

    if (u_scale_down > 0.0) {
        let dist = length(next_normed - prev_normed);
        normal *= smoothstep(0.0, u_scale_down, dist);
    }

    let offsetInScreen = actual_thickness * normal * orientation * 0.5;
    let withOffsetScreen = curr_screen + offsetInScreen;
    let withOffsetNormed = vec3f((2.0 * withOffsetScreen/vec2f(u_frame.resolution.xy) - 1.0), curr_normed.z);

    let counter = f32(segment_idx) / f32(line_num_vertices);
    var color = u_color;
    if (line_num_colors > 0 && u_color_mode > 0) {
        if (u_color_mode == COLOR_MODE_CYCLE) {
            color *= a_color[segment_idx % line_num_colors];
        }
        else if (u_color_mode == COLOR_MODE_SEGMENT) {
            color *= a_color[min(i32(counter * f32(line_num_colors)), line_num_colors -1)];
        }
        else if (u_color_mode == COLOR_MODE_BLEND) {
            let color_idx = (f32(curr_idx) / f32(line_num_vertices-1) * f32(line_num_colors-1));
            color *= mix(
                a_color[i32(color_idx) % line_num_colors], 
                a_color[(i32(color_idx) + 1) % line_num_colors], 
                fract(color_idx)
            );
        }
    }

    out.position = curr_proj.w * vec4f(withOffsetNormed, 1.0);
    out.v_dash_counter = f32(curr_idx) / f32(line_num_vertices); // this works better with dash + loop
    out.v_visibility_counter = counter;
    out.v_color = color;
    if (u_loop > 0) {
        out.v_uv.x = f32(segment_idx) / f32(line_num_vertices);
    } else {
        out.v_uv.x = f32(curr_idx) / f32(line_num_vertices - 1);
    }
    out.v_uv.y = select(0.0, 1.0, orientation > 0);

    return out;
}

@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    let dashOffset = u_dash.x;
    let dashArray = u_dash.y;
    let dashRatio = u_dash.z;

    var dash_discard = false;
    if (dashArray > 0.0 && dashRatio > 0.0) {
        dash_discard = (modf((in.v_dash_counter + dashOffset) / dashArray).fract - (dashRatio)) < 0.0;
    }

    var color = in.v_color;
    color.a *= step(in.v_visibility_counter, u_visibility);
    color.a *= textureSample(alpha_map, texture_sampler, in.v_uv * u_texture_scale).r; // @TODO mult by alpha scale

    if (dash_discard || color.a <= u_alpha_cutoff) {
        discard;
    }

    return color;
}
    " => string shader_code;

	// material shader, shared by all MeshLines instances
	static ShaderDesc shader_desc;
	static Shader@ shader;

    // internal shader enums (don't touch!)
    0 => static int BIND_ATTRIB_POSITIONS;
    1 => static int BIND_ATTRIB_COLORS;
    2 => static int BIND_ATTRIB_WIDTH;
    3 => static int BIND_COLOR;
    4 => static int BIND_COLOR_MODE;
    5 => static int BIND_WIDTH;
    6 => static int BIND_SIZE_ATTENUATION;
    7 => static int BIND_SCALE_DOWN;
    8 => static int BIND_DASH;
    9 => static int BIND_VISIBILITY;
    10 => static int BIND_LOOP;
    11 => static int BIND_WIDTH_MODE;
    12 => static int BIND_ALPHA_MAP;
    13 => static int BIND_SAMPLER;
    14 => static int BIND_ALPHA_CUTOFF;
    15 => static int BIND_TEXTURE_SCALE;

    // color mode enum
    0 => static int COLOR_MODE_NONE; // ignore the a_color array entirely
    1 => static int COLOR_MODE_SEGMENT; // distribute a_colors evenly over entire line, with no blending
    2 => static int COLOR_MODE_BLEND; // distribute a_colors evenly over entire line, with linear blending
    3 => static int COLOR_MODE_CYCLE; // a_color[idx % a_color.size] cycle color every line segment

    // width mode enum
    0 => static int WIDTH_MODE_NONE; // ignore the widths array entirely
    1 => static int WIDTH_MODE_BLEND;  // distribute widths over entire line, with linear blending
    2 => static int WIDTH_MODE_CYCLE;  // cycle width every line segment

    // default binding values
	static float empty_float_arr[4];
	[1.0, 1, 1, 1] @=> static float white_float_arr[];
    static Texture@ white_pixel;
    if (white_pixel == null) {
        Texture tex @=> white_pixel;
        tex.write(white_float_arr);
    }

    // local params
	Material line_material;
	Geometry line_geo; // just used to set vertex count
    int n_positions; // #line vertices

    // constructor
    fun MeshLines() {
        // create shader if not already created
        if (shader == null) {
            shader_code => shader_desc.vertexCode;
            shader_code => shader_desc.fragmentCode;
            null @=> shader_desc.vertexLayout;
            new Shader(shader_desc) @=> shader;
        }

        // init material shader
        line_material.shader(shader);
        line_material.topology(Material.Topology_TriangleStrip); 

        // init storage buffers
        line_material.storageBuffer(BIND_ATTRIB_POSITIONS, empty_float_arr);
        line_material.storageBuffer(BIND_ATTRIB_COLORS, white_float_arr);
        line_material.storageBuffer(BIND_ATTRIB_WIDTH, white_float_arr);

        // prep geo
        line_geo.vertexCount(0);

        // set geo and mat on GMesh
        line_geo => this.geo;
        line_material => this.mat;

        // init uniforms
        width(.1);
        sizeAttenuation(true);
        scaleDown(0.0);
        _dash(@(0,0,.5));
        color(Color.WHITE);
        visibility(1.0);
        colorMode(COLOR_MODE_BLEND);
        loop(false);
        widthMode(WIDTH_MODE_BLEND);
        alphaMap(white_pixel);
        sampler(TextureSampler.linear());
        alphaCutoff(0);
        textureScale(@(1,1));
    }

// == PUBLIC API =======================================================
	fun void positions(vec3 p[]) {
        if (p == null || p.size() < 2) {
            line_material.storageBuffer(BIND_ATTRIB_POSITIONS, empty_float_arr);
            0 => n_positions;
        } else {
            line_material.storageBuffer(BIND_ATTRIB_POSITIONS, p);
            p.size() => n_positions;
        }
        _updateVertexCount();
	}

	fun void colors(vec3 colors[]) {
        if (colors == null || colors.size() == 0) {
            line_material.storageBuffer(BIND_ATTRIB_COLORS, white_float_arr);
        } else {
            vec4 co[0];
            for (auto c : colors ) co << @(c.r, c.g, c.b, 1.0);
            line_material.storageBuffer(BIND_ATTRIB_COLORS, co);
        }
    }

	fun void colors(vec4 colors[]) {
        if (colors == null || colors.size() == 0) {
            line_material.storageBuffer(BIND_ATTRIB_COLORS, white_float_arr);
        } else {
            line_material.storageBuffer(BIND_ATTRIB_COLORS, colors);
        }
    }

	fun void widths(float w[]) {
        if (w == null || w.size() == 0) {
            line_material.storageBuffer(BIND_ATTRIB_WIDTH, white_float_arr);
        } else {
            line_material.storageBuffer(BIND_ATTRIB_WIDTH, w);
        }
    }

    fun vec4 color() { return line_material.uniformFloat4(BIND_COLOR); }
    fun void color(vec3 v) { line_material.uniformFloat4(BIND_COLOR, @(v.r, v.g, v.b, 1.0)); }
    fun void color(vec4 v) { line_material.uniformFloat4(BIND_COLOR, v); }

    fun void colorMode(int i) { line_material.uniformInt(BIND_COLOR_MODE, i); }
    fun int colorMode() { return line_material.uniformInt(BIND_COLOR_MODE); }

    fun float dashOffset() { return line_material.uniformFloat3(BIND_DASH).x; }
    fun void dashOffset(float f) { 
        _dash() => vec3 d;
        f => d.x;
        line_material.uniformFloat3(BIND_DASH, d); 
    }

    // set the length of each dashed segment as a fractional ratio of the entire line
    // range: [0, 1]
    fun float dashLength() { return line_material.uniformFloat3(BIND_DASH).y; }
    fun void dashLength(float f) { 
        _dash() => vec3 d;
        f => d.y;
        line_material.uniformFloat3(BIND_DASH, d); 
    }

    fun float dashRatio() { return line_material.uniformFloat3(BIND_DASH).z; }
    fun void dashRatio(float f) { 
        _dash() => vec3 d;
        f => d.z;
        line_material.uniformFloat3(BIND_DASH, d); 
    }

    fun float scaleDown() { return line_material.uniformFloat(BIND_SCALE_DOWN); }
    fun void scaleDown(float f) { line_material.uniformFloat(BIND_SCALE_DOWN, f); }

    fun int sizeAttenuation() {
        return line_material.uniformInt(BIND_SIZE_ATTENUATION);
    }
    fun void sizeAttenuation(int attenuate) {
        line_material.uniformInt(BIND_SIZE_ATTENUATION, attenuate ? true : false);
    }

    // set the width in worldspace units
    fun void width(float w) { line_material.uniformFloat(BIND_WIDTH, w); }
    fun float width() { return line_material.uniformFloat(BIND_WIDTH); }

    fun void widthMode(int m) { line_material.uniformInt(BIND_WIDTH_MODE, m); }
    fun int widthMode() { return line_material.uniformInt(BIND_WIDTH_MODE); }

    // percentage of line that is shown
    fun void visibility(float w) { line_material.uniformFloat(BIND_VISIBILITY, w); }
    fun float visibility() { return line_material.uniformFloat(BIND_VISIBILITY); }

    fun void loop(int b) { line_material.uniformInt(BIND_LOOP, b); _updateVertexCount(); }
    fun int loop() { return line_material.uniformInt(BIND_LOOP); }

    fun void alphaMap(Texture t) { line_material.texture(BIND_ALPHA_MAP, t); }
    fun Texture alphaMap() { return line_material.texture(BIND_ALPHA_MAP); }

    fun void sampler(TextureSampler s) { line_material.sampler(BIND_SAMPLER, s); }
    fun TextureSampler sampler() { return line_material.sampler(BIND_SAMPLER); }

    fun void alphaCutoff(float c) { line_material.uniformFloat(BIND_ALPHA_CUTOFF, c); }
    fun float alphaCutoff() { return line_material.uniformFloat(BIND_ALPHA_CUTOFF); }

    fun void textureScale(vec2 s) { line_material.uniformFloat2(BIND_TEXTURE_SCALE, s); }
    fun vec2 textureScale() { return line_material.uniformFloat2(BIND_TEXTURE_SCALE); }
// == END PUBLIC API =======================================================

// == PRIVATE API ==========================================================
    fun vec3 _dash() { return line_material.uniformFloat3(BIND_DASH); }
    fun void _dash(vec3 v) { line_material.uniformFloat3(BIND_DASH, v); }

    fun void _updateVertexCount() {
        if (loop()) line_geo.vertexCount((n_positions + 1) * 2);
        else        line_geo.vertexCount(n_positions * 2);
    }
}

