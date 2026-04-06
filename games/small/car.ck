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

GG.outputPass().gamma(true);
g.backgroundColor(Color.GRAY);

GWindow.mouseMode(GWindow.MOUSE_DISABLED);

TextureLoadDesc desc;
true => desc.flip_y;

<<< "load desc flip_y: ", desc.flip_y >>>;
Texture.load(me.dir() + "./assets/car.png", desc) @=> Texture car_tex;

vec2 last_mouse;
int frame;
while (1) {
    GG.nextFrame() => now;
    g.mousePos() => vec2 mouse_pos;


    // compute mouse angle
    // Math.remap(M.angle(last_mouse, mouse_pos)+ 2 * M.PI, M.PI, 3 * M.PI, 0, 1) => float angle;

    // only update frame if mouse has moved sufficient amount
    if (M.dist2(mouse_pos, last_mouse) > .001) {
        Math.remap(M.angle(last_mouse, mouse_pos), -M.PI, M.PI, -.5, .5) => float angle;
        <<< (angle * 16) $ int >>>;
        (angle * 16) $ int => frame;
        if (frame < 0) 16 +=> frame;
    }


    spork ~ smokeCloud(mouse_pos, Color.WHITE, Math.pow(M.dist(mouse_pos, last_mouse) * 5, .4));

	g.sprite(car_tex, 16, frame, mouse_pos, 1);

    mouse_pos => last_mouse;
}

fun void smokeCloud(vec2 pos, vec3 color, float sca) {
    // params
    2 => int num;

    .4::second => dur max_dur;
    vec2 positions[num];
    vec2 dir[num];
    float radius[num];
    dur durations[num];
    float velocities[num];

    // init
    for (int i; i < num; i++) {
        pos => positions[i];
        M.randomDir() => dir[i];
        sca * Math.random2f(.08, .11) => radius[i];
        Math.random2f(.3, max_dur/second)::second => durations[i];
        .5  * Math.random2f(.01, .02) => velocities[i];
    }

    dur elapsed_time;
    while (elapsed_time < max_dur) {
        GG.nextFrame() => now;
        GG.dt()::second +=> elapsed_time;

        for (int i; i < num; i++) {
            // update line 
            (elapsed_time) / durations[i] => float t;


            // if animation still in progress for this line
            if (t < 1) {
                // update position
                velocities[i] * dir[i] +=> positions[i];
                // shrink radii down to 0
                radius[i] * M.easeOutQuad(1 - t) => float r;
                // draw
                g.circleFilled(positions[i], r, color);
            }
        }
    }
}