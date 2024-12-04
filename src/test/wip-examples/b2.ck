/*
Demo ideas:
- get collision data, play "doh" for each collision
- add a "gravity" slider
- on mouse click, create explosion at position
- on collision, tween albedo to red
- orthographic camera

Plumbing
- multithreading physics sim
- fix timestep
- add option for #simulation steps per frame

------

// workflow WITHOUT shreds / Chuck_Events

b2World world;

class b2ContactEvents {
    b2ContactBeginTouchEvent begin_touch_events[];
    b2ContactEndTouchEvent end_touch_events[];
    b2ContactHitEvent hit_events[];
}


class b2ContactHitEvent {
	b2Shape shapeIdA;
	b2Shape shapeIdB;

	// Point where the shapes hit
	vec2 point;

	// Normal vector pointing from shape A to shape B
	vec2 normal;

	// The speed the shapes are approaching. Always positive. Typically in meters per second.
	float approachSpeed;
}

while (1) {
    GG.nextFrame() => now;
    world.contactEvents() @=> b2ContactEvents @ events; // TODO call something other than event
    for (auto hit : events.hitEvents()) {
        hit.shapIdA().body() @=> b2Body @ body_a;
        hit.shapIdB().body() @=> b2Body @ body_b;
        // do something with the bodies
    }
}

b2API b2ContactEvents b2World_GetContactEvents(b2WorldId worldId);

--------

// workflow WITH shreds / Chuck_Events
// need 1 shred PER shape, also somehow need to group all contact events together to broadcast to a single shred
// less efficient and more complex

--------

b2Body / b2World / etc are just IDS, not actual object pointers.
As a result, they don't need constructors / destructors to manage memory.
Furthermore, ChuGL doesn't need to worry about refcount / garbage collection.
Under the hood, box2d already handles memory efficiently and recycles memory pools.

create via b2XXX.createYYY()
destroy via b2YYY.destroy()
The ChuGL programmer is therefore responsible for their own physics memory management.

refcounting / destructors wouldn't work anyways, because technically any world can 
always have a reference to any body, and vice versa. So it's a circular reference.

"Box2D allows you to avoid destroying bodies by destroying the world directly using b2DestroyWorld(), 
which does all the cleanup work for you. However, you should be mindful to nullify body ids that you 
keep in your application."
- so box2d already handles memory references for you, just explicitly called b2World.destroy()

Is Box2D threadsafe?
- No. Box2D will likely never be thread-safe. Box2D has a large API and trying to make such an API thread-safe would have a large performance and complexity impact. However, you can call read only functions from multiple threads. For example, all the spatial query functions are read only.
- so like with imgui, add validation on write functions: error if calling shred is not a registered graphics shred


struct b2BodyDef
{
	/// The body type: static, kinematic, or dynamic.
	b2BodyType type;

	/// The initial world position of the body. Bodies should be created with the desired position.
	/// @note Creating bodies at the origin and then moving them nearly doubles the cost of body creation, especially
	///	if the body is moved after shapes have been added.
	b2Vec2 position;

	/// The initial world rotation of the body. Use b2MakeRot() if you have an angle.
	b2Rot rotation;

	/// The initial linear velocity of the body's origin. Typically in meters per second.
	b2Vec2 linearVelocity;

	/// The initial angular velocity of the body. Radians per second.
	float angularVelocity;

	/// Linear damping is use to reduce the linear velocity. The damping parameter
	/// can be larger than 1 but the damping effect becomes sensitive to the
	/// time step when the damping parameter is large.
	///	Generally linear damping is undesirable because it makes objects move slowly
	///	as if they are floating.
	float linearDamping;

	/// Angular damping is use to reduce the angular velocity. The damping parameter
	/// can be larger than 1.0f but the damping effect becomes sensitive to the
	/// time step when the damping parameter is large.
	///	Angular damping can be use slow down rotating bodies.
	float angularDamping;

	/// Scale the gravity applied to this body. Non-dimensional.
	float gravityScale;

	/// Sleep velocity threshold, default is 0.05 meter per second
	float sleepThreshold;

	/// Use this to store application specific body data.
	void* userData;

	/// Set this flag to false if this body should never fall asleep.
	bool enableSleep;

	/// Is this body initially awake or sleeping?
	bool isAwake;

	/// Should this body be prevented from rotating? Useful for characters.
	bool fixedRotation;

	/// Treat this body as high speed object that performs continuous collision detection
	/// against dynamic and kinematic bodies, but not other bullet bodies.
	///	@warning Bullets should be used sparingly. They are not a solution for general dynamic-versus-dynamic
	///	continuous collision. They may interfere with joint constraints.
	bool isBullet;
} b2BodyDef;

typedef struct b2ShapeDef
{
	/// The Coulomb (dry) friction coefficient, usually in the range [0,1].
	float friction;

	/// The restitution (bounce) usually in the range [0,1].
	float restitution;

	/// The density, usually in kg/m^2.
	float density;

	/// Collision filtering data.
	b2Filter filter;

	/// Custom debug draw color.
	uint32_t customColor;

	/// A sensor shape generates overlap events but never generates a collision response.
	bool isSensor;
} b2ShapeDef;


Body: transform

ShapeDef:  collision parameters (friction, density, filter)
Polygon/circle/capsule: geometry data

ShapeDef is analgous to a physics "Material", only instead of data for
rendering, it contains data for physics simulation.

a (shapeDef, geometry) pair is bound to a body via CreatePolygonShape(),
which actually creates a shape_id and shape object.

b2World > b2Body > b2shape


*/


/*
TODO 11/23
- after 30 shapes despawn the oldest
- add sokol timer to ChuGL API
- profile debug draw on audio side

- add immediate-mode b2_world.step(int substeps, int worker_threads, float dt) command and simpler variants
*/

GOrbitCamera camera --> GG.scene();
GG.scene().camera(camera);
camera.posZ(50);

public class b2DebugDraw_SolidPolygon extends GGen
{
	me.dir() + "./b2_solid_polygon_shader.wgsl" @=> string b2_solid_polygon_shader_path;

	// set drawing shader
	ShaderDesc b2_solid_polygon_shader_desc;
	b2_solid_polygon_shader_path => b2_solid_polygon_shader_desc.vertexPath;
	b2_solid_polygon_shader_path => b2_solid_polygon_shader_desc.fragmentPath;
	null => b2_solid_polygon_shader_desc.vertexLayout; // no vertex layout

	// material shader (draws all solid polygons in 1 draw call)
	Shader b2_solid_polygon_shader(b2_solid_polygon_shader_desc);
	Material b2_solid_polygon_material;
	b2_solid_polygon_material.shader(b2_solid_polygon_shader);

	// storage buffers
	int u_polygon_vertex_counts[0];
	float u_polygon_vertices[0];
	float u_polygon_transforms[0];
	float u_polygon_colors[0];
	float u_polygon_aabb[0];
	float u_polygon_radius[0];
	int num_solid_polygons;

	// initialize material uniforms
	// TODO: binding empty storage buffer crashes wgpu
	int empty_int_arr[1];
	float empty_float_arr[1];
	initStorageBuffers();

	Geometry b2_solid_polygon_geo; // just used to set vertex count
	GMesh b2_solid_polygon_mesh(b2_solid_polygon_geo, b2_solid_polygon_material) --> this;

	// set trillion to be the initial aabb (flt max is actualy more like 3.4e38)
	@(1000000000, 1000000000, -1000000000, -1000000000) => vec4 init_aabb;

	fun void initStorageBuffers() {
		b2_solid_polygon_material.storageBuffer(0, empty_int_arr);
		b2_solid_polygon_material.storageBuffer(1, empty_float_arr);
		b2_solid_polygon_material.storageBuffer(2, empty_float_arr);
		b2_solid_polygon_material.storageBuffer(3, empty_float_arr);
		b2_solid_polygon_material.storageBuffer(4, empty_float_arr);
		b2_solid_polygon_material.storageBuffer(5, empty_float_arr);
	}

	fun void drawSolidPolygon(
		vec2 position,
		float rotation_radians,
		vec2 vertices[], 
		float radius,
		vec3 color
	) {
		num_solid_polygons++;

		u_polygon_vertex_counts << u_polygon_vertices.size() / 2; // offset
		u_polygon_vertex_counts << vertices.size(); // count

		init_aabb => vec4 aabb;
		for (auto v : vertices) {
			u_polygon_vertices << v.x;
			u_polygon_vertices << v.y;

			// update aabb
			Math.min(aabb.x, v.x) => aabb.x;
			Math.min(aabb.y, v.y) => aabb.y;
			Math.max(aabb.z, v.x) => aabb.z;
			Math.max(aabb.w, v.y) => aabb.w;
		}

		u_polygon_aabb << aabb.x << aabb.y << aabb.z << aabb.w;

		u_polygon_transforms << position.x << position.y; // position
		u_polygon_transforms << Math.cos(rotation_radians) << Math.sin(rotation_radians); // rotation

		u_polygon_radius << radius;

		u_polygon_colors << color.r << color.g << color.b;
		u_polygon_colors << 1.0;
	}

	fun void drawBox(
		vec2 position,
		float rotation_radians,
		float width,
		float height,
		vec3 color
	) {
		.5 * width => float hw;
		.5 * height => float hh;
		this.drawSolidPolygon(
			position,
			rotation_radians,
			[@(-hw, hh), @(-hw, -hh), @(hw, -hh), @(hw, hh)], 
			0,
			color	
		);
	}

	fun void drawSquare(
		vec2 position,
		float rotation_radians,
		float l,
		vec3 color
	) {
		this.drawBox(position, rotation_radians, l, l, color);
	}

	fun void update() {
		if (num_solid_polygons == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// upload
		{ // b2 solid polygon
			b2_solid_polygon_material.storageBuffer(0, u_polygon_vertex_counts);
			b2_solid_polygon_material.storageBuffer(1, u_polygon_vertices);
			b2_solid_polygon_material.storageBuffer(2, u_polygon_transforms);
			b2_solid_polygon_material.storageBuffer(3, u_polygon_colors);
			b2_solid_polygon_material.storageBuffer(4, u_polygon_aabb);
			b2_solid_polygon_material.storageBuffer(5, u_polygon_radius);

			// update geometry vertices (6 vertices per polygon plane)
			b2_solid_polygon_geo.vertexCount(6 * num_solid_polygons);
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

// batch draws simple line segments (no width)
public class b2DebugDraw_Lines extends GGen
{
	me.dir() + "./b2_lines_shader.wgsl" @=> string shader_path;

	// set drawing shader
	ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	[VertexFormat.Float2] @=> shader_desc.vertexLayout; // no vertex layout

	// material shader (draws all line segments in 1 draw call)
	Shader shader(shader_desc);
	<<< "lines shader id", shader.id() >>>;
	Material material;
	material.shader(shader);
	material.topology(Material.Topology_LineList);

	// vertex buffers
	vec2 u_positions[0];

	Geometry geo; // just used to set vertex count
	geo.vertexCount(0);
	GMesh mesh(geo, material) --> this;

	fun void drawSegment( vec2 p1, vec2 p2) {
		u_positions << p1 << p2;
	}

	fun void update()
	{
		// update GPU vertex buffers
		geo.vertexAttribute(0, u_positions);
		geo.vertexCount(u_positions.size());

		// reset
		u_positions.clear();
	}
}

// batch draws solid and wireframe circles
public class b2DebugDraw_Circles extends GGen
{
	me.dir() + "./b2_circle_shader.wgsl" @=> string shader_path;

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
	vec4 u_center_radius_thickness[0];
	vec4 u_colors[0];

	Geometry geo;
	geo.vertexCount(0);
	GMesh mesh(geo, material) --> this;

	fun void initStorageBuffers() {
		material.storageBuffer(0, empty_float_arr);
		material.storageBuffer(1, empty_float_arr);
		material.uniformInt(2, 1); // default antialiasing to true
	}

	// set whether to antialias circles
	fun void antialias(int value) {
		material.uniformInt(2, value);
	}

	fun void drawCircle( vec2 center, float radius, float thickness, vec3 color) {
		u_center_radius_thickness << @(center.x, center.y, radius, thickness);
		u_colors << @(color.r, color.g, color.b, 1.0);
	}

	fun void update()
	{
		if (u_center_radius_thickness.size() == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// update GPU vertex buffers
		// geo.vertexAttribute(0, u_center_radius_thickness);
		// geo.vertexAttribute(1, u_colors);
		material.storageBuffer(0, u_center_radius_thickness);
		material.storageBuffer(1, u_colors);
		geo.vertexCount(6 * u_center_radius_thickness.size());

		// reset
		u_center_radius_thickness.clear();
		u_colors.clear();
	}
}

// batch draws solid capsules
public class b2DebugDraw_Capsule extends GGen
{
	me.dir() + "./b2_capsule_shader.wgsl" @=> string shader_path;

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
	vec4 u_p1p2[0];
	float u_radius[0];
	vec4 u_colors[0];

	Geometry geo;
	geo.vertexCount(0);
	GMesh mesh(geo, material) --> this;

	fun void initStorageBuffers() {
		material.storageBuffer(0, empty_float_arr);
		material.storageBuffer(1, empty_float_arr);
		material.storageBuffer(2, empty_float_arr);
	}

	fun void drawCapsule( vec2 p1, vec2 p2, float radius, vec3 color) {
		u_p1p2 << @(p1.x, p1.y, p2.x, p2.y);
		u_radius << radius;
		u_colors << @(color.r, color.g, color.b, 1.0);
	}

	fun void update()
	{
		if (u_radius.size() == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// update GPU vertex buffers
		material.storageBuffer(0, u_p1p2);
		material.storageBuffer(1, u_radius);
		material.storageBuffer(2, u_colors);
		geo.vertexCount(6 * u_radius.size());

		// reset
		u_p1p2.clear();
		u_radius.clear();
		u_colors.clear();
	}
}


// custom debug draw
class DebugDraw extends b2DebugDraw
{
	b2DebugDraw_SolidPolygon solid_polygons --> GG.scene();
	b2DebugDraw_Lines lines --> GG.scene();
	b2DebugDraw_Circles circles --> GG.scene();
	b2DebugDraw_Capsule capsules --> GG.scene();

	fun void drawSolidPolygon(
		vec2 position,
		float rotation_radians,
		vec2 vertices[], 
		float radius,
		vec3 color
	) {
		solid_polygons.drawSolidPolygon(position, rotation_radians, vertices, radius, color);
	}

	fun void drawSegment(
		vec2 p1, vec2 p2, vec3 color
	) {
		// color unused
		lines.drawSegment(p1, p2);
	}

	// draws a polygon outline
	fun void drawPolygon(vec2 vertices[], vec3 color) {
		// just draw as individual line segments for now
		for (int i; i < vertices.size() - 1; i++)
			lines.drawSegment(vertices[i], vertices[i+1]);

		// to close the loop
		lines.drawSegment(vertices[-1], vertices[0]);
	}

	fun void drawCircle(vec2 center, float radius, vec3 color) {
		circles.drawCircle(center, radius, .1, color);
	}

	fun void drawSolidCircle(vec2 center, float rotation_radians, float radius, vec3 color) {
		circles.drawCircle(center, radius, 1.0, color);
	}
	
	fun void drawSolidCapsule(vec2 p1, vec2 p2, float radius, vec3 color) {
		capsules.drawCapsule(p1, p2, radius, color);
	}


	// upload all the collected debug draw data to the materials/geometry/GPU
	fun void update() {
		solid_polygons.update();
		lines.update();
		circles.update();
		capsules.update();
	}
}

DebugDraw debug_draw;
true => debug_draw.drawShapes;
true => debug_draw.drawAABBs; // calls drawPolygon, not drawSegment

b2WorldDef world_def;
// .05 => world_def.hitEventThreshold;

b2.createWorld(world_def) => int world_id;

{ // simulation config (eventually group these into single function/command/struct)
    b2.world(world_id);
    // b2.substeps(1);
}

b2BodyDef ground_body_def;
@(0.0, -10.0) => ground_body_def.position;
b2.createBody(world_id, ground_body_def) => int ground_id;
b2ShapeDef ground_shape_def;
true => ground_shape_def.enableHitEvents;
b2.makeBox(50.0, 10.0) @=> b2Polygon ground_box;
b2.createPolygonShape(ground_id, ground_shape_def, ground_box);

// PlaneGeometry plane_geo;
// FlatMaterial mat;
// GMesh@ dynamic_box_meshes[0];
int dynamic_body_ids[0];

fun void addBody(int which)
{
    // create dynamic box
    b2BodyDef dynamic_body_def;
    b2BodyType.dynamicBody => dynamic_body_def.type;
    @(Math.random2f(-4.0, 4.0), Math.random2f(6.0, 12.0)) => dynamic_body_def.position;
    Math.random2f(0.0,Math.two_pi) => float angle;
    @(Math.cos(angle), Math.sin(angle)) => dynamic_body_def.rotation;
    b2.createBody(world_id, dynamic_body_def) => int dynamic_body_id;
    dynamic_body_ids << dynamic_body_id;

    // then shape
	b2ShapeDef dynamic_shape_def;
	true => dynamic_shape_def.enableHitEvents;
	if (which == 0) {
		b2.makeBox(1.0f, 1.0f) @=> b2Polygon dynamic_box;
		b2.createPolygonShape(dynamic_body_id, dynamic_shape_def, dynamic_box);
	} else if (which == 1) {
		b2.makePolygon(
			[ // hexagon
				@(-1, 0),
				@(-.5, Math.sqrt(3)/2),
				@(.5, Math.sqrt(3)/2),
				@(1, 0),
				@(.5, -Math.sqrt(3)/2),
				@(-.5, -Math.sqrt(3)/2),
			],
			0.0
		) @=> b2Polygon dynamic_polygon;
		b2.createPolygonShape(dynamic_body_id, dynamic_shape_def, dynamic_polygon);
	} else if (which == 2) {
		b2Circle circle(Math.random2f(0.3, 0.7));
		b2.createCircleShape(dynamic_body_id, dynamic_shape_def, circle);
	} else if (which == 3) {
		b2Capsule capsule(@(0.0, 0.0), @(0.0, 1.0), Math.random2f(0.2, .7));
		b2.createCapsuleShape(dynamic_body_id, dynamic_shape_def, capsule);
	} else if (which == 4) {

	}
    // matching GGen
    // GMesh dynamic_box_mesh(plane_geo, mat) --> GG.scene();
    // dynamic_box_mesh.scaX(1.0);
    // dynamic_box_mesh.scaY(1.0);
    // dynamic_box_meshes << dynamic_box_mesh;
}

b2BodyMoveEvent move_events[0];

int begin_touch_events[0];
int end_touch_events[0];
b2ContactHitEvent hit_events[0];

while (1) {
    GG.nextFrame() => now;

    if (GWindow.keyDown(GWindow.Key_1)) addBody(0);
    if (GWindow.keyDown(GWindow.Key_2)) addBody(1);
	if (GWindow.keyDown(GWindow.Key_3)) addBody(2);
	if (GWindow.keyDown(GWindow.Key_4)) addBody(3);

	// update box mesh positions
    // for (int i; i < dynamic_body_ids.size(); i++) {
        // b2Body.position(dynamic_body_ids[i]) => vec2 p;
        // dynamic_box_meshes[i].pos(@(p.x, p.y, 0.0));
        // dynamic_box_meshes[i].rotZ(b2Body.angle(dynamic_body_ids[i]));
    // }

    b2World.bodyEvents(world_id, move_events);
    // for (int i; i < move_events.size(); i++) {
    //     move_events[i] @=> b2BodyMoveEvent @ move_event;
    //     if (move_event.fellAsleep)
    //         <<< move_event.bodyId, "fell asleep", b2Body.isValid(move_event.bodyId) >>>;
    //     else
    //         <<< move_event.bodyId, "awake", b2Body.isValid(move_event.bodyId) >>>;
    // }

    b2World.contactEvents(world_id, begin_touch_events, end_touch_events, hit_events);
    for (int i; i < hit_events.size(); i++) {
        hit_events[i] @=> b2ContactHitEvent @ hit_event;
        <<< "hit:", hit_event.shapeIdA, hit_event.shapeIdB, hit_event.point, hit_event.normal, hit_event.approachSpeed >>>;
    }
    // for (int i; i < begin_touch_events.size(); 2 +=> i) {
    //     <<< "begin touch:", begin_touch_events[i], "touched", begin_touch_events[i+1] >>>;
    // }
    // for (int i; i < end_touch_events.size(); 2 +=> i) {
    //     <<< "end touch:", end_touch_events[i], "stopped touching", end_touch_events[i+1] >>>;
    // }

	b2World.draw(world_id, debug_draw);
	debug_draw.update();
}