
@import "g2d"
@import "../../T.ck"

G2D g2d;

GG.outputPass().tonemap(OutputPass.ToneMap_None);
GG.camera().orthographic();
GG.camera().viewSize(10); // viewport height is 10 meters

class M {
    fun static vec2 rot2vec(float radians) {
        return @( Math.cos(radians), Math.sin(radians) );
    }

    fun static vec2 normalize(vec2 n) {
        return n / Math.hypot(n.x, n.y); // hypot is the magnitude
    }

    fun static float mag(vec2 n) {
        return  Math.hypot(n.x, n.y); 
    }

    // unwinds rad to be in range [0, 2pi]
    fun static float unwind(float rad) {
        while (rad > Math.two_pi) Math.two_pi -=> rad;
        while (rad < 0)           Math.two_pi +=> rad;
        T.assert(rad >= 0 && rad <= Math.two_pi, "unwind failed");
        return rad;
    }

    // compute the angle between two angles in radians
    fun static float deltaAngle(float rad1, float rad2) {
        unwind(rad1) => rad1;
        unwind(rad2) => rad2;
        Math.fabs(rad1 - rad2) => float delta;
        Math.min(delta, Math.fabs(Math.two_pi - delta)) => delta;
        T.assert(delta <= Math.pi, "deltaAngle can be at most pi");
        return delta;
    }

    // given an event happens once every `period_secs`, returns
    // the amount of time you need to wait until the next occurance.
    // returns the amount of time in seconds until the next event occurs
    fun static float poisson(float period_sec) {
        (1 / 1000.0) => float dt_sec;
        dt_sec / period_sec => float success_rate;

        if (success_rate <= 0) T.assert(false, "poisson success_rate <= 0");

        0 => int count;
        while (Math.randomf() > success_rate) count++;

        return count * dt_sec;
    }
}

class FX {

} FX fx;

class GS {
    // config --------------
    UI_Float env_pole_pos(.9995);
    UI_Float scope_scale(.0055);
    UI_Float bang_threshold(.004);
    UI_Float rotation_speed(0.5);
    UI_Float aim_width_rad(Math.pi/20.0);
    UI_Float rotation_interp(.14);

    // if true, snaps aim to clock direction.
    // if false, uses relative aim (higher pitch to rotate clockwise, vice versa)
    UI_Bool aim_mode_abs(false); 

    UI_Float enemy_size(.1);
    UI_Float enemy_radius(.05);
    UI_Float enemy_speed(.05);

    // Audio state
    int vol_above_thresh_last_frame;
    float last_pitch;

    // Player state
    float rot_rad;
    6 => float midi_pitch_target; // last detected pitch
    midi_pitch_target => float midi_pitch_curr; // last detected pitch
    float pitch_rad;

    // Enemy state
    Entity entities[0]; // ==optimize== turn into pool
    int entity_count;


    fun void spawn() {
        <<< "spawning!" >>>;
        null => Entity@ e;

        if (entities.size() <= entity_count) {
            entities << new Entity;
        } 
        entities[entity_count] @=> e;

        // pick random angle
        Math.random2f(0, Math.two_pi) => float rand_angle;
        M.rot2vec(rand_angle) => vec2 rand_dir;

        // initialize entity
        5 * rand_dir => e.pos;
        rand_angle => e.rot_rad;
        1 => e.hp;
        true => e.active;

        entity_count++;
    }

    fun void updateEntities(float dt) {
        for (0 => int i; i < entity_count; i++) {
            entities[i] @=> Entity@ e;
            // check death
            if (e.hp <= 0)  {
                false => e.active;
                // swap with last element
                entities[entity_count - 1] @=> entities[i];
                e @=> entities[entity_count - 1];
                entity_count--;
                i--;
                continue;
            }
            
            // update logic
            -1 * M.normalize(e.pos) * dt * gs.enemy_speed.val() +=> e.pos;
        }
    }

    fun void drawEntities() {
        for (0 => int i; i < entity_count; i++) {
            entities[i] @=> Entity@ e;
            T.assert(e.active, "inactive entity within entity_count");

            M.rot2vec(e.rot_rad) => vec2 dir;
            g2d.capsuleFilled(
                e.pos, e.pos + gs.enemy_size.val() * dir, 
                gs.enemy_radius.val(), Color.WHITE
            );
        }
    }
} GS gs;

class Entity { // ==optimize== no class, just store in float array
    vec2 pos;
    float rot_rad;
    float hp;
    false => int active;
}



// audio setup (envelope follower) =====================

// TODO: maybe run through LPF first for gain detection?
adc => Gain g => OnePole env_follower => blackhole;
// square the input
adc => g;
// multiply
3 => g.op;
// filter pole position
gs.env_pole_pos.val() => env_follower.pole;

// audio analysis
8 * 1024 $ int => int WINDOW_SIZE; 
// TODO: if this is large (eg 48000) it sometimes causes crash on ChuGL.cpp 
// Assertion(xform != NULL) failed: file "/Users/Andrew/Google-Drive/Stanford/chugl/src/ChuGL.cpp", line 154
// seems to happen from setting accum.size
// run in debugger to see if memory corruption is happening, might be in Flip.size() method...
env_follower => Flip accum => blackhole;
WINDOW_SIZE => accum.size;
float samples[0];

////////////////////////////////////////////////////////////////////
// PitchTrack is a monophonic autocorrelation pitch tracker with  //
// a fast response and extremely high accuracy, even at low       //
// frequencies. It is adapted from [helmholtz~] for Pd by Katja,  //
// documented at http://www.katjaas.nl/helmholtz/helmholtz.html   //
////////////////////////////////////////////////////////////////////

// Options
// get(): (read only) get calculated frequency
//
// fidelity: (float) [0-1], default 0.95
//   This is a threshold for certainty about the result. A highly periodic
//   signal (ie one that has a strong pitch center) should produce a result
//   with a high fidelity, which a non-periodic signal (eg noise) will have
//   a very low fidelity. Setting this parameter close to 1 should reduce
//   the number of inaccurate reports.
//
// sensitivity (float) [0-1], default 0.003
//   This is the minimum RMS value to trigger a pitch calculation. Setting
//   this parameter low forces PitchTrack to attempt to find the pitch of
//   even very quiet sounds. Higher values will cause it to trigger only
//   on louder notes.
//
// overlap (int) [1-?], default 2
//   How much to overlap successive analysis frames. Higher values should
//   produce smoother values, at the cost of an increase of CPU load.
//
// frame (int) [128-?], default 2048
//   Size of FFT frame for analysis. Smaller values result in lower latency
//   and high responsiveness but less accuracy. Higher values result in
//   considerably greater CPU load. Values that aren't powers of 2 get
//   rounded up to the next power of 2. Recommend 512, 1024, or 2048.
//
// bias (float) [0-1], default 0.2
//   Katja's pitch tracker introduces a small bias to help with the tracking.
//   (See the link above.) I don't know how this parameter affects the output.

// Example
// PitchTrack must connect to blackhole to run
adc => PitchTrack pitch => blackhole;
pitch.frame(512);
pitch.overlap(4);

while (true) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    { // mic input
        // bang
        int bang;
        env_follower.last() > gs.bang_threshold.val() => int vol_above_thresh;
        if (!gs.vol_above_thresh_last_frame && vol_above_thresh) {
            <<< "BANG:", env_follower.last() >>>;
            true => bang;
        }
        vol_above_thresh => gs.vol_above_thresh_last_frame;

        if (bang) { // hit detection ==optimize== can do this in entity update of last frame
            Math.FLOAT_MAX => float min_dist;
            null => Entity@ target_entity;
            for (0 => int i; i < gs.entity_count; i++) {
                // get angle between enemy and aim
                gs.entities[i] @=> Entity@ e;

                M.deltaAngle(e.rot_rad, gs.rot_rad) < gs.aim_width_rad.val() => int in_range;
                if (in_range) {
                    M.mag(e.pos) => float dist;
                    if (target_entity == null || dist < min_dist) {
                        dist => min_dist;
                        e @=> target_entity;
                    }
                }
            }
            // dmg
            if (target_entity != null) {
                1.0 -=> target_entity.hp;
                // TODO attack animation
            }
        }

        // pitch --> rotate
        Math.fmod(Std.ftom(pitch.get()), 12) => gs.midi_pitch_target;
    }

    float angular_velocity;
    int pitch_updated;
    { // update
        (!Math.equal(pitch.get(), gs.last_pitch)) => pitch_updated;

        // TODO: zero out target pitch when it's not updated?
        if (!pitch_updated) 6 => gs.midi_pitch_target;

        // interpolate current pitch to target
        gs.midi_pitch_curr + gs.rotation_interp.val() * dt * 60 * (gs.midi_pitch_target - gs.midi_pitch_curr) => gs.midi_pitch_curr;

        // if (pitch_updated) {
            if (gs.aim_mode_abs.val()) {
                // map pitch to clock position
                (-1 * (gs.midi_pitch_curr $ int)  * (Math.two_pi / 12.0)) + Math.pi/2.0 => gs.pitch_rad;
                M.unwind(gs.pitch_rad) => gs.pitch_rad;
                // interpolate aim direction towards pitch
                gs.rot_rad + dt * 5 * (gs.pitch_rad - gs.rot_rad) => gs.rot_rad;
            } else {
                // relative aiming , speed scaled
                (6 - gs.midi_pitch_curr) * gs.rotation_speed.val() => angular_velocity;
                angular_velocity * dt +=> gs.rot_rad;
                true => pitch_updated;
            }
        // }
        M.unwind(gs.rot_rad) => gs.rot_rad;
        pitch.get() => gs.last_pitch;

        // enemy spawn
        if (Math.randomf() < dt / 3) gs.spawn();

        gs.updateEntities(dt);
    }

    { // draw
        // g2d.dashed(@(0,0), 5 * M.rot2vec(gs.rot_rad), Color.WHITE, .1);
        // target in abs aim mode
        g2d.line(@(0,0), 5 * M.rot2vec(gs.pitch_rad), Color.RED);
        // draw aim width
        g2d.dashed(@(0,0), 5 * M.rot2vec(gs.rot_rad + gs.aim_width_rad.val()), Color.WHITE, .1);
        g2d.dashed(@(0,0), 5 * M.rot2vec(gs.rot_rad - gs.aim_width_rad.val()), Color.WHITE, .1);

        // draw angular speedometer
        .1 => float speedometer_hh;
        1.5 => float speedometer_hw;
        g2d.box(@(0, 3), 2* speedometer_hw, 2*speedometer_hh, Color.WHITE);
        g2d.line(@(0, 3 + speedometer_hh), @(0, 3 - speedometer_hh), Color.WHITE);
        if (pitch_updated) {
            -(6 - gs.midi_pitch_curr) / 6.0 * speedometer_hw => float vel_meter_x;
            g2d.boxFilled(
                @(vel_meter_x * .5, 3),
                0.0,
                Math.fabs(vel_meter_x),
                2*speedometer_hh,
                Color.WHITE
            );
        }

        // hit detection debug draw
        for (0 => int i; i < gs.entity_count; i++) {
            gs.entities[i] @=> Entity@ e;
            if (M.deltaAngle(e.rot_rad, gs.rot_rad) < gs.aim_width_rad.val()) {
                g2d.line(@(0,0), e.pos, Color.RED);
            }
        }

        

        gs.drawEntities();

        g2d.update();
    }

    { // developer console
        UI.begin("DEV CONSOLE");
        // UI.checkbox("b2DebugDraw", conf.b2_debug_draw);
        if (UI.slider("Envelope Pole Position", gs.env_pole_pos, .9, 1.0)) gs.env_pole_pos.val() => env_follower.pole;
        UI.slider("Envelope Bang Threshold", gs.bang_threshold, 0.0, 0.1);

        UI.slider("Rotation Speed", gs.rotation_speed, 0.0, 1.5);

        UI.checkbox("Aim Mode Abs", gs.aim_mode_abs);

        UI.slider("Rotation Speed", gs.rotation_speed, 0.0, 1.5);
        UI.slider("Rotation Interp", gs.rotation_interp, 0.0, .15);
        UI.slider("Aim Width", gs.aim_width_rad, 0.0, 1);

        UI.slider("Enemy size", gs.enemy_size, 0.0, .1);
        UI.slider("Enemy radius", gs.enemy_radius, 0.0, .1);
        UI.slider("Enemy speed", gs.enemy_speed, 0.0, .1);

        accum.upchuck();
        accum.output( samples ); // ==optimize== track in UI chugin directly, don't need to pass array memory from cpp --> chuck --> cpp
        // void plotLines(string label, float[] values, int values_offset, string overlay_text, float scale_min, float scale_max, vec2 graph_size)
        UI.plotLines("Env History", samples, 0, "", 0, gs.scope_scale.val(), @(-1,-1));

        UI.end();
    }
}