// basic scene setup ================================================
GCube cube --> GG.scene();
cube.mat().color(Color.DARKBLUE);
GG.camera().posZ(5);

// ui setup ==========================================================
UI_Window window;
// window.text("");

UI_SliderFloat alphaslider;
alphaslider.text("Float Slider! Sets cube alpha");
alphaslider.range(0, 1.0);

window.add(alphaslider);

fun void sliderListener() {
    while (true) {
        alphaslider => now;
        cube.mat().alpha(alphaslider.val());
        <<< "alph: " + cube.mat().alpha() >>>;
    }
} 
spork ~ sliderListener();

// Game loop =========================================================
while (true) { 
    // rotate center cube
    GG.dt() => cube.rotateZ;
    GG.nextFrame() => now; 
}