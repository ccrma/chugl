//-----------------------------------------------------------------------------
// name: custom-fx.ck
// desc: demos the use of CustomFX with your own glsl screen shaders
//       
// requires: ChuGL 0.1.6 + chuck-1.5.2.0 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// Setup FX chain ============================================
GG.fx() --> CustomFX colorSeparator --> CustomFX tesselator;
colorSeparator.shaderPath(me.dir() + "./color-separation.glsl");
tesselator.shaderPath(me.dir() + "./tesselate.glsl");

colorSeparator.setFloat2("u_Separation", @(0.02, 0.02));
tesselator.setInt2("u_Tesselation", @(4, 4));

// Scene setup ===============================================
GCube cube --> GG.scene();
Math.PI/4 => cube.rotZ;

// UI ========================================================
UI_Window window;
window.text("CustomFX Demo");

UI_SliderFloat2 sliderSeparation;
sliderSeparation.text("Color Separation Amount");
sliderSeparation.range(0, .1);
sliderSeparation.val(colorSeparator.getFloat2("u_Separation"));

UI_SliderInt2 sliderTesselation;
sliderTesselation.text("Tesselation Amount");
sliderTesselation.range(0, 50);
sliderTesselation.val(tesselator.getInt2("u_Tesselation"));

window.add(sliderSeparation);
window.add(sliderTesselation);

// Event Listeners ===========================================

fun void SliderSeparationListener() {
    while (true) {
        sliderSeparation => now;
        colorSeparator.setFloat2("u_Separation", sliderSeparation.val());
    }
}
spork ~ SliderSeparationListener();

fun void SliderTesselationListener() {
    while (true) {
        sliderTesselation => now;
        tesselator.setInt2("u_Tesselation", sliderTesselation.val());
    }
}
spork ~ SliderTesselationListener();

// Game loop =================================================
while (true) { 
    cube.rotateOnWorldAxis(@(0, 1, 0), .2 * GG.dt());
    GG.nextFrame() => now; 
}
