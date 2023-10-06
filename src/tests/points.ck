InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

GG.fullscreen();
GG.lockCursor();

// Scene setup ===================

GScene scene;
PointsMat pointsMat;  pointsMat.pointSize(55.0);
NormMat normMat;
GMesh pointMesh;

// points stress test
100 => int POINTS_PER_AXIS;
CustomGeometry pointsGeo;
float pointPos[POINTS_PER_AXIS * POINTS_PER_AXIS * POINTS_PER_AXIS * 3];
float pointColor[pointPos.size()];

// populate a 1x1x1 cube
for (int i; i < POINTS_PER_AXIS; i++) {
for (int j; j < POINTS_PER_AXIS; j++) {
for (int k; k < POINTS_PER_AXIS; k++) {
    i * POINTS_PER_AXIS * POINTS_PER_AXIS + j * POINTS_PER_AXIS + k => int index;

    1.0 * POINTS_PER_AXIS => float colorScale;
    POINTS_PER_AXIS / 10.0 => float posScale;
    // caculate position
    i / colorScale => float x;
    j / colorScale => float y;
    k / colorScale => float z;

    // set position
    posScale * x => pointPos[3*index + 0];
    posScale * y => pointPos[3*index + 1];
    posScale * z => pointPos[3*index + 2];
    
    // set color (same as position)
    x => pointColor[3*index + 0];
    y => pointColor[3*index + 1];
    z => pointColor[3*index + 2];
}}}

<<< "Rendering ", pointPos.size()/3, " points!" >>>;

pointsGeo.positions(pointPos);
pointsGeo.colors(pointColor);

pointMesh.set(pointsGeo, pointsMat);
pointMesh --> scene;

// toggle size attentuation
fun void toggleAttenuation() {
    while (1::second => now)
        1 - pointsMat.attenuate() => pointsMat.attenuate;
} // spork ~ toggleAttenuation();


// Game loop =====================

0 => int fc;
now => time lastTime;
while (true) {
    // compute timing
    fc++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    deltaTime/second => float dt;

    // <<< "fc: ", fc++ , "now: ", now, "dt: ", dt, "dSamps: ", deltaTime >>>;

    GG.nextFrame() => now;
}