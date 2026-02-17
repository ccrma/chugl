/*
a327ex Downwell Trails blog: https://github.com/a327ex/blog/issues/9
*/

@import "../lib/g2d/ChuGL.chug"
// @import "../lib/g2d/ChuGL-debug.chug"
@import "../lib/g2d/g2d.ck"

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
vec2 hist[12]; // 12 frames at 60fps = .2 seconds
int hist_idx;
fun void addHist(vec2 p) {
    if (hist_idx >= hist.size()) 0 => hist_idx;
    p => hist[hist_idx++];
}

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;
    g.mousePos() => vec2 mouse_pos;

    // update
    addHist(mouse_pos);

    // draw
    for (auto p : hist) {
        trail_g.circleFilled(p, .5, Color.WHITE);
    }

    { // trail subtract
        res_x / 2 => int NUM_BARS; // draw black strip every other line
        g.screen_w / NUM_BARS => float dx;
        -.5 * g.screen_w => float x;
        g.pushLayer(1); // to draw on *top* of trail
        g.pushColor(Color.BLACK);

        // TODO test with non-black background and multiply blend mode
        for (int i; i < NUM_BARS; i++) {
            trail_g.line(
                @(x, -g.screen_h),
                @(x, g.screen_h),
                Color.BLACK
            );
            dx +=> x;
        }

        g.popColor();
        g.popLayer();
    }

    g.circleFilled(mouse_pos, .5, Color.WHITE);
}