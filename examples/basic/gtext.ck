//--------------------------------------------------------------------
// name: gtext.ck
// desc: drawing text with GText!
// requires: ChuGL + chuck-1.5.5.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//   date: Fall 2024
//--------------------------------------------------------------------

// update rendergraph: 
// GG.rootPass() --> (new) scenePass --> null (no GG.outputPass)
// NOTE: this is useful for 2D scenes or scenes without complex lighting
// NOTE: this replaces the default rendergraph
// GG.rootPass() --> GG.scenePass() --> GG.outputPass()
GG.rootPass() --> ScenePass sp( GG.scene() );
// set new orbit camera as main camera
GG.scene().camera(new GOrbitCamera);

// create a GText GGen
GText text --> GG.scene();
// set text size
text.size(.1);
// set default text to render
text.text(
"PLANETFALL: INTERLOGIC Science Fiction
Copyright (c) 1983 by Infocom, Inc. All rights reserved.
PLANETFALL and INTERLOGIC are trademarks of Infocom, Inc.
Release 29 / Serial number 840118

------

Another routine day of drugery aboard the Stellar Patrol Ship Feinstein.
This morning's assignment for a certain lowly Ensign Seventh Class:
scrubbing the filthy metal deck at the port end of Level Nine. With your
Patrol-issue self-contained multi-purpose all-weather scrub-brush you
shine the floor with a diligence born of the knowledge that at any
moment dreaded Ensign First Class Blather, the bane of your shipboard
existence, could appear."
);

UI_String text_input(text.text());

// ChuGL UI ints for selecting font
UI_Int font_index;
[ 
    "chugl:cousine-regular",
    "chugl:karla-regular",
    "chugl:proggy-tiny",
    "chugl:proggy-clean",
] @=> string builtin_fonts[];

// ChuGL UI variable for text color
UI_Float4 text_color;
// default to white
@(1, 1, 1, 1) => text_color.val;

// center point of the text
[0.5, 0.5] @=> float control_points[];

// UI variables
UI_Float line_spacing(1.0);
UI_Float text_size(text.size());
UI_Bool text_rotate;
UI_Float antialias(text.antialias());
UI_Int characters(text.characters());
UI_Float width(text.maxWidth());
UI_Int alignment;
[
    "left",
    "center",
    "right"
] @=> string alignments[];


// background surface to show text alignment+wrapping
FlatMaterial plane_mat;
// add plane as child of the GText
GMesh plane(new PlaneGeometry, plane_mat) --> text;
// set options
plane_mat.color(Color.GRAY);
// make plane transparent
plane_mat.transparent(true); plane_mat.alpha(.5);
// offset plane to slightly behind text
plane.posZ(-.01);
// scale the plane
plane.scaY(100);
// scale to text width
plane.scaX(text.maxWidth());

// main render loop
while (true)
{
    // synchronize
    GG.nextFrame() => now;

    // set UI window size
    UI.setNextWindowSize(@(400, 600), UI_Cond.Once);
    // begin UI
    if (UI.begin("GText Example", null, 0))
    {
        // set the text input width (negative means fill the width)
        UI.setNextItemWidth(-1);
        // the text input widget
        if (UI.inputTextMultiline("##GText Input", text_input, 10000, @(0,0), 0)) {
            text.text(text_input.val());
        }
        
        // invisible widget purely for vertical spacing
        UI.dummy(@(0.0, 20.0));
        
        // list of fonts
        if (UI.listBox("Builtin Fonts", font_index, builtin_fonts, -1)) {
            text.font(builtin_fonts[font_index.val()]);
        }
        // color picker
        if (UI.colorPicker("Text Color", text_color, UI_ColorEditFlags.AlphaPreview | UI_ColorEditFlags.AlphaBar)) {
            text.color(text_color.val());
        }
        // move the control point / center of text
        if (UI.slider("Control Points", control_points, 0.0, 1.0)) {
            text.controlPoints(@(control_points[0], control_points[1]));
        }
        // vertical line spacing
        if (UI.slider("Line Spacing", line_spacing, 0.01, 3.0)) {
            text.spacing(line_spacing.val());
        }
        // text size
        if (UI.slider("Size", text_size, 0.01, 2.0)) {
            text.size(text_size.val());
        }
        // anti-aliasing beyond 1 is purely stylistic!
        if (UI.slider("Antialias", antialias, 0, 50)) {
            text.antialias(antialias.val());
        }
        // number of characters to display
        // (hold down + button for text crawl)
        if (UI.inputInt("Characters\n(hold down +)", characters)) {
            text.characters(characters.val());
        }
        // setting this to > 0 enforces width-based alignment and enables the gray back pane
        if (UI.slider("Wrap Width", width, -2, 50)) {
            plane.scaX(Math.max(0.0, width.val()));
            text.maxWidth(width.val());
        }
        // list of box for selecting text alignment
        if (UI.listBox("Alignment", alignment, alignments, -1)) {
            text.align(alignment.val());
        }
        // toggle rotation of text
        UI.checkbox("Rotate Text", text_rotate);
    }
    // end UI
    UI.end();

    // check and if enabled, rotate the text around Y axis
    if (text_rotate.val()) GG.dt() => text.rotateY;
}
