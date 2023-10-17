// basic scene setup ================================================

GCube cube --> GG.scene();
cube.mat().color(Color.DARKBLUE);
GG.camera().posZ(5);

GCube childCubes[0];

// ui setup ==========================================================
GUI_Window window;
window.label("ChuGL GUI Window");

GUI_Button button;
button.label("Click me! To change color of cube");

GUI_FloatSlider fslider;
fslider.label("Float Slider! Rotates center cube");
fslider.range(0, Math.PI * 2);

GUI_IntSlider islider;
islider.label("Int Slider! Changes number of children");
islider.range(0, 10);

GUI_Checkbox checkbox;
checkbox.label("Check box! to connect / disconnect children");

GUI_Color3 color;
color.label("Color! Changes background color");

window.add(button);
window.add(fslider);
window.add(islider);
window.add(checkbox);
window.add(color);


fun void ButtonListener(GUI_Button @ button) {
    while (true) {
        button => now;
        cube.mat().color(Color.random());
    }
} spork ~ ButtonListener(button);


fun void FSliderListener(GUI_FloatSlider @ fslider) {
    while (true) {
        fslider => now;
        <<< "fslider: " + fslider.val() >>>;
        cube.rotation(@(fslider.val(), 0, 0));
    }
} 
spork ~ FSliderListener(fslider);

fun void ISliderListener(GUI_IntSlider @ islider) {
    while (true) {
        islider => now;
        <<< "islider: " + islider.val() >>>;
        islider.val() => int numChildren;
        // set number of children to islider.val()

        // case where we need to add children
        while (childCubes.size() < numChildren) {
            GCube childCube;
            childCube.scale(@(0.25, 0.25, 0.25));
            childCube.mat().color(Color.random());
            childCubes << childCube;  // append
            if (checkbox.val()) {
                // connect to scenegraph
                childCube --> cube;
            }
        } 

        // case where we need to remove children
        while (childCubes.size() > numChildren) {
            // get last element
            childCubes[childCubes.size()-1] @=> GCube @ childCube;
            // remove last element
            childCubes.popBack();
            // disconnect from SG
            childCube --< cube;
        } 

        // set positions around circumference
        for (0 => int i; i < childCubes.size(); i++) {
            ( i$float / childCubes.size() ) * Math.PI * 2.0 => float angle;
            childCubes[i].position(@(Math.cos(angle) * 2, Math.sin(angle) * 2, 0));
        }
        
    }
} spork ~ ISliderListener(islider);

fun void CheckboxListener(GUI_Checkbox @ checkbox) {
    while (true) {
        checkbox => now;
        // connect all children
        if (checkbox.val()) {
            for (auto c : childCubes) {
                c --> cube;
            }
        // disconnect all children
        } else {
            for (auto c : childCubes) {
                c --< cube;
            }
        }
    }
} spork ~ CheckboxListener(checkbox);

fun void ColorListener(GUI_Color3 @ color) {
    GG.scene().backgroundColor(color.val());
    while (true) {
        color => now;
        GG.scene().backgroundColor(color.val());
    }
} spork ~ ColorListener(color);



while (true) { 
    // rotate center cube
    GG.dt() => cube.rotZ;

    // rotate children
    for (auto c : childCubes) {
        -3 * GG.dt() => c.rotZ;
    }

    GG.nextFrame() => now; 
}