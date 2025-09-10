//--------------------------------------------------------------------
// name: points.ck
// desc: 1,000,000 points with color; use WASD to move through
// requires: ChuGL + chuck-1.5.5.5 or higher
//
// author: Andrew Zhu Aday
//  date: Fall 2023
//--------------------------------------------------------------------

// scene setup ===================
GPoints points --> GG.scene();
// set background color
GG.scene().backgroundColor(@(0,0,0));

// choose mouse mode
GWindow.mouseMode( GWindow.MOUSE_DISABLED );
// set fly camera as main camera
GFlyCamera cam => GG.scene().camera;
// set camera position
cam.posZ(10);

// points stress test
100 => int POINTS_PER_AXIS;

// prepare vertex data for 1,000,000 points!
POINTS_PER_AXIS * POINTS_PER_AXIS * POINTS_PER_AXIS => int numPoints;
vec3 pointPos[numPoints];
vec3 pointColor[numPoints];

// populate within a 100x100x100 cube
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
    @(
        posScale * x - posScale / 2.0, 
        posScale * y - posScale / 2.0, 
        posScale * z - posScale / 2.0
    ) => pointPos[index];
    
    // set color of this vertex
    // (same as position, so have 1:1 mapping between xyz physical space and rgb color space) 
    @(
        x, 
        y, 
        z
    ) => pointColor[index];
}}}

// set vertex position data
points.positions(pointPos);
// set vertex color data
points.colors(pointColor);

// render loop
while (true)
{
    // synchronize 
    GG.nextFrame() => now; 

    // begin UI
    if (UI.begin("Points"))
    { UI.text("WASD to move"); }

    // end UI
    UI.end();
}
