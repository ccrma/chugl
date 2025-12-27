@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"
@import "sound.ck"

// PIN CLIMB
// https://abagames.github.io/crisp-game-lib-11-games/?pinclimb

/*
IDEAS
- have resolution match camera viewsize so worldspace units == screenspace pixels
    - lets us get rid of the n2w and w2n stuff
- play with different screen shaders
    - CRT
    - pixellate
    - see ABA themes: https://abagames.github.io/crisp-game-lib/ref_document/types/ThemeName.html
    - dithering would be cool
- expanding on idea (don't do now, move onto other games)
    - music
        - generative background music
            - like Rez, put all SFX on quantized grid
        - OR maybe the sfx are sufficient to create their own musical soundscape
    - different types of pins
        - bomb pins that kill you
        - glass pins that break after one use
        - ghost pins that pop in and out of existence
        - moving pins (diagonal, up-down, left-right)
            - this is harder, messes with collision detection
    - falling powerups
        - aimer: lets you aim the cord and fire directly
            - again this messes with current collision detection
        - slow down time
        - slow down pin falling
        - longer cord

scoring ideas
- connection distance (true to original) equiv to total distance traveled
    - rewards all player movement, incentivies going down to get extra score
    which is more exciting, introduces risk-reward tradeoff
- net vertical distance traveled / height (like doodle jump)
    - doesn't reward player for going down, which is risky 
- time survived: prob least interesting option, likely has lowest variance

Q: why do games use high score as a motivation?

*/

// text
GText.defaultFont("chugl:proggy-tiny");
"pin climb" => string title;
"[press] stretch" => string description;

// sound
CKFXR sfx => dac;
[ 0, 2, 4, 7, 11, 12, 14] @=> int scale[];
60 + 12 => float root;

fun float pos2note(float x) {
    return root + scale[
        Math.remap(
            x, 
            screen_min_y, screen_max_y,
            0, scale.size() - 1
        ) $ int
    ];
}

// indexes scale with octave increase
fun float len2note(float x) {
    return scale[Math.fmod(x, scale.size()) $ int] + Math.floor(x / scale.size()) * 12;
}

PulseOsc square => Gain gain(.2) => dac;
0 => square.gain;
.25 => square.width;

// appearance
1 => float aspect;
320 => int resolution;
GWindow.sizeLimits(0, 0, 0, 0, @(aspect, 1));
GWindow.center();
G2D g;
g.resolution(resolution, (resolution/aspect) $ int );
g.antialias(false);

// utility (TODO maybe move this into g2d)
-.5 * GG.camera().viewSize() => float screen_min_y;
.5 * GG.camera().viewSize()  => float screen_max_y;

// gameplay params
UI_Float pin_fall_speed(.3);
UI_Float pin_radius(.12);
UI_Float screen_zeno(.10);
UI_Float spawn_dist_min(.58);
UI_Float cord_grow_rate(6);
UI_Float cord_base_length(.6);
UI_Float cord_angular_speed(Math.pi);

// gamestate
float gametime; // time in room_play
vec2 pins[];
float cord_len;
float cord_angle;
vec2 cord_pos;
float spawn_dist;
int score;
int highscore;

4.5 => float MARGIN;
GText highscore_text --> GG.scene();
highscore_text.controlPoints(@(1, 1));
highscore_text.pos(@(MARGIN, MARGIN));

GText score_text;
score_text.controlPoints(@(0, 1));
score_text.pos(@(-MARGIN, MARGIN));

0 => int Room_Start;
1 => int Room_Play;
int room;


fun void init() {
    0 => gametime;
    [@(0, 4)] @=> pins;
    cord_base_length.val() => cord_len;
    0 => cord_angle;
    pins[0] => cord_pos;
    spawn_dist_min.val() => spawn_dist;
    0 => score;
}

// circle-arc collision
// assumes delta_theta > 0, and start_theta > 0
fun int collide(vec2 p1, float r1, vec2 p2, float r2, float start_theta, float delta_theta) {
    Math.euclidean(p1, p2) => float dist;
    // check overlap
    if (!(dist < (r1 + r2))) return false;

    T.assert(delta_theta > 0, "invalid delta theta");
    T.assert(start_theta >= 0 && start_theta <= Math.two_pi, "invalid start theta: " + start_theta);

    // see if circle at p1 is in arc section
    M.angle(p2, p1) => float angle;
    Math.atan2(r1, dist) => float delta_angle;

    angle - delta_angle => float t0; // circle
    angle + delta_angle => float t1;
    start_theta => float t2;  // arc
    start_theta + delta_theta => float t3;
    // unwrap to avoid discontinuity at -pi, pi
    if (t0 < 0) {
        Math.two_pi +=> t0;
        Math.two_pi +=> t1;
    }
    T.assert(t0 > 0 && t0 <= Math.two_pi, "invalid t0");
    T.assert(t1 > 0, "invalid t1");

    return M.overlap(t0, t1, t2, t3);
}


init();
// gameloop
while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;
    dt +=> gametime;
    
    // difficulty scaling
    // difficulty incr every 30 seconds instead of crisp-game-lib's typical 60
    (1 + (gametime / 30)) $ int => int difficulty; 

    // score
    highscore_text.text("HI " + highscore);
    score_text.text(score + "");

    UI.text("gametime: " + gametime);
    UI.text("difficulty: " + difficulty);
    UI.text("#effects: " + g.effects_count);
    UI.slider("screen zeno", screen_zeno, 0, 1);

    if (room == Room_Start) {
        g.text(title, @(0, 1 + .2 * Math.sin(3 * (now/second))));
        g.text(description, @(0, -1));
        if (g.anyInputDown()) {
            Room_Play => room;
            init();
            score_text --> GG.scene();
        }
    } else {
        if (g.anyInput()) dt * cord_grow_rate.val() * (.6 + .4 * difficulty) +=> cord_len;
        else (cord_len - cord_base_length.val()) * .1 -=> cord_len;
    }

    { // update and draw
        // map cord_len to pitch
        (cord_len - cord_base_length.val()) => float cord_growth;
        Math.clampf(cord_growth, 0, 1) => square.gain;
        Std.mtof(
            8 * (cord_len - cord_base_length.val()) 
            // len2note(10 * cord_growth)
            + 48
        ) => square.freq;
        
        dt * pin_fall_speed.val() * difficulty => float pin_fall_amt;
        if (cord_pos.y > .55 * screen_min_y) {
            (cord_pos.y - .55 * screen_min_y) * screen_zeno.val() +=> pin_fall_amt;
        }

        // update player
        dt * cord_angular_speed.val() * (.7 + .3 * difficulty) => float delta_angle;
        pin_fall_amt -=> cord_pos.y;

        int climb;
        vec2 next_pos;
        for (pins.size() - 1 => int i; i >= 0; i--) {
            pin_fall_amt -=> pins[i].y;

            if (pins[i].y < screen_min_y - pin_radius.val()) {
                pins.erase(i);
            }
            else {
                (cord_pos == pins[i]) => int same_pin;
                g.circle(pins[i], pin_radius.val(), Color.WHITE);

                if (room == Room_Play) {
                    if (same_pin) g.circleFilled(pins[i], pin_radius.val(), Color.WHITE);
                    if (
                        !same_pin
                        &&
                        // M.isect(cord_pos, cord_pos + M.rot2vec(cord_angle) * cord_len, pins[i], pin_radius.val()).x != 0
                        collide(pins[i], pin_radius.val(), cord_pos, cord_len, cord_angle, delta_angle)
                    ) {
                        pins[i] => next_pos;
                        true => climb;
                    }
                }
            }
        }

        // update cord angle *after* collision detection
        M.unwind(cord_angle + delta_angle) => cord_angle; // clamp to [0, 2pi]

        if (climb) {
            sfx.coin(pos2note(cord_pos.y), pos2note(next_pos.y));
            (10 * M.dist(next_pos, cord_pos)) $ int => int points;
            g.score("+" + points, next_pos, .5::second, .5, pin_radius.val() * (3 + .1 * points));
            points +=> score;

            next_pos => cord_pos;
            cord_base_length.val() => cord_len;

        }

        // spawn logic
        pin_fall_amt -=> spawn_dist;
        while (spawn_dist < 0) {
            pins << g.n2w(M.rnd(-.8, .8), 1);
            spawn_dist +=> pins[-1].y; // adjust y pos based on where it's supposed to spawn
            M.rnd(spawn_dist_min.val(), spawn_dist_min.val() + 1) +=> spawn_dist;
        }

        // draw player
        if (room == Room_Play) {
            g.line(cord_pos, cord_pos + M.rot2vec(cord_angle) * cord_len); 

            // death cond
            if ( cord_pos.y < -5 - 2*pin_radius.val()) {
                0 => cord_len;
                sfx.explosion(.88::second); // fade explode sound slightly faster than visuals
                g.explode(@(cord_pos.x, screen_min_y), 2, 1::second);
                Room_Start => room;

                Math.max(score, highscore) => highscore;
                0 => score;
                score_text.detachParent();
            }
        }

    }
}
