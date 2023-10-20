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
PointMaterial pointsMat;  pointsMat.pointSize(55.0);
NormalsMaterial normMat;
GMesh pointMesh;

// points stress test
100 => int POINTS_PER_AXIS;
CustomGeometry pointsGeo;
POINTS_PER_AXIS * POINTS_PER_AXIS * POINTS_PER_AXIS => int numPoints;
float pointPos[numPoints * 3];
float pointColor[numPoints * 4];

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
    // setting color in hsv
    x * 360.0 => float hue;
    y => float sat;
    z => float val;

    // convert to rgb
    Color.hsv2rgb(@(hue, sat, val)) => vec3 rgb;
    @(x, y, z) => rgb;
    
    rgb.x => pointColor[4*index + 0];
    rgb.y => pointColor[4*index + 1];
    rgb.z => pointColor[4*index + 2];

    1 => pointColor[4*index + 3];  // alpha always 1
}}}

<<< "Rendering ", pointPos.size()/3, " points!" >>>;

pointsGeo.poss(pointPos);
pointsGeo.colors(pointColor);

pointMesh.set(pointsGeo, pointsMat);
pointMesh --> scene;

// toggle size attentuation
fun void toggleAttenuation() {
    while (1::second => now)
        1 - pointsMat.attenuatePoints() => pointsMat.attenuatePoints;
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