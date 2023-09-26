0 => int fc;
now => time lastTime;
// CglUpdate UpdateEvent;
while (true) {
    now - lastTime => dur dt;
    now => lastTime;
    <<< "fc: ", fc++ , "now: ", now, "dt: ", dt>>>;

    CGL.nextFrame() => now;
    // UpdateEvent => now;
}