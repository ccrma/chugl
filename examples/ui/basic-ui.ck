//-----------------------------------------------------------------------------
// name: basic-UI.ck
// desc: demos the use of basic UI widgets including buttons, sliders, 
//       checkboxes, and color pickers
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// basic scene setup ================================================
GCube cube --> GG.scene();
cube.mat().color(Color.DARKBLUE);
GG.camera().posZ(5);

GCube childCubes[0];

// ui setup ==========================================================
UI_Window window;
window.text("ChuGL UI Window");

UI_Text text;
text.text("This is a text widget! Below are other widgets.");
text.wrap(true);
text.mode(UI_Text.MODE_DEFAULT);
text.color(Color.GREEN);

UI_Button button;
button.text("Click me! To change color of cube");

UI_SliderFloat fslider;
fslider.text("Float Slider! Rotates center cube");
fslider.range(0, Math.PI * 2);

UI_SliderInt islider;
islider.text("Int Slider! Changes number of children");
islider.range(0, 10);

UI_Checkbox checkbox;
checkbox.text("Check box! to connect / disconnect children");

UI_Color3 color;
color.text("Color! Changes background color");

UI_Dropdown dropdown;
dropdown.text("Dropdown! Select polygon mode");
dropdown.options(["FILL", "LINE", "POINT"]);

window.add(button);
window.add(text);
window.add(fslider);
window.add(islider);
window.add(checkbox);
window.add(color);
window.add(dropdown);


fun void ButtonListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.mat().color(Color.random());
    }
} spork ~ ButtonListener(button);


fun void FSliderListener(UI_SliderFloat @ fslider) {
    while (true) {
        fslider => now;
        <<< "fslider: " + fslider.val() >>>;
        cube.rot(@(fslider.val(), 0, 0));
    }
} 
spork ~ FSliderListener(fslider);

fun void ISliderListener(UI_SliderInt @ islider) {
    while (true) {
        islider => now;
        <<< "islider: " + islider.val() >>>;
        islider.val() => int numChildren;
        // set number of children to islider.val()

        // case where we need to add children
        while (childCubes.size() < numChildren) {
            GCube childCube;
            childCube.sca(0.25);
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
            childCubes[i].pos(@(Math.cos(angle) * 2, Math.sin(angle) * 2, 0));
        }
        
    }
} spork ~ ISliderListener(islider);

fun void CheckboxListener(UI_Checkbox @ checkbox) {
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

fun void ColorListener(UI_Color3 @ color) {
    GG.scene().backgroundColor(color.val());
    while (true) {
        color => now;
        GG.scene().backgroundColor(color.val());
    }
} spork ~ ColorListener(color);

fun void DropdownListener(UI_Dropdown @ dropdown) {
    while (true) {
        dropdown => now;
        dropdown.val() => int val;
        <<< "dropdown: " + val >>>;
        if (val == 0) {
            cube.mat().polygonMode(Material.POLYGON_FILL);
        } else if (val == 1) {
            cube.mat().polygonMode(Material.POLYGON_LINE);
        } else if (val == 2) {
            cube.mat().polygonMode(Material.POLYGON_POINT);
        }
    }
} spork ~ DropdownListener(dropdown);


// Game loop =========================================================
while (true) { 
    // rotate center cube
    GG.dt() => cube.rotateZ;

    // rotate children
    for (auto c : childCubes) {
    -3 * GG.dt() => c.rotateZ;
    }

    GG.nextFrame() => now; 
}