//-----------------------------------------------------------------------------
// name: triangle.ck
// desc: Showcase triangle geometry
//
// requires: ChuGL + chuck-1.5.1.8 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// Helper class for holding cylinder geometry parameters ======================
class TriangleParams {
    Math.PI / 3.0 => float theta;
    1.0 => float width;
    .866 => float height;
}
TriangleParams params;

fun void updateParams(Geometry @ geo) {
    (geo $ TriangleGeometry).set(
        params.theta,
        params.width,
        params.height
    );
}

// connect triangle mesh to scene
GTriangle triangle --> GG.scene();

// place small circle at origin (helps show triangle is centered)
GCircle circle --> GG.scene();
circle.mat().color(Color.RED);
circle.sca(.01);

// set initial geometry params
updateParams(triangle.geo());  

// initialize materials
PhongMaterial phongMat;
NormalsMaterial normMat;
MangoUVMaterial mangoMat;

// UI Setup ===================================================================
UI_Window window;
window.text("Triangle Geometry Params");

UI_Dropdown dropdown;
dropdown.text("Material");
dropdown.options(["shaded", "normals", "uv"]);

UI_SliderFloat thetaSlider;
thetaSlider.text("theta");
thetaSlider.range(0.001, Math.PI);
thetaSlider.val(params.theta);

UI_SliderFloat widthSlider;
widthSlider.text("width");
widthSlider.range(0.01, 3.0);
widthSlider.val(params.width);

UI_SliderFloat heightSlider;
heightSlider.text("height");
heightSlider.range(0.01, 3.0);
heightSlider.val(params.height);

// add to window
window.add(dropdown);
window.add(thetaSlider);
window.add(widthSlider);
window.add(heightSlider);

// UI Listeners ===============================================================

// material dropdown
fun void dropdownListener() {
    while (true) {
        dropdown => now;
        if (dropdown.val() == 0) {
            triangle.mat(phongMat);
        } else if (dropdown.val() == 1) {
            triangle.mat(normMat);
        } else if (dropdown.val() == 2) {
            triangle.mat(mangoMat);
        }
    }
} spork ~ dropdownListener();

// theta 
fun void thetaListener() {
    while (true) {
        thetaSlider => now;
        thetaSlider.val() => params.theta;
        updateParams(triangle.geo());
    }
} spork ~ thetaListener();

// width
fun void widthListener() {
    while (true) {
        widthSlider => now;
        widthSlider.val() => params.width;
        updateParams(triangle.geo());
    }
} spork ~ widthListener();

// height
fun void heightListener() {
    while (true) {
        heightSlider => now;
        heightSlider.val() => params.height;
        updateParams(triangle.geo());
    }
} spork ~ heightListener();

// Game loop ================================================================
while (true) { GG.nextFrame() => now; }
