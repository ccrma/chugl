/*
TODO
- remove radius from b2_polygon shader (simplify overall)

- pickup effects for health and SP

*/

// import materials for 2D drawing
@import "b2.ck"

GG.logLevel(GG.LogLevel_Debug);

GG.camera().orthographic();
GG.camera().viewSize(5);

// set target resolution
// most steam games (over half) default to 1920x1080
// 1920x1080 / 4 = 480x270
GG.renderPass().resolution(480, 270);
GG.hudPass().resolution(480, 270);

// set MSAA
GG.renderPass().msaa(1);

// pixelate the output
TextureSampler output_sampler;
TextureSampler.Filter_Nearest => output_sampler.filterMin;  
TextureSampler.Filter_Nearest => output_sampler.filterMag;  
TextureSampler.Filter_Nearest => output_sampler.filterMip;  
GG.outputPass().sampler(output_sampler);

// TODO: fix the screenpass stuff
// 2D Vector Graphics
b2DebugDraw_Circles circles --> GG.scene();
b2DebugDraw_Lines lines --> GG.scene();
b2DebugDraw_SolidPolygon polygons --> GG.scene();
circles.antialias(false);

// font
GText.defaultFont(me.dir() + "./m5x7.ttf");

// Globals ============================================
class G
{
    static dur dt;
    static float dtf; // equivalent to G.dt, but cast as float
    1.0 => static float dt_rate; // modifier for time stretch
    static dur dt_cum;      // cumulative dt (equiv to time-stretched "now")
    static float dtf_cum;   // cumulative dtf

    static GCamera@ camera;

    static HashMap@ pickup_map; // hashmap from pickup b2bodyid --> pickup type
}

GG.camera() @=> G.camera;
new HashMap @=> G.pickup_map;

// Pickup Type Enums
0 => int PickupType_Basic;
1 => int PickupType_Boost;
2 => int PickupType_Health;
3 => int PickupType_SP;

// pickup type colors
[
    Color.hex(0xFFFFFF),
    Color.hex(0x00ffff),
    Color.hex(0xFF0000),
    Color.hex(0xFFD700),
] @=> vec3 pickup_colors[];

[
    "DEFAULT PICKUP",
    "+BOOST",
    "+HP",
    "+SP",
] @=> string pickup_text[];

.25 => float pickup_scale;


// Math ============================================
class M
{
    fun static vec2 rot2vec(float radians) {
        return @( Math.cos(radians), Math.sin(radians) );
    }

    fun static vec2 randomDir() {
        return rot2vec(Math.random2f(0, Math.two_pi));
    }

    fun static float easeOutQuad(float x) {
        return 1 - (1 - x) * (1 - x);
    }

    fun static float easeInOutCubic(float x) {
        if (x < 0.5) 
            return 4 * x * x * x;
        else 
            return 1 - Math.pow(-2 * x + 2, 3) / 2;
    }

    fun static float lerp(float t, float a, float b) {
        return a + t * (b - a);
    }

    // returns ratio of x in range [a, b]
    fun static float invLerp(float x, float a, float b) {
        return (x - a) / (b - a);
    }

    // rotate dir v by `a` radians
    fun static vec2 rot(vec2 v, float a) {
        Math.cos(a) => float cos_a;
        Math.sin(a) => float sin_a;
        return @(
            cos_a * v.x - sin_a * v.y,
            sin_a * v.x + cos_a * v.y
        );
    }

    fun static vec2 normalize(vec2 n)
    {
        return n / Math.hypot(n.x, n.y); // hypot is the magnitude
    }

    // ---------------
    // random
    // ---------------
    fun static vec2 randomPointInCircle(vec2 center, float min_r, float max_r) {
        Math.random2f(0, Math.two_pi) => float theta;
        Math.random2f(min_r, max_r) => float radius;
        return center + radius * @(Math.cos(theta), Math.sin(theta));
    }


    fun static vec2 randomPointInArea(vec2 center, float w, float h) {
        return center + @(-w * 0.5, -h * 0.5) + @(
            Math.random2f(0, w),
            Math.random2f(0, h)
        );

    }
}

// physics state setup ============================================

// collision category flags (for b2 physics)
(1 << 1) => int Category_Player;      // 2
(1 << 2) => int Category_Projectile;  // 4
(1 << 3) => int Category_Pickup;      // 8

b2WorldDef world_def;
b2.createWorld(world_def) => int world_id;
int begin_touch_events[0]; // for holding collision data
int begin_sensor_events[0]; // for holding sensor data

{ // simulation config (eventually group these into single function/command/struct)
    b2.world(world_id);
    b2.substeps(4);
}

// projectile b2 definitions
b2Filter projectile_filter;
Category_Projectile => projectile_filter.categoryBits;
0 => projectile_filter.maskBits; // collide with nothing

b2ShapeDef projectile_shape_def;
projectile_filter @=> projectile_shape_def.filter;

b2BodyDef projectile_body_def;
b2BodyType.kinematicBody => projectile_body_def.type;

// player ============================================

class Player
{
    // TODO cache and store player rotation / position once per frame rather than recalculating constantly
    int body_id;
    int shape_id;
    float rotation;
    .1 => float radius;

    1.0 => float velocity_scale;
    Color.WHITE => vec3 boost_trail_color;

    fun @construct() {
        // create body def
        // @(Math.random2f(-4.0, 4.0), Math.random2f(6.0, 12.0)) => kinematic_body_def.position;
        // Math.random2f(0.0,Math.two_pi) => float angle;
        // @(Math.cos(angle), Math.sin(angle)) => kinematic_body_def.rotation;
        b2BodyDef player_body_def;
        b2BodyType.dynamicBody => player_body_def.type; // must be dynamic to trigger sensor

        b2.createBody(world_id, player_body_def) => this.body_id;

        // then shape
        // TODO make player collider match the visual
        b2Filter player_filter;
        Category_Player => player_filter.categoryBits;
        Category_Pickup => player_filter.maskBits; // nothing

        b2ShapeDef shape_def;
        player_filter @=> shape_def.filter;

        b2Circle circle(this.radius);
        b2.createCircleShape(this.body_id, shape_def, circle) => this.shape_id;
    }

    fun vec2 pos() {
        return b2Body.position(this.body_id);
    }
}

Player player;

// Effects ==============================================================

/* Effects Architecture
To avoid the rabbit hole / performance sink of an EffectsManager +
Effect base class + overriding/constructors/etc we instead
have all effects be functions that can be sporked and combined
to create larger meta effects.

An effect is: a temporary shred that interpolates value(s) over time
Currently it both
- updates the interpolated value
- applies those values to update state / make draw commands

All the interpolated state in contained within the function body 
of the effect itself, although this may change (add pooling?) 
for performance reasons

So far have observed these different "types" of effects
- only single global instance allowed (e.g. cameraShake
and timeWarp)
    - for these, a single generation counter is sufficient
- any number if instances allowed, no external state for de-duping required
(e.g. bullet boom effect)
- multiple allowed, but each of these is named and must be deduped
within that name. Use a a hashmap<string name, int generation> for this

Rules:
- an effect ought to be sporked from a permanent graphics shred
- an effect *cannot* spork other effects, instead just spork the two
effects separately from the permanent graphics shred (this avoids bug
where the parent effect shred exits before child is finished)

Meta: follow philosophy of change/discipline/constrain your coding
style to fit within the constraints of simplicity and performance,
rather than complexifying the system (e.g. going the OOP/Effects 
command queue route) to accomodate a more flexible but less performant
programming style (e.g. one where effects can spawn other effects)
- to be seen if this is the right move
*/

int camera_shake_generation;
fun void cameraShakeEffect(float amplitude, dur shake_dur, float hz) {
    ++camera_shake_generation => int gen;
    dur elapsed_time;

    // generate shake params
    shake_dur / 1::second => float shake_dur_secs;
    vec2 camera_deltas[(hz * shake_dur_secs + 1) $ int];
    for (int i; i < camera_deltas.size(); i++) {
        @(Math.random2f(-amplitude, amplitude),
        Math.random2f(-amplitude, amplitude)) => camera_deltas[i];
    }
    @(0,0) => camera_deltas[0]; // start from original pos
    @(0,0) => camera_deltas[-2]; // return to original pos
    @(0,0) => camera_deltas[-1]; // return to original pos (yes need this one too)

    (1.0 / hz)::second => dur camera_delta_period; // time for 1 cycle of shake

    while (true) {
        GG.nextFrame() => now;
        // another shake triggred, stop this one
        if (elapsed_time > shake_dur || gen != camera_shake_generation) break;
        // update elapsed time
        G.dt +=> elapsed_time;

        // compute fraction shake progress
        elapsed_time / shake_dur => float progress;
        elapsed_time / camera_delta_period => float elapsed_periods;
        elapsed_periods $ int => int floor;
        elapsed_periods - floor => float fract;

        // clamp to end of camera_deltas
        if (floor + 1 >= camera_deltas.size()) {
            camera_deltas.size() - 2 => floor;
            1.0 => fract;
        }

        // interpolate the progress
        camera_deltas[floor] * (1.0 - fract) + camera_deltas[floor + 1] * fract => vec2 delta;
        // update camera pos with linear decay based on progress
        (1.0 - progress) * delta => G.camera.pos;
    }
}

// shrinking white square on blaster end
fun void shootEffect(dur tween_dur) {
    dur elapsed_time;

    while (elapsed_time < tween_dur) {
        GG.nextFrame() => now;
        G.dt +=> elapsed_time;

        elapsed_time / tween_dur => float t;
        .1 * (1 - t * t) => float shoot_effect_scale;
        polygons.drawSquare(
            player.pos() + player.radius * M.rot2vec(player.rotation),
            player.rotation, // rotation
            shoot_effect_scale,
            Color.WHITE
        );
    }
}

fun void boomEffect(vec2 pos) {
    dur elapsed_time;
    while (elapsed_time < .25::second) {
        GG.nextFrame() => now;
        G.dt +=> elapsed_time;

        if (elapsed_time < .1::second) 
            polygons.drawSquare(
                pos,
                0, // rotation
                .1,
                Color.WHITE
            );
        else 
            polygons.drawSquare(
                pos,
                0, // rotation
                .1,
                Color.RED
            );
    }
}

// slows time to rate, and over d ramps back to normal speed
int slow_effect_generation;
fun void slowEffect(float rate, dur d) {
    ++slow_effect_generation => int gen;
    dur elapsed_time;

    while (gen == slow_effect_generation) {
        GG.nextFrame() => now;
        // must use GG.dt() so it's not affected by it's own time warp!
        GG.dt()::second +=> elapsed_time; 

        if (elapsed_time > d) break;

        M.lerp(M.easeInOutCubic(elapsed_time / d), rate, 1.0) => float t;

        // adjust animation rate
        t => G.dt_rate;
        // adjust physics rate
        t => b2.rate;

        T.assert(G.dt_rate <= 1.0, "dt_rate exceeds bounds");
        if (G.dt_rate > 1.0) {
            <<< "rate",  G.dt_rate, "t", t, "d", d, "rate", rate >>>;
        }
    }

    // restore to normal rate
    if (gen == slow_effect_generation) {
        1.0 => G.dt_rate;
        1.0 => b2.rate;
    }
}

// spawns an explosion of lines going in random directinos that gradually shorten
fun void explodeEffect(vec2 pos, dur max_dur) {
    // params
    Math.random2(8, 16) => int num; // number of lines

    vec2 positions[num];
    vec2 dir[num];
    float lengths[num];
    dur durations[num];
    float velocities[num];

    // init
    for (int i; i < num; i++) {
        pos => positions[i];
        M.randomDir() => dir[i];
        Math.random2f(.1, .2) => lengths[i];
        Math.random2f(.3, max_dur / second)::second => durations[i];
        Math.random2f(.01, .02) => velocities[i];
    }

    dur elapsed_time;
    while (elapsed_time < max_dur) {
        GG.nextFrame() => now;
        G.dt +=> elapsed_time;

        for (int i; i < num; i++) {
            // update line 
            (elapsed_time) / durations[i] => float t;
            // if animation still in progress for this line
            if (t < 1) {
                // update position
                velocities[i] * dir[i] +=> positions[i];
                // shrink lengths linearly down to 0
                lengths[i] * (1 - t) => float len;
                // draw
                lines.drawSegment(positions[i], positions[i] + len * dir[i]);
            }
        }
    }
}

// NOTE: not affected by time warp
int screen_flash_effect_generation;
fun void screenFlashEffect(float hz, dur duration) {
    <<< "screen flash EFfect" >>>;
    ++screen_flash_effect_generation => int gen;

    (1.0/hz)::second => dur period;

    dur elapsed_time;
    int toggle;
    while (elapsed_time < duration && gen == screen_flash_effect_generation) {
        <<< "screen flash", toggle >>>;
        if (toggle) {
            GG.outputPass().exposure(1.0);
            <<< "1 exposure" >>>;
        } else {
            GG.outputPass().exposure(0.0);
            <<< "0 exposure" >>>;
        }
        1 - toggle => toggle;

        // pass time
        period => now;
        period +=> elapsed_time;
    }

    // restore
    if (gen == screen_flash_effect_generation) {
        GG.outputPass().exposure(1.0);
    }
}


int refresh_effect_generation;
// white box that slides up across player (for power up)
fun void refreshEffect() {
    ++refresh_effect_generation => int gen;
    .1::second => dur effect_dur;

    dur elapsed_time;

    player.radius * 4 => float width;

    while (elapsed_time < effect_dur && gen == refresh_effect_generation) {
        GG.nextFrame() => now;
        G.dt +=> elapsed_time;

        (elapsed_time / effect_dur) => float t;

        (width * (1 - t)) => float height;
        (width * 0.5) - height / 2 => float y_offset;

        polygons.drawBox(
            player.pos() + @(0, y_offset),
            0, // rotation
            width,
            height,
            Color.WHITE 
        );
    }

}

// flame trail from ship
// multiple allowed
// gradually shrinking/fading circle
// ==optimize== group all trails into circular buffer and pool, 1 shred
fun void boostTrailEffect(vec2 pos, vec3 color, float radius_scale) {
    radius_scale * Math.random2f(.04, .046) => float init_radius;
    init_radius * 8 * 1::second => dur effect_dur; // time to dissolve

    Math.random2f(0.8, 1.0) * color => vec3 init_color;

    dur elapsed_time;
    while (elapsed_time < effect_dur) {
        GG.nextFrame() => now;
        G.dt +=> elapsed_time;
        1.0 - elapsed_time / effect_dur => float t;

        circles.drawCircle(pos, init_radius * t, 1.0, t * t * init_color);
    }
}

// slowly expanding ring
fun void rippleEffect(vec2 pos) {
    .5 => float end_radius;
    .5::second => dur effect_dur;

    dur elapsed_time;
    while (elapsed_time < effect_dur) {
        GG.nextFrame() => now;
        G.dt +=> elapsed_time;
        M.easeOutQuad(elapsed_time / effect_dur) => float t;

        circles.drawCircle(pos, end_radius * t, .1 * (1 - t), Color.WHITE * (1 - t));
    }
}

// toggles a UI_Bool at hz freq n times after initial wait
fun void blinkEffect(UI_Bool b, float hz, dur d, dur wait) {
    wait => now;
    (1 / hz)::second => dur T;
    dur elapsed_time;
    while (elapsed_time < d) {
        G.dt_rate * T => now;
        T +=> elapsed_time;
        (1 - b.val()) => b.val;
    }
}

// multiple parts
// 1. color changes from white --> blue after .2 sec
// 2. after x secs, starts flashing (toggling draw) every .05 seconds 6 times
// 3. outer ring scale from 1 --> 2 over .35 sec
fun void pickupEffect(vec2 pos, int type) {
    // params
    UI_Bool draw(true);
    Color.WHITE => vec3 color;
    1 => float ring_scale;

    (.2 + .6)::second => dur effect_dur;
    dur elapsed_time;
    spork ~ blinkEffect(draw, 20, 1::second, .2::second);

    while (true) {
        GG.nextFrame() => now;
        G.dt +=> elapsed_time;
        if (elapsed_time > effect_dur) break;

        // after .2 secs
        if (elapsed_time > .2::second) {
            // change color
            if (elapsed_time > .2::second)
                pickup_colors[type] => color;

            // scale outer ring over .35 seconds
            ((elapsed_time - .2::second) / .6::second) => float t;
            1 + t * t => ring_scale;
        }

        if (draw.val()) { // draw
            if (type == PickupType_Boost || type == PickupType_SP) {
                polygons.drawSquare(pos, Math.pi * .25, pickup_scale * .4, color);
                lines.square(pos, Math.pi * .25, ring_scale * pickup_scale, color);
            } else if (type == PickupType_Health) {
                pickup_scale * .6 => float w; // center size
                polygons.drawBox(pos, 0, w, w * .33, color);
                polygons.drawBox(pos, .5 * Math.pi, w, w * .33, color);
                lines.circle(pos, ring_scale * w, Color.hex(0xFFFFFF));
            }
        }
    }
}

// TODO: add text garbage collection to ChuGL....
// using pool for now to prevenet leak
class TextPool 
{
    GText text_pool[64]; // shouldn't need more than this?
    HashMap text_to_index_map; // map from GText to text_pool index

    // populate hashmap, init GText
    for (int i; i < text_pool.size(); i++) {
        text_to_index_map.set(text_pool[i], i);
        text_pool[i].text("");
        text_pool[i] --> GG.scene();
        text_pool[i].antialias(false);
    }

    fun GText get() {
        // ==optimize== move from linear search to linked-list of available texts
        for (int i; i < text_pool.size(); i++) {
            if (text_pool[i] != null) {
                text_pool[i] @=> GText text;
                null @=> text_pool[i]; // set to null to mark borrowed
                return text;
            }
        }
        T.assert(false, "insufficient GText in TextPool");
        return null;
    }

    fun void ret(GText text) {
        text_to_index_map.has(text) => int belongs;
        T.assert(belongs, "trying to return text that does not belong to TextPool");

        if (belongs) {
            text_to_index_map.getInt(text) => int i;
            T.assert(text_pool[i] == null, "text borrowed but still in pool?");
            text @=> text_pool[i];
            text.text("");
        }
    }
}
TextPool text_pool;

fun void textEffect(string text_str, vec2 pos, vec3 color) {
    text_pool.get() @=> GText text;

    text.color(color);
    text.pos(pos);
    text.sca(.42);
    text.text(text_str);

    UI_Bool draw(true);
    spork ~ blinkEffect(draw, 20, .8::second, .2::second);

    dur elapsed_time;
    while (true) {
        GG.nextFrame() => now;
        G.dt +=> elapsed_time;
        if (elapsed_time > .8::second) break;

        text.alpha(draw.val());
    }

    text_pool.ret(text);
}


// ==optimize== group all under projectile pool if there are ever enough to matter
fun void spawnPickupTest(vec2 pos) {
    GG.nextFrame() => now; // to register as graphics shred

    // params
    .1 => float w;
    .1 => float h;
    .12 => float speed;
    .08 => float steering_rate;
    1 => float angular_velocity;

    b2BodyDef pickup_body_def;
    b2BodyType.kinematicBody => pickup_body_def.type;
    angular_velocity => pickup_body_def.angularVelocity;
    pos => pickup_body_def.position;
    false => pickup_body_def.enableSleep; // disable otherwise slowly rotating objects will be put to sleep

    b2Filter pickup_filter;
    Category_Pickup => pickup_filter.categoryBits;
    Category_Player => pickup_filter.maskBits;

    b2ShapeDef pickup_shape_def;
	/// A sensor shape generates overlap events but never generates a collision response.
    true => pickup_shape_def.isSensor; // note: sensors only create events with dynamic bodies?
    pickup_filter @=> pickup_shape_def.filter;

    b2.createBody(world_id, pickup_body_def) => int body_id;
    T.assert(b2Body.isValid(body_id), "spawnPickup body invalid");
    b2.makeBox(w, h) @=> b2Polygon polygon;
    b2.createPolygonShape(body_id, pickup_shape_def, polygon);

    // register body id in LUT
    G.pickup_map.set(body_id, PickupType_Basic);

    polygon.vertices() @=> vec2 vertices[];

    // draw until destroyed
    while (true) {
        GG.nextFrame() => now;

        // if destroyed, break out
        if (!b2Body.isValid(body_id)) break;

        // calculate transform
        b2Body.position(body_id) => vec2 pos;
        b2Body.angle(body_id) => float rot_radians;

        // steering towards player (if player is alive)
        // https://code.tutsplus.com/understanding-steering-behaviors-seek--gamedev-849t
        if (b2Body.isValid(player.body_id)) {
            b2Body.linearVelocity(body_id) => vec2 current_heading;
            // lines.drawSegment(pos, pos + current_heading); // debug draw
            M.normalize(player.pos() - pos) => vec2 desired_heading;
            desired_heading - current_heading => vec2 steering;
            b2Body.linearVelocity(
                body_id, 
                speed * M.normalize(current_heading + steering_rate * steering)
                // desired_heading // uncomment for no steering, instant course correction
            );
        }

        // <<< rot_radians , "velocity: ", b2Body.angularVelocity(body_id), b2Body.isAwake(body_id) >>>;
        // <<< "sleep enabled", b2Body.isSleepEnabled(body_id), "sleep threshold", b2Body.sleepThreshold(body_id) >>>;

        // TODO add outline width option to polygon drawer
        lines.drawPolygon(
            pos,
            rot_radians,
            vertices
        );
    }

    { // cleanup
        // remove from pick up map
        G.pickup_map.del(body_id);
        <<< "pickup map size: ", G.pickup_map.size() >>>;
    }
} 
spork ~ spawnPickupTest(@(1,0));


// boost pickup
// -1 == spawn side right, go left
// 1 == spawn side left, go right
-1 => int Side_Right;
1  => int Side_Left;
fun void spawnPickup(int which_side, int pickup_type) 
{
    T.assert(which_side == Side_Right || which_side == Side_Left, "spawnPickup invalid side, " + which_side);
    if (which_side != Side_Right && which_side != Side_Left) return;

    GG.nextFrame() => now; // to register as graphics shred

    // params
    .2 => float speed;
    Math.random2f(2.5, 3.5) => float pulse_freq; // for pulsing the health orb
    pickup_colors[pickup_type] => vec3 color;

    int body_id;

    { // create b2 body
        b2BodyDef pickup_body_def;
        b2BodyType.kinematicBody => pickup_body_def.type;

        // pickups that rotate while moving across screen
        if (pickup_type == PickupType_Boost || pickup_type == PickupType_SP) {
            Math.random2f(1, 2) => float angular_velocity;
            angular_velocity => pickup_body_def.angularVelocity;
        }

        G.camera.NDCToWorldPos(@(-1 * which_side, Math.random2f(-0.66, 0.66), 0.5)) $ vec2 => pickup_body_def.position;
        false => pickup_body_def.enableSleep; // disable otherwise slowly rotating objects will be put to sleep

        b2Filter pickup_filter;
        Category_Pickup => pickup_filter.categoryBits;
        Category_Player => pickup_filter.maskBits;

        b2ShapeDef pickup_shape_def;
        true => pickup_shape_def.isSensor;
        pickup_filter @=> pickup_shape_def.filter;

        b2.createBody(world_id, pickup_body_def) => body_id;
        T.assert(b2Body.isValid(body_id), "boost pickup b2body invalid");

        // physics shape
        if (pickup_type == PickupType_Boost || pickup_type == PickupType_SP) {
            b2.makeBox(pickup_scale, pickup_scale) @=> b2Polygon polygon;
            b2.createPolygonShape(body_id, pickup_shape_def, polygon);
        } else if (pickup_type == PickupType_Health) {
            b2.createCircleShape(body_id, pickup_shape_def, new b2Circle(pickup_scale * .5));
        } 
    }

    // register body id in LUT
    G.pickup_map.set(body_id, pickup_type);

    // movement (side to side on screen)
    b2Body.linearVelocity(body_id, speed * @(which_side, 0));

    // draw until destroyed
    while (true) {
        GG.nextFrame() => now;

        // if destroyed, break out
        if (!b2Body.isValid(body_id)) break;

        // calculate transform
        b2Body.position(body_id) => vec2 pos;
        b2Body.angle(body_id) => float rot_radians;

        // if its off the screen, destroy and break
        G.camera.worldPosToNDC(@(pos.x, pos.y, 0)) => vec3 pos_ndc;
        if (Math.fabs(pos_ndc.x) > 1 || Math.fabs(pos_ndc.y) > 1) {
            b2.destroyBody(body_id);
            break;
        }

        { // draw
            if (pickup_type == PickupType_Boost || pickup_type == PickupType_SP) {
                polygons.drawSquare(pos, rot_radians, pickup_scale * .4, color);
                lines.square(pos, rot_radians, pickup_scale, color);
            } else if (pickup_type == PickupType_Health) {
                pickup_scale * .6 => float w; // center size
                1 + .12 * Math.sin(G.dtf_cum * pulse_freq) *=> w; // oscillate!

                polygons.drawBox(pos, 0, w, w * .33, color);
                polygons.drawBox(pos, .5 * Math.pi, w, w * .33, color);
                lines.circle(pos, w, Color.hex(0xFFFFFF));
            } 
        }
    }

    { // cleanup
        // remove from pick up map
        G.pickup_map.del(body_id);
        <<< "pickup map size: ", G.pickup_map.size() >>>;
    }
}


// temp
fun void boostSpawner() {

        spork ~ spawnPickup(Side_Left, Math.random2(PickupType_Boost, PickupType_Health));
        spork ~ spawnPickup(Side_Right, Math.random2(PickupType_Boost, PickupType_Health));
    while (2::second => now) {
        spork ~ spawnPickup(Side_Left, Math.random2(PickupType_Boost, PickupType_SP));
        spork ~ spawnPickup(Side_Right, Math.random2(PickupType_Boost, PickupType_SP));
    }
} 
spork ~ boostSpawner();
/*
TODO:
- in collision event, detect if its boost or other pickup
- spawn a gradually expanding boost fade effect
*/


class ProjectilePool
{
    int body_ids[0];
    int num_active;
    .05 => float radius;
    5 => float speed;

    // debug
    int prev_active;

    fun int _addCollider() {
        // TODO: how do I stop simulating? just b2.destroyBody() ?
        // use b2Body.isEnabled().... (rather than awake)
        b2.createBody(world_id, projectile_body_def) => int body_id;
        b2Circle circle(radius);
        b2.createCircleShape(body_id, projectile_shape_def, circle);
        return body_id;
    }

    // fires a single projectile from given position along the direction
    fun void fire(vec2 position, float dir_radians) {
        // add new body
        if (num_active >= body_ids.size()) {
            body_ids << _addCollider();
        } 

        body_ids[num_active++] => int body_id;
        b2Body.position(body_id, position);
        b2Body.linearVelocity(
            body_id,
            speed * @(Math.cos(dir_radians), Math.sin(dir_radians)) 
        );
    }

    fun void update() {
        num_active => prev_active;
        for (int i; i < num_active; i++) {
            body_ids[i] => int body_id;
            b2Body.position(body_ids[i]) => vec2 pos;
            G.camera.worldPosToNDC(@(pos.x, pos.y, 0)) => vec3 pos_ndc;

            // if its off the screen, set inactive
            if (Math.fabs(pos_ndc.x) > 1 || Math.fabs(pos_ndc.y) > 1) {
                // swap with last
                body_ids[num_active - 1] => body_ids[i];
                body_id => body_ids[num_active - 1];
                // decrement counter to re-check the newly swapped id
                num_active--;
                // projectile boom!
                spork ~ boomEffect(pos);
            } else {
                // else draw the active!
                circles.drawCircle(pos, radius, .15, Color.WHITE);
            }
        }

        // if (num_active != prev_active)
            // <<< "num bullets active ", prev_active, " to ", num_active >>>;
    }
}

ProjectilePool projectile_pool;

int g_fire_mode;
// fire mode flags
0 => int FireMode_Normal;
1 => int FireMode_Triple;
2 => int FireMode_Spread;
3 => int FireMode_Count;

fun void shoot() {
    .1::second => dur attack_rate;
    0::second => dur cooldown;
    while (true) {
        GG.nextFrame() => now; // need to make this a graphics shred to use b2
        G.dt +=> cooldown;
        if (cooldown > attack_rate) {
            attack_rate -=> cooldown;
            spork ~ shootEffect(.5 * attack_rate);

            player.pos() => vec2 player_pos;
            // fire mode
            if (g_fire_mode == FireMode_Normal) {
                projectile_pool.fire(player_pos, player.rotation);
            } else if (g_fire_mode == FireMode_Triple) {
                .1 * M.rot2vec(player.rotation - (Math.pi / 2.0)) => vec2 perp;
                projectile_pool.fire(player_pos + perp, player.rotation);
                projectile_pool.fire(player_pos       , player.rotation);
                projectile_pool.fire(player_pos - perp, player.rotation);
            } else if (g_fire_mode == FireMode_Spread) {
                projectile_pool.fire(player_pos, player.rotation + .2);
                projectile_pool.fire(player_pos, player.rotation     );
                projectile_pool.fire(player_pos, player.rotation - .2);
            }
        }
    }
} 
spork ~ shoot();

// gameloop ============================================
DebugDraw debug_draw;
true => debug_draw.drawShapes;

int keys[0];
// keys.help();

while (true) {
    GG.nextFrame() => now;
    
    // update globals ======================================================
    GG.dt() * G.dt_rate => G.dtf;
    G.dtf::second => G.dt;
    G.dt +=> G.dt_cum;
    G.dtf +=> G.dtf_cum;

    // input ======================================================
    if (GWindow.keyDown(GWindow.Key_Space)) {
        <<< "shaking" >>>;
        spork ~ cameraShakeEffect(.1, 1::second, 30);
    }

    // boost
    if ( GWindow.key(GWindow.Key_Up) || UI.isKeyDown(UI_Key.GamepadR2) ) {
        1.5 => player.velocity_scale;
        Color.hex(0x00ffff) => player.boost_trail_color;
    } else if ( GWindow.key(GWindow.Key_Down) || UI.isKeyDown(UI_Key.GamepadL2) ) {
        (0.5) => player.velocity_scale;
        Color.hex(0xff0000) => player.boost_trail_color;
    } else {
        1.0 => player.velocity_scale;
        Color.hex(0xffa500) => player.boost_trail_color;
    }

    if (
        GWindow.key(GWindow.Key_Left)
        ||
        UI.isKeyDown(UI_Key.GamepadLStickLeft)
    ) {
        .1 +=> player.rotation;
    } 
    if (
        GWindow.key(GWindow.Key_Right)
        ||
        UI.isKeyDown(UI_Key.GamepadLStickRight)
    ) {
        -.1 +=> player.rotation;
    } 

    // switch weapons
    if (
        UI.isKeyPressed(UI_Key.GamepadFaceDown)
        ||
        GWindow.keyDown(GWindow.Key_Tab)
    ) {
        (1 + g_fire_mode) % FireMode_Count => g_fire_mode;
    }

    // explode test
    if (
        UI.isKeyPressed(UI_Key.GamepadFaceRight)
        ||
        GWindow.keyDown(GWindow.Key_Enter)
    ) {
        .5::second => dur explode_dur;
        // shake it up
        spork ~ cameraShakeEffect(.08, explode_dur, 30);
        // time warp!
        spork ~ slowEffect(.15, 2::second);
        spork ~ explodeEffect(@(0,0), explode_dur);
        spork ~ screenFlashEffect(20, .1::second); // TODO add back after fixing screen pass to flash white
    }

    // effects test
    if (GWindow.keyDown(GWindow.Key_1)) {
        spork ~ refreshEffect();
    }

    { // player logic
        // update ======================================================
        @(Math.cos(player.rotation), Math.sin(player.rotation)) => vec2 player_dir;
        // apply velocity
        b2Body.linearVelocity(
            player.body_id, 
            player.velocity_scale * player_dir
        );
        // apply rotation
        b2Body.angle(player.body_id, player.rotation);


        // render ======================================================
        b2Body.position(player.body_id) => vec2 player_pos;

        // wrap player around screen
        G.camera.worldPosToNDC(player_pos $ vec3) => vec3 player_pos_ndc;
        if (player_pos_ndc.x > 1) -1 => player_pos_ndc.x;
        if (player_pos_ndc.x < -1) 1 => player_pos_ndc.x;
        if (player_pos_ndc.y > 1) -1 => player_pos_ndc.y;
        if (player_pos_ndc.y < -1) 1 => player_pos_ndc.y;
        G.camera.NDCToWorldPos(player_pos_ndc) $ vec2 => player_pos;
        b2Body.position(player.body_id, player_pos);

        // draw circle at player position
        // circles.drawCircle(player_pos, player.radius, .15, Color.WHITE);

        lines.drawPolygon(
            player_pos,
            -Math.pi / 2 + player.rotation,
            [
                1* @(0, .1),
                // 1* @(-.15, 0),
                1* @(-.15, -.1),
                1* @(.15, -.1),
                // 1* @(.15, 0)
            ]
        );

        // ship exhaust
        M.rot2vec(player.rotation - (Math.pi / 2.0)) => vec2 player_rot_perp;
        player_pos - .9 * player.radius * player_dir => vec2 exhaust_pos;

        // ~20 shreds
        spork ~ boostTrailEffect(
            exhaust_pos + .07 * player_rot_perp,
            player.boost_trail_color,
            player.velocity_scale // scale trail size by speed
        );

        // ~20 shreds
        spork ~ boostTrailEffect(
            exhaust_pos - .07 * player_rot_perp,
            player.boost_trail_color,
            player.velocity_scale // scale trail size by speed
        );
    }

    projectile_pool.update();


    // collisions (TODO does it matter what order in game loop this happens?)
    {
        // b2World.contactEvents(world_id, begin_touch_events, null, null);
        // for (int i; i < begin_touch_events.size(); i++) {
        //     <<< "begin_touch:", begin_touch_events[i] >>>;
        // }

        b2World.sensorEvents(world_id, begin_sensor_events, null);
        for (int i; i < begin_sensor_events.size(); 2 +=> i) {
            // assume for now that only pickups trigger sensor events
            // and that the order is deterministic (sensor, player, sensor, player....) 
            begin_sensor_events[i] => int pickup_id;
            begin_sensor_events[i + 1] => int player_id;

            T.assert(player_id == player.body_id, "non-player triggered sensor event");

            // switch on pickup type
            G.pickup_map.getInt(pickup_id) => int pickup_type;
            if (pickup_type == PickupType_Basic) {
                // pickup effect
                spork ~ rippleEffect(b2Body.position(pickup_id));

                // spawn another!
                G.camera.NDCToWorldPos(
                    @(
                        Math.random2f(-1, 1),
                        Math.random2f(-1, 1),
                        .0
                    )
                ) $ vec2 => vec2 new_pickup_pos;
                spork ~ spawnPickupTest(new_pickup_pos);
            } else { // all other pickup types
                b2Body.position(pickup_id) => vec2 pickup_pos;
                spork ~ pickupEffect(pickup_pos, pickup_type);
                spork ~ textEffect(
                    pickup_text[pickup_type],
                    M.randomPointInCircle(pickup_pos, .25, .35),
                    pickup_colors[pickup_type]
                );
            }

            { // cleanup
                // remove sensor from b2
                b2.destroyBody(pickup_id);
            }
        }
    }


    // flush 
    if (true) {
        circles.update();
        lines.update();
        polygons.update();
    } else {
        // physics debug draw
        b2World.draw(world_id, debug_draw);
        debug_draw.update();
    }
}
