/*
TODO
- alpha cutoff
- UV

Extra
- bevel join
- round join
- round endcaps

check out: https://github.com/spite/THREE.ConstantSpline?tab=readme-ov-file

References
*/

public class Lines3D extends GMesh
{	
	me.dir() + "./lines3d.wgsl" @=> string shader_path;

	// set drawing shader
	static ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
    null @=> shader_desc.vertexLayout;

	// material shader (draws all line segments in 1 draw call)
	static Shader@ shader;
    if (shader == null) new Shader(shader_desc) @=> shader;

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

    // color mode enum
    0 => static int COLOR_MODE_NONE; // ignore the a_color array entirely
    1 => static int COLOR_MODE_SEGMENT; // distribute a_colors evenly over entire line, with no blending
    2 => static int COLOR_MODE_BLEND; // distribute a_colors evenly over entire line, with linear blending
    3 => static int COLOR_MODE_CYCLE; // a_color[idx % a_color.size] cycle color every line/vertex segment

    // width mode enum
    0 => static int WIDTH_MODE_NONE; 
    1 => static int WIDTH_MODE_BLEND; 
    2 => static int WIDTH_MODE_CYCLE; 

	Material line_material;
	line_material.shader(shader);
	line_material.topology(Material.Topology_TriangleStrip); 

	// vertex buffers
	vec4 u_positions[0];
	vec4 u_colors[0];
	float u_widths[0];

	Geometry line_geo; // just used to set vertex count
	line_geo.vertexCount(0);
 
    line_geo => this.geo;
    line_material => this.mat;

	float empty_float_arr[4];
	[1.0, 1, 1, 1] @=> float white_float_arr[];
    initStorageBuffers();

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

	fun void initStorageBuffers() {
		line_material.storageBuffer(BIND_ATTRIB_POSITIONS, empty_float_arr);
		line_material.storageBuffer(BIND_ATTRIB_COLORS, white_float_arr);
		line_material.storageBuffer(BIND_ATTRIB_WIDTH, white_float_arr);
	}

	fun void push(vec3 p) {
		u_positions << @(p.x, p.y, p.z, 1.0);
	}

	fun void pushColor(vec3 c) { u_colors << @(c.r, c.g, c.b, 1.0); }
	fun void pushWidth(float w) { u_widths << w; };

    fun vec4 color() { return line_material.uniformFloat4(BIND_COLOR); }
    fun void color(vec3 v) { line_material.uniformFloat4(BIND_COLOR, @(v.r, v.g, v.b, 1.0)); }
    fun void color(vec4 v) { line_material.uniformFloat4(BIND_COLOR, v); }

    fun void colorMode(int i) { line_material.uniformInt(BIND_COLOR_MODE, i); }
    fun int colorMode() { return line_material.uniformInt(BIND_COLOR_MODE); }

    fun vec3 _dash() { return line_material.uniformFloat3(BIND_DASH); }
    fun void _dash(vec3 v) { line_material.uniformFloat3(BIND_DASH, v); }

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

    fun void loop(int b) { line_material.uniformInt(BIND_LOOP, b); }
    fun int loop() { return line_material.uniformInt(BIND_LOOP); }

	fun void update()
	{
		if (u_positions.size() == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// update GPU vertex buffers
		line_material.storageBuffer(BIND_ATTRIB_POSITIONS, u_positions);

        if (u_widths.size() == 0) {
            line_material.storageBuffer(BIND_ATTRIB_WIDTH, white_float_arr);
        } else {
            line_material.storageBuffer(BIND_ATTRIB_WIDTH, u_widths);
        }

        // @TODO: fix crash when binding empty storage buffer
		if (u_colors.size() == 0) {
            line_material.storageBuffer(BIND_ATTRIB_COLORS, white_float_arr);
        } else {
            line_material.storageBuffer(BIND_ATTRIB_COLORS, u_colors);
        }

        u_positions.size() => int segment_count;
        if (loop()) segment_count++;
		line_geo.vertexCount(segment_count * 2);

		// reset
        // @TODO move into reset function
		// u_positions.clear();
		// u_colors.clear();
	}
}

fun vec3 random() {
    return @(
        Math.random2f(-10, 10),
        Math.random2f(-10, 10),
        Math.random2f(-10, 10)
    );
}

fun vec3[] catmullRom(vec3 control_points[],
                           int npoints, int loop)
{
    vec3 points[0];

    int subdivisions_per_segment;
    if (loop) npoints / control_points.size() => subdivisions_per_segment;
    else npoints / (control_points.size() - 1) => subdivisions_per_segment;
	1.0 / subdivisions_per_segment => float inc;

    int segment;
    if (!loop) 1 => segment;
    for (segment; segment < control_points.size(); segment++) {
        for (float t; t <= 1.0; inc +=> t) {
            segment - 1 => int p1;
            p1 - 1 => int p0; if (p0 < 0) control_points.size() +=> p0;
            (p1 + 1) % control_points.size() => int p2;
            (p2 + 1) % control_points.size() => int p3;

            t * t => float tt;
            tt * t => float ttt;

            -ttt + 2*tt - t => float q0;
            3*ttt - 5*tt + 2 => float q1;
            -3*ttt + 4*tt + t => float q2;
            ttt - tt => float q3;

            points << .5 * (
                control_points[p0] * q0 +
                control_points[p1] * q1 +
                control_points[p2] * q2 +
                control_points[p3] * q3
            );
        }
    }

    // add start positions to complete the loop
    if (loop) {
        points << points[0];
        points << points[1];
    }

    return points;
}


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


new GOrbitCamera => GG.scene().camera;

// disable gamma correction and tonemapping
// GG.outputPass().gamma(false);
// GG.outputPass().tonemap(OutputPass.ToneMap_None);

Lines3D line --> GG.scene();
Lines3D line_catmull --> GG.scene();
Lines3D line3 --> GG.scene();

line_catmull.color(Color.ORANGE);

// spiral(1 * Math.pi, 1.0, 2.0, 100) @=> vec3 points[];
// for (auto p : points) line.push(p);

GSphere spheres[4];
// for (auto s : spheres) s --> GG.scene();

// control points
// random() => vec3 p0;
// random() => vec3 p1;
// random() => vec3 p2;
// random() => vec3 p3;
@(-3, 0, 0) => vec3 p0;
@(-1, 1, -1) => vec3 p1;
@(1, -1, 1) => vec3 p2;
@(3, 0, 0) => vec3 p3;

line3.push(p0);
line3.push(p1);
line3.push(p2);
line3.push(p3);
line3.dashLength(.02);

p0 => spheres[0].pos;
p1 => spheres[1].pos;
p2 => spheres[2].pos;
p3 => spheres[3].pos;

catmullRom( [p0, p1, p2, p3], 200, false) @=> vec3 points2[];
for (auto p : points2) line_catmull.push(p);

bezier( p0, p1, p2, p3, 200) @=> vec3 points_bezier[];
for (auto p : points_bezier) line.push(p);


line.pushColor(Color.RED);
line.pushColor(Color.GREEN);
line.pushColor(Color.BLUE);

line.pushWidth(0.0);
// line.pushWidth(0.5);
line.pushWidth(1.0);
// line.pushWidth(0.5);
line.pushWidth(0.0);

line_catmull.pushWidth(0.0);
line_catmull.pushWidth(1.0);
line_catmull.pushWidth(0.0);

UI_Float width(line.width());
UI_Bool attenuate_size(line.sizeAttenuation());
UI_Float scale_down(line.scaleDown());

UI_Float dash_offset(line.dashOffset());
UI_Float dash_len(line.dashLength());
UI_Float dash_ratio(line.dashRatio());
UI_Float4 color(line.color());
UI_Float visibility(line.visibility());

[
    "None",
    "Segment",
    "Blend",
    "Cycle"
] @=> string color_modes[];
UI_Int color_mode_idx(line.colorMode());
UI_Bool loop(line.loop());

[
    "None",
    "Blend",
    "Cycle"
] @=> string width_modes[];
UI_Int width_mode(line.widthMode());

while (1) {
    GG.nextFrame() => now;

    if (UI.slider("width", width, 0, 1)) line.width(width.val());
    if (UI.checkbox("attenuate size", attenuate_size)) line.sizeAttenuation(attenuate_size.val());
    if (UI.slider("scale down", scale_down, 0, 10)) line.scaleDown(scale_down.val());

    if (UI.slider("dash offset", dash_offset, -2, 2)) line.dashOffset(dash_offset.val());
    if (UI.slider("dash len", dash_len, -2, 2)) line.dashLength(dash_len.val());
    if (UI.slider("dash ratio", dash_ratio, -2, 2)) line.dashRatio(dash_ratio.val());

    if (UI.colorEdit("color", color)) line.color(color.val());

    if (UI.slider("visibility", visibility, -2, 2)) line.visibility(visibility.val());
    if (UI.listBox("color mode", color_mode_idx, color_modes)) line.colorMode(color_mode_idx.val());
    if (UI.listBox("width mode", width_mode, width_modes)) line.widthMode(width_mode.val());

    if (UI.checkbox("loop", loop)) line.loop(loop.val());
    
    line.update();
    line_catmull.update();
    line3.update();
}