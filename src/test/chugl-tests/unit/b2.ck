b2_WorldDef world_def;
// world_def.help();
T.assert(world_def.enableSleep == true, "enableSleep default value");
T.assert(world_def.enableContinuous == true, "enableContinuous default value");
T.assert(T.feq(world_def.contactHertz, 30.0), "contactHertz default value");
false => world_def.enableSleep;
T.assert(world_def.enableSleep == false, "enableSleep set to false");

b2.createWorld(world_def) => int world_id;
T.assert(b2_World.isValid(world_id), "world created");
T.assert(!b2_World.isValid(0), "invalid world");
b2.destroyWorld(world_id);
T.assert(!b2_World.isValid(world_id), "destroyed world");

b2_Filter filter;
T.assert(filter.categoryBits == 0x0001, "categoryBits default value");
T.assert(filter.maskBits & 0xFFFFFFFF == 0xFFFFFFFF, "maskBits default value");
T.assert(filter.groupIndex == 0, "groupIndex default value");

b2_ShapeDef shape_def;
T.assert(T.feq(shape_def.density, 1.0), "density default value");
T.assert(shape_def.filter.categoryBits == 0x0001, "filter.categoryBits default value");
T.assert(shape_def.filter.maskBits & 0xFFFFFFFF == 0xFFFFFFFF, "filter.maskBits default value");
T.assert(shape_def.filter.groupIndex == 0, "filter.groupIndex default value");

b2_BodyDef body_def;
@(1337, 2.3) => body_def.position;
T.assert(body_def.type == b2_BodyType.staticBody, "type default value");
T.assert(T.feq(body_def.gravityScale, 1.0), "gravityScale default value");
T.assert(body_def.isAwake, "body is awake by default");

b2.createWorld(world_def) => world_id;
b2.createBody(world_id, body_def) => int body_id;
T.assert(b2_Body.isValid(body_id), "body created");
T.assert(T.feq(1337, b2_Body.position(body_id).x), "position.x set");

b2_Polygon.makeBox(.5, .5) @=> b2_Polygon@ box_poly;
b2_Shape.createPolygonShape(body_id, shape_def, box_poly) => int shape_id;

b2_Shape.filter(shape_id) @=> b2_Filter circle_shape_filter;
T.assert(circle_shape_filter.categoryBits == 0x0001, "filter.categoryBits default value");

b2_Circle circle(@(1,2), 3);
b2_Shape.createCircleShape(body_id, shape_def, circle) => int circle_shape_id;

b2_Segment segment(@(1,2), @(3,4));
b2_Shape.createSegmentShape(body_id, shape_def, segment) => int segment_shape_id;

b2_Capsule capsule(@(1,2), @(3,4), 5);
b2_Shape.createCapsuleShape(body_id, shape_def, capsule) => int capsule_shape_id;

int shape_ids[0];
b2_Body.shapes(body_id, shape_ids);
T.assert(shape_ids.size() == 4, "shape added to body");
for (auto shape_id : shape_ids)
    T.assert(b2_Shape.isValid(shape_id), "shape is valid");

b2.destroyBody(body_id);
T.assert(!b2_Body.isValid(body_id), "destroy body");
T.assert(!b2_Shape.isValid(shape_ids[0]), "destroy shape");

