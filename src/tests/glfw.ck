now => time lastTime;
0 => int fc;


fun void cycleMouseModes() {
    [CGL.MOUSE_LOCKED, CGL.MOUSE_HIDDEN, CGL.MOUSE_NORMAL] @=> int modes[];
    0 => int i;
    while (2::second => now) {
        <<< "activating mouse mode: ", modes[i] >>>;
        modes[i] => CGL.mouseMode;
        (i + 1) % 3 => i;
    }
} // spork ~ cycleMouseModes();

fun void windowResizeListener() {
    WindowResize wr;
    while (true) {
        wr => now;
        // <<< "window resize event: ", CGL.windowWidth(), "x", CGL.windowHeight() >>>;
        <<< "window resized -- framebufferWidth: ", CGL.framebufferWidth(), "framebufferHeight: ", CGL.framebufferHeight() >>>;
    }
} spork ~ windowResizeListener();

fun void cycleWindowModes() {
    while (true) {
        <<< "fullscreenn" >>>;
        CGL.fullscreen();
        2.5::second => now;
        <<< "windowed" >>>;
        CGL.windowed(400, 600);
        2.5::second => now;
    }
} spork ~ cycleWindowModes();

fun void printer() {
    while (1::second => now) {
        now - lastTime => dur dt;
        now => lastTime;
        <<< "fc: ", fc++ , "now: ", now, "dt: ", dt>>>;
        <<< "===================GLFW state=====================" >>>;

        <<< 
            // "window width: ", CGL.windowWidth(),
            // "window height: ", CGL.windowHeight(),
            "framebuffer width: ", CGL.framebufferWidth(),
            "framebuffer height: ", CGL.framebufferHeight(),
            "glfw time: ", CGL.time(),
            "glfw dt: ", CGL.dt(),
            "mouse x: ", CGL.mouseX(),
            "mouse y: ", CGL.mouseY()
        >>>;
        0 => fc;
    }
}  // spork ~ printer();

while (true) { CGL.nextFrame() => now; }