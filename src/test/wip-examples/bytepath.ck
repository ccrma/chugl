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

class Player 
{
    int body_id;
    int shape_id;
    float rotation;
    .1 => float radius;

    fun @construct() {
        // create body def
        b2BodyDef dynamic_body_def;
        b2BodyType.kinematicBody => dynamic_body_def.type;
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

Player player;

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

    // update ======================================================
    @(Math.cos(player.rotation), Math.sin(player.rotation)) => vec2 player_dir;
    b2Body.linearVelocity(
        player.body_id, 
        player_dir
    );


    // render ======================================================
    // draw circle at player position
    b2Body.position(player.body_id) => vec2 player_pos;
    circles.drawCircle(player_pos, player.radius, .15, Color.WHITE);
    lines.drawSegment(player_pos, player_pos + player.radius * player_dir);

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
