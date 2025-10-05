@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"

G2D g;

// GWindow.windowed(1920, 1080);
// g.resolution(1920, 1080);


.2 => float r;
@(-1, 0) => vec2 l1;
@(1, 0) => vec2 l2;
// gameloop

2 => float hw;
3 => float hh;

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    GWindow.scrollY() * dt +=> r;

    g.line(l1, l2);

    g.mousePos() => vec2 circle_pos;
    // g.circleDotted(circle_pos, r, 0, Color.WHITE);
    // <<< "isect? ", M.isect(l1, l2, circle_pos, r) >>>;
    if (M.aabbIsect(
        @(-hw, -hh, hw, hh),
        @(
            g.mousePos().x - .25, g.mousePos().y - .3,
            g.mousePos().x + .25, g.mousePos().y + .3
        )
    )) {
        g.pushColor(Color.RED);
    }
    g.box(@(-hw, -hh), @(hw, hh));
    g.box(
        g.mousePos(), .5, .6
    );




    // g.circleFilled(g.NDCToWorldPos(-1.0, 0), 1.0);
    // g.circleFilled(g.NDCToWorldPos(1.0, 0), 1.0);
    // g.circleFilled(g.NDCToWorldPos(0.0, 1.0), 1.0);
    // g.circleFilled(g.NDCToWorldPos(0.0, -1.0), 1.0);
}
