InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

CglScene scene;
FileTexture tex;
tex.path("./tests/textures/snowflake7_alpha.png");

SphereGeo geo;
CglMesh mesh;
PointsMat mat;  mat.pointSize(55.0);
mat.sprite(tex);
// mat.color(@(0.0, 1.0, 0.0));

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

    CGL.nextFrame() => now;
}