<<< "starting" >>>;

0 => int frameCounter;
now => time lastTime;

CglUpdate UpdateEvent;
CglFrame FrameEvent;

while (true) {
    CGL.Render();
    UpdateEvent => now;

    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    <<< "frame: ", frameCounter, " delta: ", deltaTime/second, "FPS: ", second / deltaTime>>>;
}


