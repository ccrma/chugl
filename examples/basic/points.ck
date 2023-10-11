// Scene setup ===================
GG.scene().backgroundColor(@(0,0,0));
GPoints points --> GG.scene();
// GG.camera().lookAt(@(1, 1, 1));
GG.camera().position(15 * @(0, 0, 1));
GG.camera().lookAt(@(0, 0, 0));

// points stress test
100 => int POINTS_PER_AXIS;

// prepare vertex data for 1,000,000 points!
POINTS_PER_AXIS * POINTS_PER_AXIS * POINTS_PER_AXIS => int numPoints;
float pointPos[numPoints * 3];
float pointColor[numPoints * 4];

// populate within a 10x10x10 cube
for (int i; i < POINTS_PER_AXIS; i++) {
for (int j; j < POINTS_PER_AXIS; j++) {
for (int k; k < POINTS_PER_AXIS; k++) {
    // get the index of this vertex
    i * POINTS_PER_AXIS * POINTS_PER_AXIS + j * POINTS_PER_AXIS + k => int index;

    1.0 * POINTS_PER_AXIS => float colorScale;
    POINTS_PER_AXIS / 10.0 => float posScale;
    // caculate position
    i / colorScale => float x;
    j / colorScale => float y;
    k / colorScale => float z;

    // set position of this vertex
    (posScale * x) - (posScale / 2.0) => pointPos[3*index + 0];
    (posScale * y) - (posScale / 2.0) => pointPos[3*index + 1];
    (posScale * z) - (posScale / 2.0) => pointPos[3*index + 2];
    
    // set color of this vertex
    // (same as position, so have 1:1 mapping between xyz physical space and rgb color space) 
    x => pointColor[4*index + 0];
    y => pointColor[4*index + 1];
    z => pointColor[4*index + 2];
    1 => pointColor[4*index + 3];  // alpha always 1
}}}

points.geo().positions(pointPos);  // set vertex position data
points.geo().colors(pointColor);   // set vertex color data

// toggle size attentuation (do points get smaller as they get further away?)
fun void toggleAttenuation() {
    while (1::second => now)
        1 - points.mat().attenuatePoints() => points.mat().attenuatePoints;
} 
// spork ~ toggleAttenuation();

// camera update
fun void updateCamera(float dt) {
    // calculate position of mouse relative to center of window
    (GG.windowWidth() / 2.0 - GG.mouseX()) / 100.0 => float mouseX;
    (GG.windowHeight() / 2.0 - GG.mouseY()) / 100.0 => float mouseY;

    // update camera position
    10 * Math.cos(-.18 * (now/second)) + 10 => float radius; 
    radius => GG.camera().posZ;
    -mouseX * radius * .07   => GG.camera().posX;
    mouseY * radius * .07  => GG.camera().posY;

    // look at origin
    GG.camera().lookAt( @(0,0,0) ); 
}


// Game loop =====================
while (true) { 
    // camera controls
    updateCamera(GG.dt());

    // rotate points
    GG.dt() * .11 => points.rotX;

    // draw!
    GG.nextFrame() => now; 
}