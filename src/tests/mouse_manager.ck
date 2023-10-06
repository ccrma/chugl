public class MouseManager
{
    /*
    Tracks which mouse buttons are down/up
    Tracks delta of mouse motion and scroll wheel
    */

    // state to track cumulative mouse motion
    // x: mouse delta x
    // y: mouse delta y
    // z: scrollwheel delta
    @(0.0, 0.0, 0.0) => vec3 motionDeltas;

    int mouseState[10];  // TODO: what's the max number of mouse states?
    Event mouseDownEvents[10];

    0 => static int LEFT_CLICK;
    1 => static int RIGHT_CLICK;
    2 => static int MIDDLE_CLICK;


    // returns motionDeltas since last time this function was called
    fun vec3 GetDeltas() {
        motionDeltas => vec3 tmp;
        @(0.0, 0.0, 0.0) => motionDeltas;  // rezero sum
        return tmp;
    }

    fun float GetScrollDelta() {
        return motionDeltas.z;
    }

    fun void start(int device) {
        // HID input and a HID message
        Hid hi;
        HidMsg msg;

        // open mouse 0, exit on fail
        if( !hi.openMouse( device ) ) {
            cherr <= "failed to open device " + device <= IO.newline();
            me.exit();
        }

        <<< "mouse '" + hi.name() + "' ready", "" >>>;

        // infinite event loop
        while( true ) {
            hi => now;
            while( hi.recv( msg ) ) {
                // mouse motion
                if( msg.isMouseMotion() ) {
                    if( msg.deltaX ) {
                        motionDeltas.x + msg.deltaX => motionDeltas.x;
                    }
                    if( msg.deltaY ) {
                        motionDeltas.y + msg.deltaY => motionDeltas.y;
                    }
                }
                
                // mouse button down
                else if( msg.isButtonDown() ) {
                    1 => mouseState[msg.which];
                    mouseDownEvents[msg.which].broadcast();
                }
                
                // mouse button up
                else if( msg.isButtonUp() ) {
                    0 => mouseState[msg.which];
                }
                
                // mouse wheel motion
                else if( msg.isWheelMotion() ) {
                    if( msg.deltaY )
                        motionDeltas.z + msg.deltaY => motionDeltas.z;
                }
            }
        }
    }
}

// MouseManager mm;
// spork ~ mm.start(0);

// while (true) {
//     <<< mm.GetDeltas() >>>;
//     1::second => now;
// }
