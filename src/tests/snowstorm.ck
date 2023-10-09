InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

GG.scene().backgroundColor(@(0, 0, 0));

GScene scene;
FileTexture tex;
tex.path("./tests/textures/snowflake1.png");

SphereGeometry  geo;
GMesh mesh;
PointMaterial mat; // mat.pointSize(55.0);
mat.pointSprite(tex);
mat.attenuatePoints(false);
mat.color(@(0.0, 1.0, 0.0));

mesh.set(geo, mat);

mesh --> scene;


// Game loop =====================

0 => int frameCounter;
now => time lastTime;
while (true) {
    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    deltaTime/second => float dt;

    GG.nextFrame() => now;
}