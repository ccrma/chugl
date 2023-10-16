// basic scene setup ================================================

GCube cube --> GG.scene();
cube.mat().color(Color.DARKBLUE);
GG.camera().posZ(5);



// ui setup ==========================================================
GUI_Window window;
window.label("ChuGL GUI Window");

GUI_Button button;
button.label("Click me! To change color of cube");

GUI_Checkbox checkbox;
checkbox.label("Check box! to connect / disconnect cube");

GUI_Slider slider;
slider.label("Slider! Rotates Cube");
slider.range(0, Math.PI * 2);

GUI_Color3 color;
color.label("Color! Changes background color");

window.add(button);
window.add(checkbox);
window.add(slider);
window.add(color);


fun void ButtonListener(GUI_Button @ button) {
    while (true) {
        button => now;
        cube.mat().color(Color.random());
    }
} spork ~ ButtonListener(button);

fun void CheckboxListener(GUI_Checkbox @ checkbox) {
    while (true) {
        checkbox => now;
        if (checkbox.val()) {
            cube --> GG.scene();
        } else {
            cube --< GG.scene();
        }
    }
} spork ~ CheckboxListener(checkbox);

fun void SliderListener(GUI_Slider @ slider) {
    while (true) {
        slider => now;
        <<< "slider: " + slider.val() >>>;
        cube.rotation(@(slider.val(), 0, 0));
    }
} 
spork ~ SliderListener(slider);

fun void ColorListener(GUI_Color3 @ color) {
    GG.scene().backgroundColor(color.val());
    while (true) {
        color => now;
        GG.scene().backgroundColor(color.val());
    }
} spork ~ ColorListener(color);

while (true) { GG.nextFrame() => now; }