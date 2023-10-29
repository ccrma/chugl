//-----------------------------------------------------------------------------
// name: mousecast.ck
// desc: Using the camera raycast helpers to spawn geometry from mouse clicks 
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// simplified Mouse class from examples/input/Mouse.ck  =======================
class Mouse
{
    int mouseState[3];
    Event mouseDownEvents[3];

    0 => static int LEFT_CLICK;
    1 => static int RIGHT_CLICK;
    2 => static int MIDDLE_CLICK;

    // start this device (should be sporked)
    fun void start(int device)
    {
        // HID input and a HID message
        Hid hi;
        HidMsg msg;

        // open mouse 0, exit on fail
        if( !hi.openMouse( device ) )
        {
            cherr <= "failed to open device " + device <= IO.newline();
            me.exit();
        }
        <<< "mouse '" + hi.name() + "' ready", "" >>>;

        // infinite event loop
        while( true )
        {
            hi => now;
            while( hi.recv( msg ) )
            {
                // mouse button down
                if( msg.isButtonDown() )
                {
                    1 => mouseState[msg.which];
                    mouseDownEvents[msg.which].broadcast();
                }
                // mouse button up
                else if( msg.isButtonUp() ) {
                    0 => mouseState[msg.which];
                }
            }
        }
    }
}

// Example Parameters ==========================================================
8 => float spawnDistance;
false => int rotateCam;
["Perspective", "Orthographic"] @=> string cameraModes[];

// UI setup ====================================================================
UI_Window window;
window.text("Mousecast Example");

UI_SliderFloat distanceSlider;
distanceSlider.text("Spawn Distance");
distanceSlider.range(1, 20);
distanceSlider.val(spawnDistance);

UI_Checkbox rotateCamCheckbox;
rotateCamCheckbox.text("Rotate Camera");
rotateCamCheckbox.val(rotateCam);

UI_Dropdown dropdown;
dropdown.text("Select Camera Mode");
dropdown.options(cameraModes);
dropdown.val(0);

window.add(distanceSlider);
window.add(rotateCamCheckbox);
window.add(dropdown);

fun void distanceSliderListener() {
    while (true) {
        distanceSlider => now;
        distanceSlider.val() => spawnDistance;
    }
} spork ~ distanceSliderListener();

fun void rotateCamCheckboxListener() {
    while (true) {
        rotateCamCheckbox => now;
        rotateCamCheckbox.val() => rotateCam;
    }
} spork ~ rotateCamCheckboxListener();

fun void dropdownListener() {
    while (true) {
        dropdown => now;
        dropdown.val() => int mode;
        if (cameraModes[mode] == "Perspective") {
            GG.camera().perspective();
        } else if (cameraModes[mode] == "Orthographic") {
            GG.camera().orthographic();
        }
    }
} spork ~ dropdownListener();

// setup mouse and mouse listeners =============================================
Mouse mouse;
spork ~ mouse.start(0);

fun void clickListener() {
    while (true) {
        mouse.mouseDownEvents[Mouse.LEFT_CLICK] => now;
        // get click location
        GG.mouseX() => float x;
        GG.mouseY() => float y;
        GG.windowWidth() * 1.0 => float screenWidth;
        GG.windowHeight() * 1.0 => float screenHeight;
        <<< "left click at screen pos", x, y >>>;

        vec3 spawnPos;

        // calculate spawn position differently depending on camera mode
        if (GG.camera().mode() == GCamera.ORTHO) {
            // calculate screen aspect
            screenWidth / screenHeight => float aspect;

            // calculate camera frustrum size in world space
            GG.camera().viewSize() => float frustrumHeight;  // height of frustrum in world space
            frustrumHeight * aspect => float frustrumWidth;  // width of frustrum in world space

            // convert from normalized mouse coords to view space coords
            // (we negate viewY so that 0,0 is bottom left instead of top left)
            frustrumWidth * (x / screenWidth - 0.5) => float viewX;
            -frustrumHeight * (y / screenHeight - 0.5) => float viewY;

            // convert from view space coords to world space coords
            GG.camera().posLocalToWorld(@(viewX, viewY, -spawnDistance)) => spawnPos;
        } else {
            // generate ray going from camera through click location
            GG.camera().screenCoordToWorldRay(x, y) => vec3 ray;

            <<< "generates world ray with dir", ray >>>;

            // calculate spawn position by moving along ray
            spawnDistance * ray + GG.camera().pos() => spawnPos;
        }

        // spawn sphere at spawnPos
        GSphere s --> GG.scene();
        s.pos(spawnPos);
        // set random color
        s.mat().color(Color.random());

        // project spawnPos back to screen to verify that it's the same as the click location
        <<< "spawn pos", spawnPos, "maps to screen coord", GG.camera().worldPosToScreenCoord(spawnPos) >>>;
    }
} spork ~ clickListener();

// gameloop ====================================================================
while (true) {
    if (rotateCam) { -GG.dt() * .2 => GG.camera().rotateY; }
    GG.nextFrame() => now;
}