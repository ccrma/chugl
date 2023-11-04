// Scene setup ===========================================================
GCube cube --> GG.scene();
GCube child --> cube;
1 => child.posX;

5 => GG.camera().posZ;

// UI setup ==========================================================
UI_Window window;
window.text("Transform Test Suite");

UI_SliderFloat rotation_x_slider;
rotation_x_slider.text("Rotation X");
rotation_x_slider.range(-Math.two_pi, Math.two_pi);

UI_SliderFloat rotation_y_slider;
rotation_y_slider.text("Rotation Y");
rotation_y_slider.range(-Math.two_pi, Math.two_pi);

UI_SliderFloat rotation_z_slider;
rotation_z_slider.text("Rotation Z");
rotation_z_slider.range(-Math.two_pi, Math.two_pi);

UI_Button translate_x_right_button;
translate_x_right_button.text("Translate X Right");

UI_Button translate_x_left_button;
translate_x_left_button.text("Translate X Left");

UI_Button translate_y_up_button;
translate_y_up_button.text("Translate Y Up");

UI_Button translate_y_down_button;
translate_y_down_button.text("Translate Y Down");

UI_Button translate_z_in_button;
translate_z_in_button.text("Translate Z In");

UI_Button translate_z_out_button;
translate_z_out_button.text("Translate Z Out");

UI_Button scale_x_up_button;
scale_x_up_button.text("Scale X Up");

UI_Button scale_x_down_button;
scale_x_down_button.text("Scale X Down");

UI_Button scale_y_up_button;
scale_y_up_button.text("Scale Y Up");

UI_Button scale_y_down_button;
scale_y_down_button.text("Scale Y Down");

UI_Button scale_z_up_button;
scale_z_up_button.text("Scale Z Up");

UI_Button scale_z_down_button;
scale_z_down_button.text("Scale Z Down");

UI_Checkbox lookat_checkbox;
lookat_checkbox.text("Look At");

window.add(rotation_x_slider);
window.add(rotation_y_slider);
window.add(rotation_z_slider);
window.add(translate_x_right_button);
window.add(translate_x_left_button);
window.add(translate_y_up_button);
window.add(translate_y_down_button);
window.add(translate_z_in_button);
window.add(translate_z_out_button);
window.add(scale_x_up_button);
window.add(scale_x_down_button);
window.add(scale_y_up_button);
window.add(scale_y_down_button);
window.add(scale_z_up_button);
window.add(scale_z_down_button);
window.add(lookat_checkbox);

// UI event handlers ====================================================

fun void RotationXListener(UI_SliderFloat @ slider) {
    while (true) {
        slider => now;
        cube.rotX(slider.val());
        <<< "cube rotX: ", cube.rotX() >>>;
    }
} spork ~ RotationXListener(rotation_x_slider);

fun void RotationYListener(UI_SliderFloat @ slider) {
    while (true) {
        slider => now;
        cube.rotY(slider.val());
        <<< "cube rotY: ", cube.rotY() >>>;
    }
} spork ~ RotationYListener(rotation_y_slider);

fun void RotationZListener(UI_SliderFloat @ slider) {
    while (true) {
        slider => now;
        cube.rotZ(slider.val());
        <<< "cube rotZ: ", cube.rotZ() >>>;
    }
} spork ~ RotationZListener(rotation_z_slider);

fun void TranslateLeftListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.translateX(-1);
    }
} spork ~ TranslateLeftListener(translate_x_left_button);

fun void TranslateRightListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.translateX(1);
    }
} spork ~ TranslateRightListener(translate_x_right_button);

fun void TranslateUpListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.translateY(1);
    }
} spork ~ TranslateUpListener(translate_y_up_button);

fun void TranslateDownListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.translateY(-1);
    }
} spork ~ TranslateDownListener(translate_y_down_button);

fun void TranslateInListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.translateZ(1);
    }
} spork ~ TranslateInListener(translate_z_in_button);

fun void TranslateOutListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.translateZ(-1);
    }
} spork ~ TranslateOutListener(translate_z_out_button);

fun void ScaleXUpListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.scaX(cube.scaX() * 1.1);
    }
} spork ~ ScaleXUpListener(scale_x_up_button);

fun void ScaleXDownListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.scaX(cube.scaX() * (1.0/1.1));
    }
} spork ~ ScaleXDownListener(scale_x_down_button);

fun void ScaleYUpListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.scaY(cube.scaY() * 1.1);
    }
} spork ~ ScaleYUpListener(scale_y_up_button);

fun void ScaleYDownListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.scaY(cube.scaY() * (1.0/1.1));
    }
} spork ~ ScaleYDownListener(scale_y_down_button);

fun void ScaleZUpListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.scaZ(cube.scaZ() * 1.1);
    }
} spork ~ ScaleZUpListener(scale_z_up_button);

fun void ScaleZDownListener(UI_Button @ button) {
    while (true) {
        button => now;
        cube.scaZ(cube.scaZ() * (1.0/1.1));
    }
} spork ~ ScaleZDownListener(scale_z_down_button);

false => int shouldLookAt;
fun void LookAtListener(UI_Checkbox @ checkbox) {
    while (true) {
        checkbox => now;
        checkbox.val() => shouldLookAt;
    }
} spork ~ LookAtListener(lookat_checkbox);





// test parent / child accessors
fun void GetChildGetParent() {
    cube.child() @=> GGen @ c;
    child.parent() @=> GGen @ p;
    <<< "child world pos", c.posWorld(), " | refcount", c >>>;
    <<< "parent world pos", p.posWorld(), " | refcount", p >>>;
    <<< "num children", p.numChildren() >>>;
} 

// separate sporking function to test refcounting behavior
// and make sure destructor is not called
fun void ChildParentSporker() {
    while (1::second => now) {
        spork ~ GetChildGetParent();
    }
}
spork ~ ChildParentSporker();




// Game loop =============================================================
GSphere sphere --> GG.scene();
.1 => sphere.sca;
while (true) {
    sphere.pos(
        @(
            2 * Math.cos((now/second) * 0.3),
            2 * Math.sin((now/second) * 0.3),
            0
        )
    );
    if (shouldLookAt) {
        // look at point that rotates around the cube
        cube.lookAt(sphere.pos());
    }
    GG.nextFrame() => now;
}