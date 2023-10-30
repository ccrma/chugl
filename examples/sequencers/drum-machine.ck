// simplified Mouse class from examples/input/Mouse.ck  =======================
class Mouse
{
    int mouseState[3];
    Event mouseDownEvents[3];

    0 => static int LEFT_CLICK;
    1 => static int RIGHT_CLICK;
    2 => static int MIDDLE_CLICK;

    1.0 => float mouseZ;
    vec3 worldPos;

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

    fun void selfUpdate() {
        while (true) {
            GG.mouseX() => float x;
            GG.mouseY() => float y;
            GG.windowWidth() * 1.0 => float screenWidth;
            GG.windowHeight() * 1.0 => float screenHeight;

            // calculate mouse world X and Y coords
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
                GG.camera().posLocalToWorld(@(viewX, viewY, -mouseZ)) => worldPos;
            } else { // perspective
                // generate ray going from camera through click location
                GG.camera().screenCoordToWorldRay(x, y) => vec3 ray;

                // calculate spawn position by moving along ray
                mouseZ * ray + GG.camera().posWorld() => worldPos;
            }
            GG.nextFrame() => now;
        }
    }
}

// TODO add mouseclick event to chugl
Mouse mouse;
spork ~ mouse.start(0);  // start listening for mouse events
spork ~ mouse.selfUpdate();

// Scene setup ================================================================
GG.scene() @=> GScene @ scene;
GG.camera() @=> GCamera @ cam;
cam.orthographic();


fun void mousePos() {
    while (true) {
        mouse.mouseDownEvents[Mouse.LEFT_CLICK] => now;
    }
} spork ~ clickListener();


// Game loop ==================================================================

while (true) { GG.nextFrame() => now; }

