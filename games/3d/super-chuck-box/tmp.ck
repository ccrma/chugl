@import "../../lib/g2d/ChuGL-debug.chug"
@import "../../lib/M.ck"
@import "../../lib/T.ck"
@import "./constants.ck"

GOrbitCamera orbit_cam => GG.scene().camera;

CylinderGeometry cylinder_geo(
    .1, .1, 
    1, 
    6, 
    1, 
    false, 
    0, 
    Math.two_pi);

GMesh g(cylinder_geo, new PhongMaterial) --> GGen container --> GG.scene();

g.rotX(-Math.pi/2);
// container.rotX(-Math.pi/2);

<<< 1.0 / 0.0 >>>;
while (1) {
    <<< GG.fc() >>>;
    second => now;

    GG.nextFrame() => now;
    GG.dt() => float dt;
    // // g.rotateY(dt);
    // container.rotateZ(dt);

    // <<< GWindow.windowSize() >>>;
}