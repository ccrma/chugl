InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
GCamera cam;
flycam.init(IM, MM, cam);
spork ~ flycam.selfUpdate();

// ===============================

// TODO register itself in scene, loop with GG.scene().ggens;
GCube cube --> GG.scene();
GSphere sphere --> GG.scene();
GCircle circle --> GG.scene();
GPlane plane --> GG.scene();
GLines lines --> GG.scene();
GPoints points --> GG.scene();  points.mat().pointSize(55.0);
GTorus torus --> GG.scene();

[
    cube,
    sphere,
    circle,
    plane,
    lines,
    points,
    torus
] @=> GMesh ggens[];

for (int i; i < ggens.size(); i++) {
    -5 => ggens[i].posZ;
    i*2 => ggens[i].posX;

    // todo add random color helper fn
    Math.random2f(0.0, 1.0) => float r;
    Math.random2f(0.0, 1.0) => float g;
    Math.random2f(0.0, 1.0) => float b;
    @(r, g, b) => ggens[i].mat().color;
}


<<< "num lights: ", GG.scene().numLights() >>>;

while (true) {
    // GG.dt() => cube.rotX;
    Math.sin(now/second) => cube.posX;
    // GG.dt() => GG.scene().light().rotX;
    GG.nextFrame() => now;
}