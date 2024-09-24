//-----------------------------------------------------------------------------
// name: gtext.ck
// desc: showcases GText features
//       
// requires: ChuGL 0.1.6 + chuck-1.5.2.0 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// Scene Setup =================================================================

GText text --> GG.scene();
text.text("    PLANETFALL: INTERLOGIC Science Fiction
    Copyright (c) 1983 by Infocom, Inc. All rights reserved.
    PLANETFALL and INTERLOGIC are trademarks of Infocom, Inc.
    Release 29 / Serial number 840118

    ------------------------------------------------------------------------

    Another routine day of drugery aboard the Stellar Patrol Ship Feinstein.
    This morning's assignment for a certain lowly Ensign Seventh Class:
    scrubbing the filthy metal deck at the port end of Level Nine. With your
    Patrol-issue self-contained multi-purpose all-weather scrub-brush you 
    shine the floor with a diligence born of the knowledge that at any 
    moment dreaded Ensign First CLass Blather, the bane of your shipboard
    existence, could appear.");
-50 => text.posZ;

GG.font(me.dir() + "../assets/fonts/SourceSansPro-Semibold.otf");

// Post Processing =============================================================

// uncomment to make text glow!
// GG.fx() --> BloomFX bloom --> OutputFX output;
// bloom.threshold(0.5);
// bloom.strength(3.0);
// bloom.levels(10);

// UI Setup ====================================================================
GG.windowTitle("GText Example");
UI_Window window;

window.text("GText Example");

UI_InputText input;
input.text("GText Input");
input.input(text.text());
input.multiline(true);

// uncomment to only broadcast on <enter>
// input.broadcastOnEnter(true);

// uncomment to manually set the input box size
// input.size(@(-1, 20));

UI_Dropdown fontDropdown;
fontDropdown.text("Font");
[
    "SourceSansPro-Semibold",
    "SourceSerifPro-Regular"
] @=> string fonts[];
fontDropdown.options(fonts);

UI_Color3 textColorPicker;
textColorPicker.text("Text Color");
textColorPicker.val(text.color());

UI_SliderFloat textControlPointXSlider;
textControlPointXSlider.text("Control Point X");
textControlPointXSlider.val(text.controlPoints().x);
textControlPointXSlider.range(0.0, 1.0);

UI_SliderFloat textControlPointYSlider;
textControlPointYSlider.text("Control Point Y");
textControlPointYSlider.val(text.controlPoints().y);
textControlPointYSlider.range(0.0, 1.0);

UI_SliderFloat textLineSpacingSlider;
textLineSpacingSlider.text("Line Spacing");
textLineSpacingSlider.val(text.lineSpacing());
textLineSpacingSlider.range(0.01, 3.0);

UI_SliderFloat textScaleSlider;
textScaleSlider.text("Text Scale");
textScaleSlider.val(1.0);
textScaleSlider.range(0.1, 15.0);

UI_Checkbox textRotateCheckbox;
textRotateCheckbox.text("Rotate Text");
textRotateCheckbox.val(false);

window.add(input);
window.add(fontDropdown);
window.add(textColorPicker);
window.add(textControlPointXSlider);
window.add(textControlPointYSlider);
window.add(textLineSpacingSlider);
window.add(textScaleSlider);
window.add(textRotateCheckbox);

// UI Listeners =================================================================
fun void InputListener() {
    while (true) {
        input => now;
        input.input() => text.text;
    }
}
spork ~ InputListener();

fun void FontListener() {
    while (true) {
        fontDropdown => now;
        text.font(me.dir() + "../assets/fonts/" + fonts[fontDropdown.val()] + ".otf");
        <<< "font changed to", text.font() >>>;
    }
}
spork ~ FontListener();

fun void ColorListener() {
    while (true) {
        textColorPicker => now;
        textColorPicker.val() => text.color;
    }
}
spork ~ ColorListener();

fun void TextScaleListener() {
    while (true) {
        textScaleSlider => now;
        textScaleSlider.val() => text.sca;
        <<< "text scale changed to", text.sca() >>>;
    }
}
spork ~ TextScaleListener();

fun void TextControlPointXListener() {
    while (true) {
        textControlPointXSlider => now;
        @(textControlPointXSlider.val(), text.controlPoints().y) => text.controlPoints;
        <<< "text control point x changed to", text.controlPoints().x >>>;
    }
}
spork ~ TextControlPointXListener();

fun void TextControlPointYListener() {
    while (true) {
        textControlPointYSlider => now;
        @(text.controlPoints().x, textControlPointYSlider.val()) => text.controlPoints;
        <<< "text control point y changed to", text.controlPoints().y >>>;
    }
}
spork ~ TextControlPointYListener();

fun void TextLineSpacingListener() {
    while (true) {
        textLineSpacingSlider => now;
        textLineSpacingSlider.val() => text.lineSpacing;
        <<< "text line spacing changed to", text.lineSpacing() >>>;
    }
}
spork ~ TextLineSpacingListener();

fun void fpser() {
    while (true) {
        <<< "fps:", GG.fps() >>>;
        1::second => now;
    }
} spork ~ fpser();

// Game Loop ===================================================================
while (true) {
    // rotate
    if (textRotateCheckbox.val()) { .2 * GG.dt() => text.rotateY; }
    GG.nextFrame() => now; 
}
