<<< "starting" >>>;

0 => int frameCounter;
now => time lastTime;

CglUpdate UpdateEvent;
CglFrame FrameEvent;

while (true) {
    CGL.Render();
    // UpdateEvent => now;
    // FrameEvent => now;
    (1.0/60.0)::second => now;

    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    <<< "frame: ", frameCounter, " delta: ", deltaTime/second, "FPS: ", second / deltaTime>>>;
}


