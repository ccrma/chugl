InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

CglScene scene;
SphereGeo sphereGeo;
PointsMat pointsMat;  pointsMat.pointSize(15.0);
CglMesh pointMesh;

pointMesh.set(sphereGeo, pointsMat);
scene.AddChild(pointMesh);


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