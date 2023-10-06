0 => int fc;
now => time lastTime;
// NextFrameEvent UpdateEvent;
while (true) {
    now - lastTime => dur dt;
    now => lastTime;
    <<< "fc: ", fc++ , "now: ", now, "dt: ", dt>>>;

    GG.nextFrame() => now;
    // UpdateEvent => now;
}