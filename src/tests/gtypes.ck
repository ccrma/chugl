InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

GCube cube;

cube --> GG.scene();
-5 => cube.posZ;

while (true) {
    // GG.dt() => cube.rotX;
    Math.sin(now/second) => cube.posX;
    GG.dt() => GG.scene().light().rotX;
    GG.nextFrame() => now;
}