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
10000 => int NUM_VERTICES;
float positions[NUM_VERTICES * 3];
float colors[NUM_VERTICES * 4];

// randomly assign colors
for (0 => int i; i < NUM_VERTICES; i++) {
    Math.random2f(-1.0, 1.0) => positions[3*i + 0];
    Math.random2f(-1.0, 1.0) => positions[3*i + 1];
    Math.random2f(-1.0, 1.0) => positions[3*i + 2];

    1 => colors[4*i + 0];
    1 => colors[4*i + 1];
    1 => colors[4*i + 2];
    1 => colors[4*i + 3];  // alpha always 1
    // Math.random2f(0.0, 1.0) => colors[i];
}

lineGeo.positions(positions);
lineGeo.colors(colors);

GScene scene;
LineMaterial lineMat;
GMesh mesh;
mesh.set(lineGeo, lineMat);

mesh.position(@(0.0, 0.0, -5.0));
mesh --> scene;

// testers =====================

fun void cycleLineMode(LineMaterial @ mat) {
    [LineMaterial.LINE_SEGMENTS, LineMaterial.LINE_LOOP, LineMaterial.LINE_STRIP] @=> int line_modes[];
    while (true) {
        for (0 => int i; i < line_modes.size(); i++) {
            line_modes[i] => mat.lineMode;
            2::second => now;
        }
    }
} spork ~ cycleLineMode(lineMat);

fun void lerpLineWidth(LineMaterial @ mat) {
    while (true) {
        1.0 => mat.lineWidth;
        2::second => now;
        5.0 => mat.lineWidth;
        2::second => now;
        10.0 => mat.lineWidth;
        2::second => now;
    }
} spork ~ lerpLineWidth(lineMat);

fun void cycleLineColor(LineMaterial @ mat) {
    1::second => dur waitTime;
    while (true) {
        @(0.0, 1.0, 0.0) => mat.color;
        waitTime => now;
        @(1.0, 0.0, 0.0) => mat.color;
        waitTime => now;
        @(0.0, 0.0, 1.0) => mat.color;
        waitTime => now;
    }

} spork ~ cycleLineColor(lineMat);

fun void randomizePositions(CustomGeometry @ geo)
{
    while (true) {
    // while (.1::second => now) {
        // 1::second => now;
        // fine on windows, borked on mac
        for (0 => int i; i < NUM_VERTICES; i++) {
            Math.random2f(-1.0, 1.0) => positions[3*i + 0];
            Math.random2f(-1.0, 1.0) => positions[3*i + 1];
            Math.random2f(-1.0, 1.0) => positions[3*i + 2];
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