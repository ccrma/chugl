/*

Immediate-mode vector graphics library

TODO add depth/layer option

*/

public class G2D
{
    // initialize batch drawers
    G2D_Circles circles;
    G2D_Lines lines;
    G2D_SolidPolygon polygons;
    // G2D_Sprite sprites; // TODO wip. might require texture atlas. 
    G2D_Capsule capsules;

    // connect to scene
    circles.mesh --> GG.scene();
    lines.mesh --> GG.scene();
    polygons.mesh --> GG.scene();
    // sprites.mesh --> GG.scene();
	capsules.mesh --> GG.scene();


    // ----------- circle ----------
	fun void circle(vec2 center, float radius, float thickness, vec3 color) {
        circles.circle(center, radius, thickness, @(color.r, color.g, color.b, 1.0));
	}

    // ----------- line -----------
    // draws a dashed line from p1 to p2, each dash is length `segment_length`
	fun void dashed(vec2 p1, vec2 p2, vec3 color, float segment_length) {
		p2 - p1 => vec2 d;
		Math.euclidean(p1, p2) => float dist;
		d / dist => vec2 dir;
		dist / segment_length => float N;

		for (int i; i < N; 2 +=> i) {
			lines.segment(
				(p1 + i * segment_length * dir),
				(p1 + Math.min((i + 1), N) * segment_length * dir),
				color
			);
		}
	}

	fun void line(vec2 p1, vec2 p2, vec3 color) {
		lines.segment(p1, p2, color);
	}

	// draws a polygon outline at given position and rotation
	fun void polygon(vec2 pos, float rot_radians, vec2 vertices[], vec2 scale, vec3 color) {
		if (vertices.size() < 2) return;

        Math.cos(rot_radians) => float cos_a;
        Math.sin(rot_radians) => float sin_a;

		// just draw as individual line segments for now
		for (int i; i < vertices.size(); i++) {
			vertices[i] => vec2 v1;
			vertices[(i + 1) % vertices.size()] => vec2 v2;

			// scale
			scale.x *=> v1.x; scale.y *=> v1.y;
			scale.x *=> v2.x; scale.y *=> v2.y;

			lines.segment(
				pos + @(
					cos_a * v1.x - sin_a * v1.y,
					sin_a * v1.x + cos_a * v1.y
				),
				pos + @(
					cos_a * v2.x - sin_a * v2.y,
					sin_a * v2.x + cos_a * v2.y
				),
                color
			);
		}

		// // to close the loop
		// vertices[-1] => vec2 v1;
		// vertices[0] => vec2 v2;

		// // scale
		// scale.x *=> v1.x; scale.y *=> v1.y;
		// scale.x *=> v2.x; scale.y *=> v2.y;

		// drawlines.segment(
		// 	pos + @(
		// 		cos_a * v1.x - sin_a * v1.y,
		// 		sin_a * v1.x + cos_a * v1.y
		// 	),
		// 	pos + @(
		// 		cos_a * v2.x - sin_a * v2.y,
		// 		sin_a * v2.x + cos_a * v2.y
		// 	)
		// );
	}

	fun void polygon(vec2 pos, float rot_radians, vec2 vertices[], vec3 color) {
		polygon(pos, rot_radians, vertices, @(1,1), color);
	}

	[@(0.5, 0.5), @(-0.5, 0.5), @(-0.5, -0.5), @(0.5, -0.5)] @=> vec2 square_vertices[];
	fun void box(vec2 center, float width, float height, vec3 color) {
		polygon(center, 0, square_vertices, @(width, height), color);
	}

	fun void square(vec2 pos, float rot_radians, float size, vec3 color) {
		polygon(pos, rot_radians, square_vertices, @(size, size), color);
	}

	32 => int circle_segments;
	vec2 circle_vertices[circle_segments];
	for (int i; i < circle_segments; i++) {
		Math.two_pi * i / circle_segments => float theta;
		@( Math.cos(theta), Math.sin(theta)) => circle_vertices[i];
	}
	fun void circle(vec2 pos, float radians, vec3 color) {
		for (int i; i < circle_vertices.size() - 1; i++) {
			lines.segment(pos + radians * circle_vertices[i], pos + radians * circle_vertices[i+1], color);
		}
		lines.segment(pos + radians * circle_vertices[-1], pos + radians * circle_vertices[0], color);
	}

	vec2 dotted_circle_vertices[0];
	fun void dottedCircle(vec2 pos, float radius, float start_theta, vec3 color) {
		dotted_circle_vertices.clear();
		// draw every other segment
		for (int i; i < 32; i++) {
			(Math.two_pi * i / 32) + start_theta => float theta;
			pos + radius * @(Math.cos(theta), Math.sin(theta)) => vec2 vertex;
			dotted_circle_vertices << vertex;
		}

		for (int i; i < 32; 2 +=> i) {
			lines.segment(dotted_circle_vertices[i], dotted_circle_vertices[i+1], color);
		}
	}

    // ---------- filled polygons ----------
	fun void boxFilled(
		vec2 position,
		float rotation_radians,
		float width,
		float height,
		vec3 color
	) {
		.5 * width => float hw;
		.5 * height => float hh;
		polygons.polygonFilled(
			position,
			rotation_radians,
			[@(-hw, hh), @(-hw, -hh), @(hw, -hh), @(hw, hh)], 
			0,
			color	
		);
	}

	fun void boxFilled(vec2 top_left, vec2 bottom_right, vec3 color) {
		boxFilled(
			(top_left + bottom_right) * .5, // pos
			0, // rot
			Math.fabs(top_left.x - bottom_right.x), // w
			Math.fabs(top_left.y - bottom_right.y), // h
			color
		);
	}

	fun void squareFilled(
		vec2 position,
		float rotation_radians,
		float l,
		vec3 color
	) {
		boxFilled(position, rotation_radians, l, l, color);
	}

	fun void capsuleFilled(
		vec2 p1, vec2 p2, float radius, vec3 color
	) {
		capsules.capsule(p1, p2, radius, @(color.r, color.g, color.b, 1.0));
	}

    // -----------------------------------------

    fun void update() {
        circles.update();
        lines.update();
        polygons.update();
        // sprites.update();
		capsules.update();
    }
}

public class G2D_Circles
{
	me.dir() + "./g2d_circle_shader.wgsl" @=> string shader_path;

	// set drawing shader
	ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	null @=> shader_desc.vertexLayout; 

	// material shader (draws all line segments in 1 draw call)
	Shader shader(shader_desc);
	Material material;
	material.shader(shader);
	antialias(true); // default antialiasing to true

	// default bindings
	float empty_float_arr[4];
	initStorageBuffers();

	// vertex buffers
	vec4 u_center_radius_thickness[0];
	vec4 u_colors[0];

	Geometry geo;
	geo.vertexCount(0);
	GMesh mesh(geo, material);

	fun void initStorageBuffers() {
		material.storageBuffer(0, empty_float_arr);
		material.storageBuffer(1, empty_float_arr);
	}

	// set whether to antialias circles
	fun void antialias(int value) {
		material.uniformInt(2, value);
	}

	fun void circle(vec2 center, float radius, float thickness, vec4 color) {
		u_center_radius_thickness << @(center.x, center.y, radius, thickness);
		u_colors << color;
	}

	fun void update() {
		if (u_center_radius_thickness.size() == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// update GPU vertex buffers
		material.storageBuffer(0, u_center_radius_thickness);
		material.storageBuffer(1, u_colors);
		geo.vertexCount(6 * u_center_radius_thickness.size());

		// reset
		u_center_radius_thickness.clear();
		u_colors.clear();
	}
}

// batch draws simple line segments (no width)
public class G2D_Lines
{
	me.dir() + "./g2d_lines_shader.wgsl" @=> string shader_path;

	// set drawing shader
	ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	[VertexFormat.Float2, VertexFormat.Float3] @=> shader_desc.vertexLayout;

	// material shader (draws all line segments in 1 draw call)
	Shader shader(shader_desc);
	Material material;
	material.shader(shader);

	// ==optimize== use lineStrip topology + index reset? but then requires using additional index buffer
	material.topology(Material.Topology_LineList); // list not strip!

	// vertex buffers
	vec2 u_positions[0];
	vec3 u_colors[0];

	Geometry geo; // just used to set vertex count
	geo.vertexCount(0);
	GMesh mesh(geo, material);

	fun void segment(vec2 p1, vec2 p2, vec3 color) {
		u_positions << p1 << p2;
		u_colors << color << color;
	}

	fun void update()
	{
		// update GPU vertex buffers
		geo.vertexAttribute(0, u_positions);
		geo.vertexCount(u_positions.size());
		geo.vertexAttribute(1, u_colors);

		// reset
		u_positions.clear();
		u_colors.clear();
	}
}

public class G2D_SolidPolygon
{
	me.dir() + "./g2d_solid_polygon_shader.wgsl" @=> string shader_path;

	// set drawing shader
	ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	null => shader_desc.vertexLayout; // no vertex layout

	// material shader (draws all solid polygons in 1 draw call)
	Shader shader(shader_desc);
	Material solid_polygon_material;
	solid_polygon_material.shader(shader);

	// storage buffers
	int u_polygon_vertex_counts[0];
	vec2 u_polygon_vertices[0];
	vec4 u_polygon_transforms[0];
	vec4 u_polygon_colors[0];
	vec4 u_polygon_aabb[0];
	float u_polygon_radius[0];
	int num_solid_polygons;

	// initialize material uniforms
	// TODO: binding empty storage buffer crashes wgpu
	int empty_int_arr[1];
	float empty_float_arr[4];
	initStorageBuffers();

	Geometry solid_polygon_geo; // just used to set vertex count
	GMesh mesh(solid_polygon_geo, solid_polygon_material);

	@(Math.FLOAT_MAX, Math.FLOAT_MAX, -Math.FLOAT_MAX, -Math.FLOAT_MAX) => vec4 init_aabb;

	fun void initStorageBuffers() {
		solid_polygon_material.storageBuffer(0, empty_int_arr);
		solid_polygon_material.storageBuffer(1, empty_float_arr);
		solid_polygon_material.storageBuffer(2, empty_float_arr);
		solid_polygon_material.storageBuffer(3, empty_float_arr);
		solid_polygon_material.storageBuffer(4, empty_float_arr);
		solid_polygon_material.storageBuffer(5, empty_float_arr);
	}

	fun void polygonFilled(
		vec2 position,
		float rotation_radians,
		vec2 vertices[], 
		float radius,
		vec3 color
	) {

		u_polygon_vertex_counts << u_polygon_vertices.size(); // offset
		u_polygon_vertex_counts << vertices.size(); // count

		init_aabb => vec4 aabb;
		for (auto v : vertices) {
			u_polygon_vertices << v;

			// update aabb
			Math.min(aabb.x, v.x) => aabb.x;
			Math.min(aabb.y, v.y) => aabb.y;
			Math.max(aabb.z, v.x) => aabb.z;
			Math.max(aabb.w, v.y) => aabb.w;
		}

		u_polygon_aabb << @(aabb.x, aabb.y, aabb.z, aabb.w);

		u_polygon_transforms << @(
            position.x, position.y, // position
            Math.cos(rotation_radians), Math.sin(rotation_radians) // rotation
        );

		u_polygon_radius << radius;

		u_polygon_colors << @(color.r, color.g, color.b, 1.0);
	}

	fun void update() {
		if (u_polygon_aabb.size() == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// upload
		{ // b2 solid polygon
			solid_polygon_material.storageBuffer(0, u_polygon_vertex_counts);
			solid_polygon_material.storageBuffer(1, u_polygon_vertices);
			solid_polygon_material.storageBuffer(2, u_polygon_transforms);
			solid_polygon_material.storageBuffer(3, u_polygon_colors);
			solid_polygon_material.storageBuffer(4, u_polygon_aabb);
			solid_polygon_material.storageBuffer(5, u_polygon_radius);

			// update geometry vertices (6 vertices per polygon plane)
			solid_polygon_geo.vertexCount(6 * u_polygon_radius.size());
		}

		// zero
		{ // b2 solid polygon
			u_polygon_vertex_counts.clear();
			u_polygon_vertices.clear();
			u_polygon_transforms.clear();
			u_polygon_colors.clear();
			u_polygon_aabb.clear();
			u_polygon_radius.clear();
			0 => num_solid_polygons;
		}
	}
}

public class G2D_Sprite
{
	me.dir() + "./g2d_sprite_shader.wgsl" @=> string shader_path;

	// set drawing shader
	ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	null @=> shader_desc.vertexLayout; 

	// material shader (draws all line segments in 1 draw call)
	Shader shader(shader_desc);
	Material material;
	material.shader(shader);

	// default bindings
	float empty_float_arr[4];
	initStorageBuffers();

	// vertex buffers
	vec2 u_positions[0];
	vec2 u_scales[0];
	float u_rotations[0];
	vec3 u_colors[0];

	Geometry geo;
	geo.vertexCount(0);
	GMesh mesh(geo, material);

	fun void initStorageBuffers() {
		material.storageBuffer(0, empty_float_arr);
		material.storageBuffer(1, empty_float_arr);
		material.storageBuffer(2, empty_float_arr);
		material.storageBuffer(3, empty_float_arr);
	}

    fun void update() {
		if (u_rotations.size() == 0) {
            mesh --< GG.scene();
            <<< "waehawe" >>>;
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// update GPU vertex buffers
        material.storageBuffer(0, u_positions);
        material.storageBuffer(1, u_scales);
        material.storageBuffer(2, u_rotations);
        material.storageBuffer(3, u_colors);
		geo.vertexCount(6 * u_rotations.size());

		// reset
        u_positions.clear();
        u_scales.clear();
        u_rotations.clear();
        u_colors.clear();
    }

}


public class G2D_Capsule
{	
	me.dir() + "./g2d_capsule_shader.wgsl" @=> string shader_path;

	// set drawing shader
	ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	null @=> shader_desc.vertexLayout; 

	// material shader (draws all line segments in 1 draw call)
	Shader shader(shader_desc);
	Material material;
	material.shader(shader);

	// default bindings
	float empty_float_arr[4];
	initStorageBuffers();

	// vertex buffers
	vec2 u_positions[0];
	float u_radius[0];
	vec4 u_colors[0];

	Geometry geo;
	geo.vertexCount(0);
	GMesh mesh(geo, material);

	fun void initStorageBuffers() {
		material.storageBuffer(0, empty_float_arr); // p1, p2
		material.storageBuffer(1, empty_float_arr); // radius
		material.storageBuffer(2, empty_float_arr); // color
		antialias(true); // default antialias true
	}

	fun void antialias(int a) {
		material.uniformInt(3, a);
	}

	fun void capsule(vec2 p1, vec2 p2, float radius, vec4 color) {
		u_positions << p1 << p2;
		u_radius << radius;
		u_colors << color;
	}

    fun void update() {
		if (u_radius.size() == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// update GPU vertex buffers
		material.storageBuffer(0, u_positions); // p1, p2
		material.storageBuffer(1, u_radius); // radius
		material.storageBuffer(2, u_colors); // color
		geo.vertexCount(6 * u_radius.size());

		// reset
        u_positions.clear();
        u_radius.clear();
        u_colors.clear();
    }
}



G2D g2d;

fun void test() {
    while (1) {
        GG.nextFrame() => now;

        // g2d.dashed(@(-1,1), @(1,-1), Color.WHITE, .2);
        // g2d.circle(@(0,0), 1.0, Math.fabs(Math.sin(now/second)), Color.GREEN);
        // g2d.circle(@(0,0), 0.5, Math.fabs(Math.sin(.7 * (now/second))), Color.RED);
        // g2d.circle(@(0,0), 0.7, Math.fabs(Math.sin(.5 * (now/second))), Color.WHITE);
        // g2d.boxFilled(@(0,0), @(1, -1), Color.BLUE);
        // g2d.boxFilled(@(-1,0), @(0, -1), Color.PINK);

        g2d.capsuleFilled(@(-1,1), @(1,-1), Math.fabs(Math.sin(now/second)), Color.WHITE);

        g2d.update();
    }
}

test();