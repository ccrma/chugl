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

// set MSAA
GG.renderPass().samples(1);

// pixelate the output
TextureSampler output_sampler;
TextureSampler.Filter_Nearest => output_sampler.filterMin;  
TextureSampler.Filter_Nearest => output_sampler.filterMag;  
TextureSampler.Filter_Nearest => output_sampler.filterMip;  
GG.outputPass().sampler(output_sampler);


b2DebugDraw_Circles circles --> GG.scene();
b2DebugDraw_Lines lines --> GG.scene();
circles.antialias(false);

int camera_shake_generation;
fun void cameraShake(float amplitude, dur shake_dur, float hz) {
    ++camera_shake_generation => int gen;
    now => time shake_start;
    now + shake_dur => time shake_end;

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
        // shake done OR another shake triggred, stop this one
        if (now > shake_end || gen != camera_shake_generation) break;

        // compute fraction shake progress
        (now - shake_start) / shake_dur => float progress;
        (now - shake_start) / camera_delta_period => float elapsed_periods;
        elapsed_periods $ int => int floor;
        elapsed_periods - floor => float fract;

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

class Player extends GGen
{
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
}

FlatMaterial flat_material;
PlaneGeometry plane_geo;
Player player --> GG.scene();
GMesh shoot_effect(plane_geo, flat_material) --> player;
0 => shoot_effect.sca;
shoot_effect.pos(@(player.radius, 0)); // position at blaster

// TODO hashmap to track tween generations to prevent multiple concurrent of same type
fun void shootEffect(dur tween_dur) {
    now => time tween_start;
    now + tween_dur => time tween_end;

    // .1 => shoot_effect.sca;
    while (now < tween_end) {
        GG.nextFrame() => now;
        (now - tween_start) / tween_dur => float t;
        .1 * (1 - t * t) => shoot_effect.sca;
    }
    // 0 => shoot_effect.sca;
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
                <<< "deactivating bullet" >>>;
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


fun void shoot() {
    .1::second => dur attack_rate;
    now => time last_fire;
    while (true) {
        GG.nextFrame() => now;
        if (now - last_fire > attack_rate) {
            attack_rate +=> last_fire;
            spork ~ shootEffect(.5 * attack_rate);
            projectile_pool.fire(player.pos() $ vec2, player.rotation);
        }
    }
} spork ~ shoot();



// gameloop ============================================

int keys[0];
while (true) {
    GG.nextFrame() => now;

    // input ======================================================
    if (GWindow.keyDown(GWindow.Key_Space)) {
        <<< "shaking" >>>;
        spork ~ cameraShake(.1, 1::second, 30);
    }

    if (GWindow.key(GWindow.Key_Left)) {
        .1 +=> player.rotation;
    } 
    if (GWindow.key(GWindow.Key_Right)) {
        -.1 +=> player.rotation;
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

        // update ggen
        player_pos => player.pos;
        player.rotation => player.rotZ;

        // draw circle at player position
        circles.drawCircle(player_pos, player.radius, .15, Color.WHITE);
        lines.drawSegment(player_pos, player_pos + player.radius * player_dir);
    }

    projectile_pool.update();

    // flush 
    circles.update();
    lines.update();



    // { // kb test
    //     GWindow.keysDown() @=> int keysDown[];
    //     if (keysDown.size()) {
    //         <<< "Keys pressed this frame: ", keysDown.size() >>>;
    //         for (auto k : keysDown) {
    //             <<< "Key: ", k >>>;
    //         }
    //     }

    //     GWindow.keysUp() @=> int keysUp[];
    //     if (keysUp.size()) {
    //         <<< "Keys released this frame: ", keysUp.size() >>>;
    //         for (auto k : keysUp) {
    //             <<< "Key: ", k >>>;
    //         }
    //     }

    //     GWindow.keys() @=> int keys[];
    //     if (keys.size()) {
    //         <<< "Keys held this frame: ", keys.size() >>>;
    //         for (auto k : keys) {
    //             <<< "Key: ", k >>>;
    //         }
    //     }
    // }

    if (false)
    { // kb test
        GWindow.keysDown(keys);
        if (keys.size()) {
            <<< "Keys pressed this frame: ", keys.size() >>>;
            for (auto k : keys) {
                <<< "Key: ", k >>>;
            }
        }

        GWindow.keysUp(keys);
        if (keys.size()) {
            <<< "Keys released this frame: ", keys.size() >>>;
            for (auto k : keys) {
                <<< "Key: ", k >>>;
            }
        }

        GWindow.keys(keys);
        if (keys.size()) {
            <<< "Keys held this frame: ", keys.size() >>>;
            for (auto k : keys) {
                <<< "Key: ", k >>>;
            }
        }
    }
}
