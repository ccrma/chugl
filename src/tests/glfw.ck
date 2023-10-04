now => time lastTime;
0 => int fc;


fun void cycleMouseModes() {
    [GG.MOUSE_LOCKED, GG.MOUSE_HIDDEN, GG.MOUSE_NORMAL] @=> int modes[];
    0 => int i;
    while (2::second => now) {
        <<< "activating mouse mode: ", modes[i] >>>;
        modes[i] => GG.mouseMode;
        (i + 1) % 3 => i;
    }
} // spork ~ cycleMouseModes();

fun void windowResizeListener() {
    WindowResizeEvent wr;
    while (true) {
        wr => now;
        // <<< "window resize event: ", GG.windowWidth(), "x", GG.windowHeight() >>>;
        <<< "window resized -- frameWidth: ", GG.frameWidth(), "frameHeight: ", GG.frameHeight() >>>;
    }
} spork ~ windowResizeListener();

fun void cycleWindowModes() {
    while (true) {
        <<< "fullscreenn" >>>;
        GG.fullscreen();
        1.5::second => now;
        <<< "windowed" >>>;
        GG.windowed(400, 600);
        1.5::second => now;
        <<< "setting size" >>>;
        GG.resolution(300, 100);
        1.5::second => now;
    }
} spork ~ cycleWindowModes();

fun void printer() {
    while (1::second => now) {
        now - lastTime => dur dt;
        now => lastTime;
        <<< "fc: ", fc++ , "now: ", now, "dt: ", dt>>>;
        <<< "===================GLFW state=====================" >>>;

        <<< 
            // "window width: ", GG.windowWidth(),
            // "window height: ", GG.windowHeight(),
            "framebuffer width: ", GG.frameWidth(),
            "framebuffer height: ", GG.frameHeight(),
            "glfw time: ", GG.windowUptime(),
            "glfw dt: ", GG.dt(),
            "mouse x: ", GG.mouseX(),
            "mouse y: ", GG.mouseY()
        >>>;
        0 => fc;
    }
}  // spork ~ printer();

while (true) { GG.nextFrame() => now; }