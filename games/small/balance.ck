/*

Levels/Goals
- Max height
    - I want to be able to go off the top of the screen!
- Horizontal Distance traveled (maybe that's a separate level)
- time survived

Opus Magnum style different metrics
- horizontal distance
- vertical height
- time survived

Design ideas:
- visual bar to show distance from pivot / how fast you are going in a direction
    - is distance calculated from center of screen or base position of stick?
- maybe game is paused when not making sound, so you can breath

*/
@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"

G2D g;

GG.camera().posY(3.5);

class Pillar {
    float x, vx;
    float angle, vAngle;
    float len;
}

Pillar p;
int alive;

// mouse params
// UI_Float acc(.002);
// UI_Float acc_damp(.98);
// UI_Float angle_len_factor(.001);
// UI_Float angle_acc(.008);
// UI_Float angle_damp(.98);
// UI_Float angle_dt(1);

// voice params
UI_Float acc(.0032);
UI_Float acc_damp(.975);
UI_Float angle_len_factor(.001);
UI_Float angle_acc(.008);
UI_Float angle_damp(.96);
UI_Float angle_dt(1);

adc => PitchTrack pitch => blackhole;
// pitch.fidelity(.98);
pitch.frame(512);
pitch.overlap(4);
// pitch.sensitivity(.005);
float last_pitch;
float midi_pitch;
int pitch_detected;
float acc_x;

fun void init() {
    0 => p.x => p.vx => p.angle => p.vAngle;
    1 => p.len;
    true => alive;

    // GG.camera().posX(0);
}

init();
while (1) {
    GG.nextFrame() => now;
    g.mousePos() => vec2 mouse_pos;


    UI.slider("acc", acc, 0, .015);
    UI.slider("acc_damp", acc_damp, .9, 1);
    UI.slider("angle_len_factor", angle_len_factor, 0, .002);
    UI.slider("angle_acc", angle_acc, 0, .01);
    UI.slider("angle_damp", angle_damp, .9, 1);
    UI.slider("angle_dt", angle_dt, 0, 1);

    { // input
        // if (GWindow.key(GWindow.KEY_LEFT)) {
        //     .1 -=> p.x;
        // }
        // if (GWindow.key(GWindow.KEY_RIGHT)) {
        //     .1 +=> p.x;
        // }

        // mosue controls
        // if (alive && g.mouse_left) {
            // acc.val() * (mouse_pos.x - p.x) +=> p.vx; // TODO try accumulating vx instead, so you control acceleration not velocity (more unstable, like space controls)
        // }


        // voice controls
        pitch.get() => float curr_pitch;
        (curr_pitch != last_pitch) => pitch_detected;
        curr_pitch => last_pitch;
        if (pitch_detected) {
            // Math.fmod(Std.ftom(curr_pitch), 12) => midi_pitch;
            Std.ftom(curr_pitch) => midi_pitch;

            // hard-code center to Eb 3
            .33 * (midi_pitch - 51) => acc_x;

            acc.val() * acc_x +=> p.vx; 
            // <<< mouse_pos.x - p.x >>>;
        }
        acc_damp.val() *=> p.vx;


        if (!alive && GWindow.keyDown(GWindow.KEY_SPACE)) init();
    }

    // if (alive && pitch_detected) { // update
    if (alive ) { // update

        // angular vel
        angle_len_factor.val() * p.angle * p.len +=> p.vAngle;
        angle_acc.val() * p.vx +=> p.vAngle;
        angle_damp.val() *=> p.vAngle; // dampen

        // euler
        p.vx +=> p.x;
        angle_dt.val() * p.vAngle +=> p.angle;

    }

    // camera to pillar
    .1 => float zeno;
    M.lerp(zeno, GG.camera().posX(), p.x) => GG.camera().posX;


    // draw
    {
        M.rot2vec(p.angle + Math.pi/2) => vec2 dir;
        @(p.x, 0) => vec2 base;
        base + dir*p.len => vec2 top;
        g.capsuleFilled(base, base + dir*p.len, .1, Color.WHITE);

        // detect death
        if (alive && top.y < 0) {
            false => alive;
        }

        // floor
        g.box(@(p.x, -.25 * g.screen_h), 2 * g.screen_w, .5 * g.screen_h);

        // floor markers (every N meters)
        // simpler: just draw every 5, 1 before and after. nearest multiple
        p.x $ int => int cx;
        g.screen_w $ int => int n_markers;
        for (-n_markers => int i; i < n_markers; i++) {
            // g.line(@(cx + i, 0), @(cx + i, - .25));
            .04 => float r;
            if ((cx + i) % 5 == 0) M.ROOT2 *=> r;
            g.circleFilled(@(cx + i, -.5 * r), r);
        }


        // draw angular speedometer
        {
            .1 => float speedometer_hh;
            1.5 => float speedometer_hw;

            .5 * (g.screen_min.x + g.screen_max.x) => float cx;

            // g.box(@(p.x, 3), 2* speedometer_hw, 2*speedometer_hh, Color.WHITE);
            g.line(@(cx, 3 + speedometer_hh), @(cx, 3 - speedometer_hh), Color.WHITE);

            acc_x * speedometer_hw => float meter_x;
            <<< last_pitch, midi_pitch, acc_x >>>;
            if (pitch_detected) {
                g.boxFilled(
                    @(cx + meter_x * .5, 3),
                    Math.fabs(meter_x),
                    2*speedometer_hh
                );
            }
        }
    }
}