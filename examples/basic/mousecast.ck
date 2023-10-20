//-----------------------------------------------------------------------------
// name: mousecast.ck
// desc: Using the camera raycast helpers to spawn geometry from mouse clicks 
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// simplified Mouse class from examples/input/Mouse.ck
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

//==============================================================================

// scene setup
5 => GG.camera().posZ;

// setup mouse and mouse listeners
Mouse mouse;
spork ~ mouse.start(0);

fun void clickListener() {
    while (true) {
        mouse.mouseDownEvents[Mouse.LEFT_CLICK] => now;
        // get click location
        GG.mouseX() => float x;
        GG.mouseY() => float y;
        // generate ray going from camera through click location
        GG.camera().screenCoordToWorldRay(GG.mouseX(), GG.mouseY()) => vec3 ray;

        <<< "left click at screen pos", x, y >>>;
        <<< "generates world ray with dir", ray >>>;

        // calculate spawn position by moving along ray
        3 => float distance;
        distance * ray + GG.camera().pos() => vec3 spawnPos;
        // spawn sphere at spawnPos
        GSphere s --> GG.scene();
        s.pos(spawnPos);
        // set random color
        s.mat().color(Color.random());

        // project spawnPos back to screen to verify that it's the same as the click location
        <<< "spawn pos", spawnPos, "maps to screen coord", GG.camera().worldPosToScreenCoord(spawnPos) >>>;
    }
} spork ~ clickListener();


// gameloop
while (true) {
    GG.nextFrame() => now;
}