InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

CglScene scene;
NormMat fillMat, wireMat, pointMat;
fillMat.polygonMode(CglMat.POLYGON_FILL);
wireMat.polygonMode(CglMat.POLYGON_LINE);
pointMat.polygonMode(CglMat.POLYGON_POINT);

SphereGeo sphereGeo;


CglMesh fillMesh, wireMesh, pointMesh;

fillMesh.set(sphereGeo, fillMat);
wireMesh.set(sphereGeo, wireMat);
pointMesh.set(sphereGeo, pointMat);

// add to scene
scene.AddChild(fillMesh);
scene.AddChild(wireMesh);
scene.AddChild(pointMesh);

// set positions
fillMesh.SetPosition(@(-1, 0, 0));
wireMesh.SetPosition(@(0, 0, 0));
pointMesh.SetPosition(@(1, 0, 0));


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