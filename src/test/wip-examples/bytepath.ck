// import materials for 2D drawing
@import "b2.ck"

// GOrbitCamera camera --> GG.scene();
// GG.scene().camera(camera);
GG.camera() @=> GCamera camera;
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

// Math ============================================

class M
{
    fun static vec2 rot2vec(float radians) {
        return @( Math.cos(radians), Math.sin(radians) );
    }

    fun static vec2 randomDir() {
        return rot2vec(Math.random2f(0, Math.two_pi));
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

}



// Camera shake ============================================
int camera_shake_generation;
fun void cameraShake(float amplitude, dur shake_dur, float hz) {
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

// physics state setup ============================================

b2WorldDef world_def;
b2.createWorld(world_def) => int world_id;

{ // simulation config (eventually group these into single function/command/struct)
    b2.world(world_id);
    // b2.substeps(1);
}

b2BodyDef dynamic_body_def;
b2BodyType.kinematicBody => dynamic_body_def.type;

// player ============================================

class Player
{
    // TODO cache and store player rotation / position once per frame rather than recalculating constantly
    int body_id;
    int shape_id;
    float rotation;
    .1 => float radius;

    fun @construct() {
        // create body def
        // @(Math.random2f(-4.0, 4.0), Math.random2f(6.0, 12.0)) => dynamic_body_def.position;
        // Math.random2f(0.0,Math.two_pi) => float angle;
        // @(Math.cos(angle), Math.sin(angle)) => dynamic_body_def.rotation;
        b2.createBody(world_id, dynamic_body_def) => this.body_id;

        // then shape
        b2ShapeDef shape_def;
        b2Circle circle(this.radius);
        b2.createCircleShape(this.body_id, shape_def, circle) => this.shape_id;
    }

    fun vec2 pos() {
        return b2Body.position(this.body_id);
    }
}

Player player;

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
fun void slowEffect(float rate, dur d) {
    dur elapsed_time;

    while (elapsed_time < d) {
        GG.nextFrame() => now;
        // must use GG.dt() so it's not affected by it's own time warp!
        GG.dt()::second +=> elapsed_time; 
        elapsed_time / d => float t;
        M.lerp(M.easeInOutCubic(t), rate, 1.0) => G.dt_rate;
        T.assert(G.dt_rate <= 1.0, "dt_rate exceeds bounds");
        <<< "rate",  G.dt_rate, "t", t, "d", d, "rate", rate >>>;
    }

    // restore to normal rate
    1.0 => G.dt_rate;
}

// spawns an explosion of lines going in random directinos that gradually shorten
fun void explodeEffect(vec2 pos) {
    // params
    Math.random2(8, 16) => int num; // number of lines
    .5::second => dur max_dur;

    // shake it up
    spork ~ cameraShake(.08, max_dur, 30);

    // time warp!
    spork ~ slowEffect(.1, 10::second) @=> Shred slow_effect_shred;

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

    // TODO: create Effects manager that 
    // 1: can let effects spork other effects without prematurely terminating the sporked effects
    // if the shreds exits
    // 2: de-dups by string. E.g. if multiple 'slow' effects are spawned will replace the previous
    // do this via generation tracking
    while (!slow_effect_shred.done()) {
        GG.nextFrame() => now;
    }
}

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
        b2.createBody(world_id, dynamic_body_def) => int body_id;
        b2Circle circle(radius);
        b2.createCircleShape(body_id, new b2ShapeDef, circle);
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

        if (num_active != prev_active)
            <<< "num bullets active ", prev_active, " to ", num_active >>>;
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

int keys[0];
while (true) {
    GG.nextFrame() => now;
    
    // update globals ======================================================
    GG.dt() * G.dt_rate => G.dtf;
    G.dtf::second => G.dt;

    // input ======================================================
    if (GWindow.keyDown(GWindow.Key_Space)) {
        <<< "shaking" >>>;
        spork ~ cameraShake(.1, 1::second, 30);
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
    if (UI.isKeyPressed(UI_Key.GamepadFaceDown)) {
        (1 + g_fire_mode) % FireMode_Count => g_fire_mode;
    }

    // explode test
    if (UI.isKeyPressed(UI_Key.GamepadFaceRight)) {
        spork ~ explodeEffect(@(0,0));
    }

    { // player logic
        // update ======================================================
        @(Math.cos(player.rotation), Math.sin(player.rotation)) => vec2 player_dir;
        b2Body.linearVelocity(
            player.body_id, 
            player_dir
        );


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
        circles.drawCircle(player_pos, player.radius, .15, Color.WHITE);
        lines.drawSegment(player_pos, player_pos + player.radius * player_dir);
    }

    projectile_pool.update();

    // flush 
    circles.update();
    lines.update();
    polygons.update();
}
