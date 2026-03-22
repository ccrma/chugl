@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/spring.ck"
@import "../lib/M.ck"
@import "common.ck"

FlatMaterial mat;
PlaneGeometry plane;

// scene setup
GG.camera().orthographic();
GG.camera().viewSize() => float SCREEN_H;
GG.rootPass() --> new ScenePass(GG.scene());
GG.outputPass().gamma(true);

SCREEN_H * .6 => float phone_h;
GMesh mesh(plane, mat) --> GG.scene();
mesh.sca( phone_h * @(9/16.0, 1));

class GameA extends BaseGame {
    GPlane plane --> this;
    plane.color(Color.GREEN);
}

class GameB extends BaseGame {
    GPlane plane --> this;
    plane.color(Color.BLUE);
}

while (1) {
    GG.nextFrame() => now;
}