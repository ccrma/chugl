InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

// GG.fullscreen();
// GG.lockCursor();

CustomGeometry lineGeo;
// construct line segment
float positions[50000];

float colors[positions.size()];
// randomly assign colors
for (0 => int i; i < colors.size(); i++) {
    Math.random2f(0.0, 1.0) => colors[i];
}

lineGeo.positions(positions);
lineGeo.colors(colors);

GScene scene;
LineMat lineMat;
GMesh mesh;
mesh.set(lineGeo, lineMat);

mesh.position(@(0.0, 0.0, -5.0));
mesh --> scene;

// testers =====================

fun void cycleLineMode(LineMat @ mat) {
    [LineMat.LINE_SEGMENTS, LineMat.LINE_LOOP, LineMat.LINE_STRIP] @=> int line_modes[];
    while (true) {
        for (0 => int i; i < line_modes.size(); i++) {
            line_modes[i] => mat.mode;
            2::second => now;
        }
    }
} spork ~ cycleLineMode(lineMat);

fun void lerpLineWidth(LineMat @ mat) {
    while (true) {
        1.0 => mat.width;
        2::second => now;
        5.0 => mat.width;
        2::second => now;
        10.0 => mat.width;
        2::second => now;
    }
} spork ~ lerpLineWidth(lineMat);

fun void cycleLineColor(LineMat @ mat) {
    while (true) {
        @(1.0, 0.0, 0.0) => mat.color;
        2::second => now;
        @(0.0, 1.0, 0.0) => mat.color;
        2::second => now;
        @(0.0, 0.0, 1.0) => mat.color;
        2::second => now;
    }

} spork ~ cycleLineColor(lineMat);

fun void randomizePositions(CustomGeometry @ geo)
{
    while (true) {
        // 1::second => now;
        for (0 => int i; i < positions.size(); i++) {
            Math.random2f(-1.0, 1.0) => positions[i];
        }
        geo.positions(positions);
        GG.nextFrame() => now;
    }
}
spork ~ randomizePositions(lineGeo);


// Game loop =====================

0 => int frameCounter;
now => time lastTime;
while (true) {
    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    deltaTime/second => float dt;

    GG.nextFrame() => now;
}