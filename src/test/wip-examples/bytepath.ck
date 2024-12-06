// import materials for 2D drawing
@import "b2.ck"

// GOrbitCamera camera --> GG.scene();
// GG.scene().camera(camera);
GG.camera() @=> GCamera camera;
camera.orthographic();
camera.viewSize(5);
// camera.posZ(10);

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


b2DebugDraw_Circles circles --> GG.scene();
b2DebugDraw_Lines lines --> GG.scene();
b2DebugDraw_SolidPolygon polygons --> GG.scene();
circles.antialias(false);

/*
Slow Effect Brainstorm
b2: just have a "slow" factor api call e.g. b2.timeStretch()
- can't do a per-frame simulate call because this needs to happen BEFORE we flush the command queue
Effects: scale with GG.dt(), introduce a GG.timeScale() which scales all delta times

*/

// Globals ============================================
class G
{
    static dur dt;
    static float dtf;
    1.0 => static float dt_rate; // modifier for time stretch
}

// collision category flags
(1 << 1) => int Category_Player;      // 2
(1 << 2) => int Category_Projectile;  // 4
(1 << 3) => int Category_Pickup;      // 8

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

}

// physics state setup ============================================

b2WorldDef world_def;
b2.createWorld(world_def) => int world_id;
int begin_touch_events[0]; // for holding collision data
int begin_sensor_events[0]; // for holding sensor data

{ // simulation config (eventually group these into single function/command/struct)
    b2.world(world_id);
    b2.substeps(4);
}

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

    int camera_deltas_idx;
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
        (1.0 - progress) * delta => camera.pos;
    }
}

// TODO hashmap to track tween generations to prevent multiple concurrent of same type
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

// ==optimize== batch these together so all effects are managed
// by a single shred looping over a progress array, rather than per shred
fun void boomEffect(vec2 pos) {
    // how can we improve chugl syntax for these types of per-frame multi-stage timing effects?
    // 1. somehow add a retained mode to the vector graphics?
    // for (1::second) { do xyz }
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

    while (elapsed_time < d && gen == slow_effect_generation) {
        GG.nextFrame() => now;
        // must use GG.dt() so it's not affected by it's own time warp!
        GG.dt()::second +=> elapsed_time; 
        M.lerp(M.easeInOutCubic(elapsed_time / d), rate, 1.0) => float t;

        // adjust animation rate
        t => G.dt_rate;
        // adjust physics rate
        t => b2.rate;

        T.assert(G.dt_rate <= 1.0, "dt_rate exceeds bounds");
        // <<< "rate",  G.dt_rate, "t", t, "d", d, "rate", rate >>>;
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
        Math.random2f(.3, .5)::second => durations[i];
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
        period => now;
        period +=> elapsed_time;
        <<< "screen flash", toggle >>>;
        if (toggle) {
            GG.outputPass().exposure(1.0);
            <<< "1 exposure" >>>;
        } else {
            GG.outputPass().exposure(0.0);
            <<< "0 exposure" >>>;
        }
        1 - toggle => toggle;
    }

    // restore
    if (gen == screen_flash_effect_generation) {
        GG.outputPass().exposure(1.0);
    }
}


int refresh_effect_generation;
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

// ==optimize== group all under projectile pool if there are ever enough to matter
fun void spawnPickup(vec2 pos) {
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
    true => pickup_shape_def.enableSensorEvents;
    true => pickup_shape_def.isSensor; // note: sensors only create events with dynamic bodies?
    pickup_filter @=> pickup_shape_def.filter;

    b2.createBody(world_id, pickup_body_def) => int body_id;
    T.assert(b2Body.isValid(body_id), "spawnPickup body invalid");
    b2.makeBox(w, h) @=> b2Polygon polygon;
    b2.createPolygonShape(body_id, pickup_shape_def, polygon);

    polygon.vertices() @=> vec2 vertices[];

    // draw until destroyed
    while (true) {
        GG.nextFrame() => now;

        // if destroyed, break out
        if (!b2Body.isValid(body_id)) return;

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
} 
spork ~ spawnPickup(@(1,0));

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
            camera.worldPosToNDC(@(pos.x, pos.y, 0)) => vec3 pos_ndc;

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
    now => time last_fire;
    while (true) {
        GG.nextFrame() => now; // need to make this a graphics shred to use b2
        if (now - last_fire > attack_rate) {
            attack_rate +=> last_fire;
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
while (true) {
    GG.nextFrame() => now;
    
    // update globals ======================================================
    GG.dt() * G.dt_rate => G.dtf;
    G.dtf::second => G.dt;

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
        camera.worldPosToNDC(player_pos $ vec3) => vec3 player_pos_ndc;
        if (player_pos_ndc.x > 1) -1 => player_pos_ndc.x;
        if (player_pos_ndc.x < -1) 1 => player_pos_ndc.x;
        if (player_pos_ndc.y > 1) -1 => player_pos_ndc.y;
        if (player_pos_ndc.y < -1) 1 => player_pos_ndc.y;
        camera.NDCToWorldPos(player_pos_ndc) $ vec2 => player_pos;
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

        spork ~ boostTrailEffect(
            exhaust_pos + .07 * player_rot_perp,
            player.boost_trail_color,
            player.velocity_scale // scale trail size by speed
        );

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

            // pickup effect
            spork ~ rippleEffect(b2Body.position(pickup_id));

            // remove sensor from b2
            b2.destroyBody(pickup_id);

            // spawn another!
            camera.NDCToWorldPos(
                @(
                    Math.random2f(-1, 1),
                    Math.random2f(-1, 1),
                    .0
                )
            ) $ vec2 => vec2 new_pickup_pos;
            spork ~ spawnPickup(new_pickup_pos);
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
