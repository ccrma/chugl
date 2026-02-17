@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"

G2D g;
G2D trail_g;

// window setup
320 => int res_x;
240 => int res_y;
g.resolution(res_x, res_y);
g.antialias(false);
GWindow.sizeLimits(0, 0, 0, 0, @(160, 120));

// screen shader
g.screenShaderFromPath(me.dir() + "./trail_screen_shader.wgsl") @=> Shader screen_shader;

// render graph
// GG.scene().backgroundColor(Color.ORANGE);
GG.rootPass() --> GG.scenePass() --> ScreenPass screen_pass(screen_shader);
screen_pass.material().sampler(0, TextureSampler.nearest());
screen_pass.material().texture(1, GG.scenePass().colorOutput());

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
        // g.squareFilled(p, 0, 1, Color.WHITE);
        g.circleFilled(p, .5, Color.WHITE);
    }

    { // trail subtract
        res_x / 2 => int NUM_BARS; // draw black strip every other line
        g.screen_w / NUM_BARS => float dx;
        -.5 * g.screen_w => float x;
        g.pushLayer(1); // to draw on *top* of trail
        g.pushColor(Color.BLACK);
        // TODO test with non-black background and multiply blend mode
        for (int i; i < NUM_BARS; i++) {
            g.line(
                @(x, -g.screen_h),
                @(x, g.screen_h)
            );
            dx +=> x;
        }
        g.popColor();
        g.popLayer();
    }

    // cheap way to do trails (doesn't work if there's other background stuff)
    g.pushLayer(2);
    g.circleFilled(mouse_pos, .5, Color.WHITE);
    g.popLayer();

}