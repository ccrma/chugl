// jello.ck

// audio setup (envelope follower) =====================
adc => Gain g => OnePole p => blackhole;
// square the input
adc => g;
// multiply
3 => g.op;
// filter pole position
0.9995 => p.pole;

// scenegraph setup ====================================
GCube cube --> GG.scene();

// Game Loop ===========================================
while (true) {
    // update logic
    5*p.last() + 1 => cube.sca;
    .3*p.last() => cube.rotateZ;
    GG.dt() => cube.rotateX;

    // render
    GG.nextFrame() => now;
}
