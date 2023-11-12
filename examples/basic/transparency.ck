//-----------------------------------------------------------------------------
// name: transparency.ck
// desc: Transparency example showing use of transparent materials and alpha 
//       values.
//
// requires: ChuGL + chuck-1.5.1.7 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// Scene setup ================================================================
GPlane a, b, c;
a --> b --> c --> GGen group --> GG.scene();
GG.scene().light().ambient(Color.WHITE);
2 => GG.camera().posZ;

// stack transparent planes on top of each other
b.sca(.5);
a.sca(.5);
-.1 => b.posZ;
-.1 => a.posZ;

// set material colors
Color.RED => a.mat().color;
Color.GREEN => b.mat().color;
Color.BLUE => c.mat().color;

// set materials to be transparent 
// (this disables depth writing for these meshes and renders them by distance from camera)
1 => a.mat().transparent;
1 => b.mat().transparent;
1 => c.mat().transparent;

// set alpha values for each material
.5 => a.mat().alpha;
.5 => b.mat().alpha;
.5 => c.mat().alpha;

// UI Controls ================================================================
UI_Window window;
window.text("Transparency Example");

// alpha sliders
UI_SliderFloat alphaslider_a, alphaslider_b, alphaslider_c;

alphaslider_a.text("Red Plane alpha");
alphaslider_a.range(0, 1.0);
alphaslider_a.val(a.mat().alpha());

alphaslider_b.text("Green Plane alpha");
alphaslider_b.range(0, 1.0);
alphaslider_b.val(b.mat().alpha());

alphaslider_c.text("Blue Plane alpha");
alphaslider_c.range(0, 1.0);
alphaslider_c.val(c.mat().alpha());

// transparency checkboxes
UI_Checkbox transparency_a, transparency_b, transparency_c;

transparency_a.text("Red plane transparent");
transparency_a.val(a.mat().transparent());

transparency_b.text("Green plane transparent");
transparency_b.val(b.mat().transparent());

transparency_c.text("Blue plane transparent");
transparency_c.val(c.mat().transparent());

// rotation controls
UI_Checkbox rotateCheckbox;
rotateCheckbox.text("Rotate planes");
rotateCheckbox.val(true);

UI_SliderFloat rotateSpeedSlider;
rotateSpeedSlider.text("Rotation speed");
rotateSpeedSlider.range(0, 1.0);
rotateSpeedSlider.val(0.25);

// add controls to window
window.add(alphaslider_a);
window.add(alphaslider_b);
window.add(alphaslider_c);
window.add(transparency_a);
window.add(transparency_b);
window.add(transparency_c);
window.add(rotateCheckbox);
window.add(rotateSpeedSlider);

// UI Listeners ===============================================================

// alpha sliders
fun void sliderListener(Material @ mat, UI_SliderFloat @ slider) {
    while (true) {
        slider => now;
        mat.alpha(slider.val());
    }
} 
spork ~ sliderListener(a.mat(), alphaslider_a);
spork ~ sliderListener(b.mat(), alphaslider_b);
spork ~ sliderListener(c.mat(), alphaslider_c);

// transparency toggles
fun void checkboxListener(Material @ mat, UI_Checkbox @ checkbox) {
    while (true) {
        checkbox => now;
        mat.transparent(checkbox.val());
    }
}
spork ~ checkboxListener(a.mat(), transparency_a);
spork ~ checkboxListener(b.mat(), transparency_b);
spork ~ checkboxListener(c.mat(), transparency_c);

// rotation controls
rotateCheckbox.val() => int shouldRotate;
fun void rotateCheckboxListener(UI_Checkbox @ checkbox) {
    while (true) {
        checkbox => now;
        checkbox.val() => shouldRotate;
    }
} spork ~ rotateCheckboxListener(rotateCheckbox);

rotateSpeedSlider.val() => float rotateSpeed;
fun void rotateSpeedSliderListener(UI_SliderFloat @ slider) {
    while (true) {
        slider => now;
        slider.val() => rotateSpeed;
    }
} spork ~ rotateSpeedSliderListener(rotateSpeedSlider);

// Game Loop ==================================================================
while (true) {
    if (shouldRotate) rotateSpeed * GG.dt() => group.rotateY;
    GG.nextFrame() => now; 
}