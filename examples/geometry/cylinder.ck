//-----------------------------------------------------------------------------
// name: cylinder.ck
// desc: Showcase cylinder geometry
//
// source: implementation derived from three.js
// https://github.com/mrdoob/three.js/blob/master/src/geometries/CylinderGeometry.js
//
// requires: ChuGL + chuck-1.5.1.7 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// Helper class for holding cylinder geometry parameters ======================
class CylinderParams {
    .2 => float radiusTop;
    .2 => float radiusBottom;
    1.0 => float height;
    8 => int radialSegments;
    1 => int heightSegments;
    false => int openEnded;
    0.0 => float thetaStart;
    Math.TWO_PI => float thetaLength;
}
CylinderParams params;

fun void updateParams(CylinderGeometry @ geo) {
    geo.set(
        params.radiusTop, params.radiusBottom, params.height,
        params.radialSegments, params.heightSegments,
        params.openEnded, params.thetaStart, params.thetaLength
    );
}

// Create 3 meshes, rendering the geometry's normal data, UV data, and wireframe
GMesh normalsMesh, uvMesh;
CylinderGeometry geo;
updateParams(geo);  // set initial geometry params
// BoxGeometry geo;
NormalsMaterial normMat;
MangoUVMaterial mangoMat;

// wireframe mesh
GCylinder lineMesh --> GG.scene();
lineMesh.geo(geo);  // share same geometry
lineMesh.mat().polygonMode(Material.POLYGON_LINE);

// normal mesh
normalsMesh --> GG.scene();
normalsMesh.set(geo, normMat);

// uv Mesh
uvMesh --> GG.scene();
uvMesh.set(geo, mangoMat);

// place meshes side by side in a row
normalsMesh.posX(-1.8);
uvMesh.posX(0);
lineMesh.posX(1.8);

// UI Setup ===================================================================
UI_Window window;
window.text("Cylinder Geometry Params");

UI_SliderFloat radiusTopSlider;
radiusTopSlider.text("radiusTop");
radiusTopSlider.range(0.01, 3.0);
radiusTopSlider.val(params.radiusTop);

UI_SliderFloat radiusBottomSlider;
radiusBottomSlider.text("radiusBottom");
radiusBottomSlider.range(0.01, 3.0);
radiusBottomSlider.val(params.radiusBottom);

UI_SliderFloat heightSlider;
heightSlider.text("height");
heightSlider.range(0.01, 3.0);
heightSlider.val(params.height);

UI_SliderInt radialSegmentsSlider;
radialSegmentsSlider.text("radialSegments");
radialSegmentsSlider.range(3, 32);
radialSegmentsSlider.val(params.radialSegments);

UI_SliderInt heightSegmentsSlider;
heightSegmentsSlider.text("heightSegments");
heightSegmentsSlider.range(1, 32);
heightSegmentsSlider.val(params.heightSegments);

UI_Checkbox openEndedCheckbox;
openEndedCheckbox.text("openEnded");
openEndedCheckbox.val(params.openEnded);

UI_SliderFloat thetaStartSlider;
thetaStartSlider.text("thetaStart");
thetaStartSlider.range(0.0, Math.TWO_PI);
thetaStartSlider.val(params.thetaStart);

UI_SliderFloat thetaLengthSlider;
thetaLengthSlider.text("thetaLength");
thetaLengthSlider.range(0.0, Math.TWO_PI);
thetaLengthSlider.val(params.thetaLength);

// add to window
window.add(radiusTopSlider);
window.add(radiusBottomSlider);
window.add(heightSlider);
window.add(radialSegmentsSlider);
window.add(heightSegmentsSlider);
window.add(openEndedCheckbox);
window.add(thetaStartSlider);
window.add(thetaLengthSlider);

// UI Listeners ===============================================================

// radiusTop
fun void radiusTopListener() {
    while (true) {
        radiusTopSlider => now;
        radiusTopSlider.val() => params.radiusTop;
        updateParams(geo);
    }
} spork ~ radiusTopListener();

// radiusBottom
fun void radiusBottomListener() {
    while (true) {
        radiusBottomSlider => now;
        radiusBottomSlider.val() => params.radiusBottom;
        updateParams(geo);
    }
} spork ~ radiusBottomListener();

// height
fun void heightListener() {
    while (true) {
        heightSlider => now;
        heightSlider.val() => params.height;
        updateParams(geo);
    }
} spork ~ heightListener();

// radialSegments
fun void radialSegmentsListener() {
    while (true) {
        radialSegmentsSlider => now;
        radialSegmentsSlider.val() => params.radialSegments;
        updateParams(geo);
    }
} spork ~ radialSegmentsListener();

// heightSegments
fun void heightSegmentsListener() {
    while (true) {
        heightSegmentsSlider => now;
        heightSegmentsSlider.val() => params.heightSegments;
        updateParams(geo);
    }
} spork ~ heightSegmentsListener();

// openEnded
fun void openEndedListener() {
    while (true) {
        openEndedCheckbox => now;
        openEndedCheckbox.val() => params.openEnded;
        updateParams(geo);
    }
} spork ~ openEndedListener();

// thetaStart
fun void thetaStartListener() {
    while (true) {
        thetaStartSlider => now;
        thetaStartSlider.val() => params.thetaStart;
        updateParams(geo);
    }
} spork ~ thetaStartListener();

// thetaLength
fun void thetaLengthListener() {
    while (true) {
        thetaLengthSlider => now;
        thetaLengthSlider.val() => params.thetaLength;
        updateParams(geo);
    }
} spork ~ thetaLengthListener();

// Game loop ================================================================
GCamera camera;
1.0 => float camSpeed;
4.0 => float camMovement;
while (true) {
    // camera controls
    GG.mouseX() / GG.windowWidth() => float mouseX;
    GG.mouseY() / GG.windowHeight() => float mouseY;
    // normalize mouse coordinates to [-1, 1], scale by camMovement
    camMovement * (mouseX * 2.0 - 1.0) => mouseX;
    -camMovement * (mouseY * 2.0 - 1.0) => mouseY;

    camSpeed * (mouseX - camera.posX()) + camera.posX() => camera.posX;
    camSpeed * (mouseY - camera.posY()) + camera.posY() => camera.posY;
    camera.lookAt( GG.scene().pos() );

    // rotate the meshes
    GG.dt() * .8 => float dt;
    dt => normalsMesh.rotateY; dt * .17 => normalsMesh.rotateX;
    dt => uvMesh.rotateY;      dt * .17 => uvMesh.rotateX;
    dt => lineMesh.rotateY;    dt * .17 => lineMesh.rotateX;
    GG.nextFrame() => now; 
}


