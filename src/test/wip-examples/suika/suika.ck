/*
=== b2 overview ===
Body: transform, body type (static, kinematic, dynamic)

ShapeDef: collision parameters (friction, density, filter)

Polygon/circle/capsule: geometry data

ShapeDef is analgous to a physics "Material", only instead of data for
rendering, it contains data for physics simulation.

a (shapeDef, geometry) pair is bound to a body via createPolygonShape(),
which actually creates a shape_id and shape object.

b2World > b2Body > b2shape
            |         |
         body_def     |
                  shape_def and geometry

==optimization== 
I believe b2 copies all the data you pass to its construction fns,
so should only need 1 body_def, shape_def, and geometry for every *kind*
of entity, rather than a new definition structs/geometries for every single entity

Fruit Diameters
watermelon: 224  +34  x8
melon: 190       +37  x6.7
pineapple: 153   +17  x5.5
peach: 136       +24  x4.9
cante: 112       +17  x4
apple: 95        +19  x3.4
tomato: 76       +16  x2.7
orange: 60       +14  x2.14
grap: 46         +10  x1.64
strawberry: 36   +8   x1.29
cherry: 28

Ae^(kt) = 8

https://github.com/a327ex/emoji-merge?tab=readme-ov-file#arenamerge_emojis
value_to_emoji_data = {
    [1] = {emoji = 'slight_smile', rs = 9, score = 1, mass_multiplier = 8, stars = 2, spawner_offset = vec2(0, 18)},
    [2] = {emoji = 'blush', rs = 11.5, score = 3, mass_multiplier = 6, stars = 2, spawner_offset = vec2(0, 20)},
    [3] = {emoji = 'thinking', rs = 16.5, score = 6, mass_multiplier = 4, stars = 3, spawner_offset = vec2(0, 25)},
    [4] = {emoji = 'devil', rs = 18.5, score = 10, mass_multiplier = 2, stars = 3, spawner_offset = vec2(0, 27)},
    [5] = {emoji = 'angry', rs = 23, score = 15, mass_multiplier = 1, stars = 4, spawner_offset = vec2(0, 32)},
    [6] = {emoji = 'relieved', rs = 29.5, score = 21, mass_multiplier = 1, stars = 4},
    [7] = {emoji = 'yum', rs = 35, score = 28, mass_multiplier = 1, stars = 5},
    [8] = {emoji = 'joy', rs = 41.5, score = 36, mass_multiplier = 1, stars = 6},
    [9] = {emoji = 'sob', rs = 47.5, score = 45, mass_multiplier = 0.5, stars = 8},
    [10] = {emoji = 'smirk', rs = 59, score = 56, mass_multiplier = 0.5, stars = 12},
    [11] = {emoji = 'sunglasses', rs = 70, score = 66, mass_multiplier = 0.25, stars = 24},
}
the factor of density between smallest and largest is 8 / .25 = 32,
whereas difference in area is 8^2 = 64

*/

@import "../b2.ck"

GG.camera().orthographic();
GG.camera().viewSize(10); // viewport height is 10 meters
GWindow.mouseMode(GWindow.MouseMode_Disabled);

// game state enum
0 => int GameState_Start;
1 => int GameState_GameOver;

// config constants
class Config {
    // bounding container
    13.5 / 11.5 => float CONTAINER_RATIO; // ratio of height : width
    3 => float CONTAINER_HW;
    CONTAINER_RATIO * CONTAINER_HW => float CONTAINER_HH;
    .1 => float CONTAINER_THICKNESS;
    CONTAINER_HH => float GAME_OVER_HEIGHT;

    // mergeable ball sizes
    CONTAINER_HW / 16.0 => float BASE_BALL_RADIUS; // smallest ball is 1/16 the container
    1 => float BASE_BALL_DENSITY;

    1::second => dur CHECK_GAMEOVER_COOLDOWN; // only check for gameover state this much time after last merge

} Config conf;

// global gamestate
class GS {
    GameState_Start => int gamestate;

    // drop logic
    @(0, conf.CONTAINER_HH + .5) => vec2 drop_pos;
    int drop_waiting_on;  // the b2bodyid of the mergeable we last dropped and are waiting to fall

    // event times
    time last_merge_time;

    int score;
    int high_score;
    GText score_text --> GG.scene();
    GText game_over_text;
    GText retry_text;

    int b2_world_id;

    Entity entities[0];
    HashMap entity_map; // b2_body_id --> Entity
} GS gs;

{ // init gamestate
    // gs.score_text.pos(@(-1.2 * conf.CONTAINER_HW, conf.CONTAINER_HH));
    gs.score_text.pos(@(0, conf.CONTAINER_HH));
    gs.score_text.sca(.5);

    gs.game_over_text.sca(.5);
    gs.game_over_text.text("GAME OVER");

    gs.retry_text.sca(0.5);
    gs.retry_text.translateY(-1);
    gs.retry_text.text("PRESS <space> TO RETRY");
}

fun void enterGamestate(int state) {
    if (state == gs.gamestate) return;

    { // leaving logic
    }

    state => gs.gamestate;

    { // entering logic
        if (state == GameState_GameOver) {
            gs.game_over_text --> GG.scene();
            gs.retry_text --> GG.scene();
        }
        else if (state  == GameState_Start) {
            // TODO animate the destruction of existing mergeables
            Math.max(gs.score, gs.high_score) => gs.high_score;
            0 => gs.score;

            // delete all existing entities
            for (auto e : gs.entities) {
                if (b2Body.isValid(e.b2_body_id)) {
                    // b2.destroyBody(e.b2_body_id); // TODO crashes
                }
            }

            // clear LUT
            gs.entity_map.clear();

            // remove text
            gs.game_over_text.detach();
            gs.retry_text.detach();
        }
    }
}

MergeType merge_types[0];
class MergeType
{
    float radius;
    int   score;
    float density;

    fun static void add(float r, int s, float d) {
        MergeType m;
        r => m.radius;
        s => m.score;
        d => m.density;
        merge_types << m;
    }
}

//            radius    score    density    id
MergeType.add(9.0/9.0,  1,       8);        // 0
MergeType.add(11.5/9.0, 3,       6);        // 1
MergeType.add(16.5/9.0, 6,       4);        // 2
MergeType.add(18.5/9.0, 10,      2);        // 3
MergeType.add(23.0/9.0, 15,      1);        // 4
MergeType.add(29.5/9.0, 21,      1);        // 5
MergeType.add(35.0/9.0, 28,      1);        // 6
MergeType.add(41.5/9.0, 36,      1);        // 7
MergeType.add(47.5/9.0, 45,      0.5);      // 8
MergeType.add(59.0/9.0, 56,      0.5);      // 9
MergeType.add(72.0/9.0, 66,      0.25);     // 10


DebugDraw debug_draw;
true => debug_draw.drawShapes;
// true => debug_draw.drawAABBs; // calls drawPolygon, not drawSegment

// b2 world creation
b2WorldDef world_def;
// false => world_def.enableSleep;  // disable sleep on entire world so suika balls always trigger collisions
// .05 => world_def.hitEventThreshold;
b2.createWorld(world_def) => gs.b2_world_id;
{ // simulation config (eventually group these into single function/command/struct)
    b2.world(gs.b2_world_id);
    // b2.substeps(1);
}

// b2 static bodies
fun void b2CreateStaticBody(vec2 vertices[]) {
    // body def
    b2BodyDef static_body_def;
    b2BodyType.staticBody => static_body_def.type;
    @(0.0, 0.0) => static_body_def.position;
    b2.createBody(gs.b2_world_id, static_body_def) => int body_id;

    // shape def
    b2ShapeDef shape_def;

    // geometry
    b2.makePolygon(vertices, 0.0) @=> b2Polygon polygon;

    // shape
    b2.createPolygonShape(body_id, shape_def, polygon);
}

b2CreateStaticBody([
    @(-conf.CONTAINER_HW, conf.CONTAINER_HH),
    @(-conf.CONTAINER_HW - conf.CONTAINER_THICKNESS, conf.CONTAINER_HH),
    @(-conf.CONTAINER_HW - conf.CONTAINER_THICKNESS, -conf.CONTAINER_HH),
    @(-conf.CONTAINER_HW, -conf.CONTAINER_HH),
]);

b2CreateStaticBody([
    @(conf.CONTAINER_HW + conf.CONTAINER_THICKNESS, conf.CONTAINER_HH),
    @(conf.CONTAINER_HW, conf.CONTAINER_HH),
    @(conf.CONTAINER_HW, -conf.CONTAINER_HH),
    @(conf.CONTAINER_HW + conf.CONTAINER_THICKNESS, -conf.CONTAINER_HH),
]);

b2CreateStaticBody([
    @(conf.CONTAINER_HW + conf.CONTAINER_THICKNESS, -conf.CONTAINER_HH),
    @(-conf.CONTAINER_HW - conf.CONTAINER_THICKNESS, -conf.CONTAINER_HH),
    @(-conf.CONTAINER_HW - conf.CONTAINER_THICKNESS, -conf.CONTAINER_HH - conf.CONTAINER_THICKNESS),
    @(conf.CONTAINER_HW + conf.CONTAINER_THICKNESS, -conf.CONTAINER_HH - conf.CONTAINER_THICKNESS),
]);

class Entity
{
    // mergable fields
    int pl_type;
    int b2_body_id;
    float bounding_radius;

    fun @construct(int pl_type, int b2_body_id, float radius) {
        pl_type => this.pl_type;
        b2_body_id => this.b2_body_id;
        radius => this.bounding_radius;
    }
}



fun Entity addBody(int which, vec2 pos)
{
    if (which >= merge_types.size()) return null; // can't merge 2 watermelons


    merge_types[which] @=> MergeType mt;

    // body def
    b2BodyDef dynamic_body_def;
    b2BodyType.dynamicBody => dynamic_body_def.type;
    pos => dynamic_body_def.position;
    .5 * Math.random2f(-Math.pi,Math.pi) => dynamic_body_def.angularVelocity; // initial roll


    // @(Math.random2f(-4.0, 4.0), Math.random2f(6.0, 12.0)) => dynamic_body_def.position;
    // Math.random2f(0.0,Math.two_pi) => float angle;
    // @(Math.cos(angle), Math.sin(angle)) => dynamic_body_def.rotation;
    b2.createBody(gs.b2_world_id, dynamic_body_def) => int dynamic_body_id;


    // geometry 

    { // old radius and density calculations (not table driven)
        // r1 ^ 2 / r0 ^ 2 = 2
        // conf.CONTAINER_HW / 1.86 => float max_radius; // largest fruit is 1
        // max_radius / 8.6 => float A; // radius of fruit 0 is 1/8.6 the radius of largest fruit
        // Math.log(max_radius / A) / 10.0 => float k;
        // A * Math.exp(k * which) => float radius;

        // meanwhile the density is inversely proportional
        // e.g. the smallest fruit is 1/N the radius of the largest, but Nx as dense 
        // Math.pow(Math.exp(k * (10 - which)), 1.6) => dynamic_shape_def.density;
    }

    // shape def
	b2ShapeDef dynamic_shape_def;
	true => dynamic_shape_def.enableHitEvents;
    true => dynamic_shape_def.enableContactEvents;
    conf.BASE_BALL_DENSITY * mt.density => dynamic_shape_def.density;
    0.2 => dynamic_shape_def.restitution;

    // shape
    conf.BASE_BALL_RADIUS * mt.radius => float radius;
    b2Circle circle(radius);
    b2.createCircleShape(dynamic_body_id, dynamic_shape_def, circle);

    <<< which, "density: ", dynamic_shape_def.density, "| radius: ", circle.radius >>>;

    // save
    gs.entities << new Entity(which, dynamic_body_id, radius);
    gs.entity_map.set(dynamic_body_id, gs.entities[-1]);

	// if (which == 0) {
	// 	b2.makeBox(1.0f, 1.0f) @=> b2Polygon dynamic_box;
	// 	b2.createPolygonShape(dynamic_body_id, dynamic_shape_def, dynamic_box);
	// } else if (which == 1) {
	// 	b2.makePolygon(
	// 		[ // hexagon
	// 			@(-1, 0),
	// 			@(-.5, Math.sqrt(3)/2),
	// 			@(.5, Math.sqrt(3)/2),
	// 			@(1, 0),
	// 			@(.5, -Math.sqrt(3)/2),
	// 			@(-.5, -Math.sqrt(3)/2),
	// 		],
	// 		0.0
	// 	) @=> b2Polygon dynamic_polygon;
	// 	b2.createPolygonShape(dynamic_body_id, dynamic_shape_def, dynamic_polygon);
	// } else if (which == 2) {
	// 	b2Circle circle(Math.random2f(0.3, 0.7));
	// 	b2.createCircleShape(dynamic_body_id, dynamic_shape_def, circle);
	// } else if (which == 3) {
	// 	b2Capsule capsule(@(0.0, 0.0), @(0.0, 1.0), Math.random2f(0.2, .7));
	// 	b2.createCapsuleShape(dynamic_body_id, dynamic_shape_def, capsule);
	// }

    return gs.entities[-1];
}

fun void dropBody(int which) {
    addBody(which, gs.drop_pos) @=> Entity e;
    if (e != null) e.b2_body_id => gs.drop_waiting_on;
}

b2BodyMoveEvent move_events[0];

int begin_touch_events[0];
int end_touch_events[0];
b2ContactHitEvent hit_events[0];

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    { // input 
        // update drop pos
        GWindow.mouseDeltaPos().x => float mouse_dx;
        mouse_dx * dt +=> gs.drop_pos.x;
        Math.clampf(gs.drop_pos.x, .8 * -conf.CONTAINER_HW, .8 * conf.CONTAINER_HW) => gs.drop_pos.x;

        // toggle mouse mode
        if (GWindow.keyDown(GWindow.Key_Tab)) {
            if (GWindow.mouseMode() == GWindow.MouseMode_Disabled) GWindow.mouseMode(GWindow.MouseMode_Normal);
            else GWindow.mouseMode(GWindow.MouseMode_Disabled);

            <<< "current mouse mode", GWindow.mouseMode() >>>;
        }

        if (GWindow.mouseLeftDown() && gs.drop_waiting_on == 0) {
            dropBody(0);
        }

        if (gs.gamestate == GameState_GameOver) {
            // space to retry
            if (GWindow.keyDown(GWindow.Key_Space)) {
                enterGamestate(GameState_Start);
            }
        }
    }

    { // update
        if (gs.gamestate == GameState_Start) {
            b2World.contactEvents(gs.b2_world_id, begin_touch_events, end_touch_events, hit_events);
            // for (int i; i < hit_events.size(); i++) {
            //     hit_events[i] @=> b2ContactHitEvent @ hit_event;
            //     <<< "hit:", hit_event.shapeIdA, hit_event.shapeIdB, hit_event.point, hit_event.normal, hit_event.approachSpeed >>>;
            // }
            for (int i; i < begin_touch_events.size(); 2 +=> i) {
                begin_touch_events[i] => int id0;
                begin_touch_events[i+1] => int id1;

                // reset drop cooldown after collision
                if (id0 == gs.drop_waiting_on || id1 == gs.drop_waiting_on) {
                    0 => gs.drop_waiting_on; // our last drop has collided
                }

                // early out on invalid
                if (!b2Body.isValid(id0) || !b2Body.isValid(id1)) continue;

                // shortcut so we don't have to use a LUT on body_id --> pl_type
                // assume if mass is the same, they are the same body type
                Math.equal(b2Body.mass(id0), b2Body.mass(id1)) => int same_type;
                if (same_type) {
                    // TODO improve merge 
                    
                    // log merge time
                    now => gs.last_merge_time;

                    // get positions
                    b2Body.position(id0) => vec2 pos0;
                    b2Body.position(id1) => vec2 pos1;

                    // get entities
                    gs.entity_map.getObj(id0) $ Entity @=> Entity e0;
                    gs.entity_map.getObj(id1) $ Entity @=> Entity e1;
                    T.assert(e0 != null, "entity not found for b2_body_id: " + id0);
                    T.assert(e1 != null, "entity not found for b2_body_id: " + id1);
                    T.assert(e0.pl_type == e1.pl_type, "merge on different types");

                    // spawn new 
                    addBody(e0.pl_type + 1, 0.5 * (pos0 + pos1));

                    // add score
                    merge_types[e0.pl_type].score +=> gs.score;

                    // destroy old 
                    // (Removed from gs.entities array in gameover check)
                    // TODO: add hashmap_iter, can do away with array (returns HashmapItem which is a key : value struct)
                    b2.destroyBody(id0);
                    b2.destroyBody(id1);
                    gs.entity_map.del(id0);
                    gs.entity_map.del(id1);
                }
            }

            // check gameover condition
            now - gs.last_merge_time > conf.CHECK_GAMEOVER_COOLDOWN => int no_recent_merges;
            if (no_recent_merges) {
                // ==optimize== conslidate this loop over all entities with draw loop
                // so we don't loop over array twice
                for (int i; i < gs.entities.size(); i++) {
                    gs.entities[i] @=> Entity e;

                    if (!b2Body.isValid(e.b2_body_id)) {
                        // entity deleted during merge, remove and reprocess
                        // ==optimize==: use pool and swap instead of pop out
                        gs.entities.popOut(i);
                        i--;
                        continue;
                    }

                    // ignore checking the ball we just dropped
                    if (gs.drop_waiting_on == e.b2_body_id) continue;

                    if (b2Body.position(e.b2_body_id).y > conf.GAME_OVER_HEIGHT) {
                        enterGamestate(GameState_GameOver);
                        break;
                    }
                }
            }
        }
    }

    { // draw

        // draw circle at drop pos
        debug_draw.drawCircle(gs.drop_pos, .1, Color.RED);

        // score
        gs.score_text.text("Score: " + gs.score);
        

    }

	b2World.draw(gs.b2_world_id, debug_draw);
	debug_draw.update();

    { // developer console
        UI.begin("DEV CONSOLE");

        UI.separatorText("Developer Console");

        for (int i; i < 11; i++) {
            if (UI.button(i + "")) dropBody(i);
        }

        UI.end();
    }
}