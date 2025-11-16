@import "../g2d/ChuGL.chug"
@import "b2DebugDraw.ck"

GOrbitCamera camera --> GG.scene();
GG.scene().camera(camera);
camera.posZ(50);

GText desc --> GG.scene();
desc.text("press <1,2,3,4> to spawn physics bodies");

DebugDraw debug_draw;
true => debug_draw.drawShapes;
true => debug_draw.drawBounds; // calls drawPolygon, not drawSegment

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
			Math.random2f(0, 1) // radius
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

false => debug_draw.outlines_only;

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