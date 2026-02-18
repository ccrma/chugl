/*
a327ex Downwell Trails blog: https://github.com/a327ex/blog/issues/9

this is overcomplicated; with black background, don't need separate renderpass.
Can just layer trail history, black lines, then player body
*/

@import "../lib/g2d/ChuGL.chug"
// @import "../lib/g2d/ChuGL-debug.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"

GScene trail_scene;

G2D g;
G2D trail_g(trail_scene);

// window setup
320 => int res_x;
240 => int res_y;
g.resolution(res_x, res_y); 
g.antialias(false);         
GWindow.sizeLimits(0, 0, 0, 0, @(res_x, res_y));

// TODO: does camera need to be mapped too (between g and trail_g)? Can two GScenes even share the same GCamera?
trail_g.antialias(false);

// screen shader
// g.screenShaderFromPath(me.dir() + "./trail_screen_shader.wgsl") @=> Shader screen_shader;
// ScreenPass main_pass(screen_shader);
// main_pass.name("Main ScreenPass");
// main_pass.material().sampler(0, TextureSampler.nearest());
// main_pass.material().texture(1, g._scene_pass.colorOutput());

// render graph
GG.rootPass() --> trail_g._scene_pass --> g._scene_pass --> GG.outputPass();
GG.outputPass().input(g._scene_pass.colorOutput());
trail_g._scene_pass.colorOutput(g._scene_pass.colorOutput());
false => g._scene_pass.clear;

// trail history
vec4 hist[12]; // 12 frames at 60fps = .2 seconds
int hist_idx;
fun void addHist(vec2 p, float angle) {
    if (hist_idx >= hist.size()) 0 => hist_idx;
    @(p.x, p.y, now/second, angle) => hist[hist_idx++];
}

fun vec2 prevPos() {
    return hist[hist_idx - 2] $ vec2;
}

res_x / 2 => int NUM_BARS; // draw black strip every other line

int draw_extra[NUM_BARS];

fun void drawExtra() {
    while (.1::second => now)
    for (int i; i < draw_extra.size(); ++i)
        (Math.randomf() < .2) => draw_extra[i];
} spork ~ drawExtra();


vec2 pos_displace;
while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;
    GG.fc() => int fc;
    g.mousePos() => vec2 mouse_pos;
    now/second => float t_sec;

    // update
    M.angle(prevPos(), mouse_pos) + Math.pi / 2=> float angle;
    Math.cos(angle) => float cos_a;
    Math.sin(angle) => float sin_a;
    M.dist(prevPos(), mouse_pos) => float speed;
    <<< angle, speed >>>;

    addHist(mouse_pos, angle);

    // draw trail
    Math.remap(speed, 0, .3, 1, .5) => float a_scale;
    for (auto p : hist) {
        // scale down size of old trail
        Math.max(0, 1 - (t_sec - p.z)/.3) => float r;

        // random jitter to radius
        Math.random2f(.9, 1.1) *=> r;

        // random jitter to pos
        p$vec2 => vec2 c;
        // if (fc % 2 == 0) 
        // M.randomPointInCircle(c, 0, .2*r) => c;

        // trail_g.circleFilled(c, r * .5, Color.RED);

        trail_g.pushRot(p.w);
        trail_g.ellipseFilled(c, r * @(a_scale * .5, .5), Color.RED);
        trail_g.popRot();
    } 

    { // trail subtract
        g.screen_w / NUM_BARS => float dx;
        -.5 * g.screen_w => float x;
        trail_g.pushLayer(1); // to draw on *top* of trail
        trail_g.pushColor(Color.BLACK);

        // TODO test with non-black background and multiply blend mode
        for (int i; i < NUM_BARS; i++) {
            trail_g.line(
                M.rotate(@(x, -g.screen_h), cos_a, sin_a),
                M.rotate(@(x, g.screen_h), cos_a, sin_a)
            );
            // if (Math.randomf() < trail_rnd) {
            // if (draw_extra[i]) {
            // if (Math.sin(now/second) < M.fract((2*i * 1.0 / NUM_BARS))) {
            now/second => float t;
            if (M.fract(16*(i + 30*t) $ float / NUM_BARS) < .5 ) {
            // if (maybe) {
                trail_g.line(
                    M.rotate(@(x + .5*dx, -g.screen_h), cos_a, sin_a),
                    M.rotate(@(x + .5*dx, g.screen_h), cos_a, sin_a)
                );
            }
            dx +=> x;
        }

        trail_g.popColor();
        trail_g.popLayer();
    }

    if (fc % 4 == 0) M.randomPointInCircle(@(0,0), 0, .05) => pos_displace;
    // g.circleFilled(mouse_pos + pos_displace, .5 * Math.random2f(.9, 1.02), Color.WHITE);
    g.pushRot(angle); <<< "angle", angle >>>;
    .5 * Math.remap(speed, 0, .3, 1, .5) => float a;
    g.ellipseFilled(mouse_pos + pos_displace, .9 * @(a, .5), Color.WHITE);
    g.popRot();

}