InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

GScene scene;
NormMat fillMat, wireMat, pointMat;
fillMat.polygonMode(CglMat.POLYGON_FILL);
wireMat.polygonMode(CglMat.POLYGON_LINE);
pointMat.polygonMode(CglMat.POLYGON_POINT);
pointMat.pointSize(15.0);    // note: mac m1 doesn't support glPointSize. this becomes a no-op

SphereGeometry  SphereGeometry ;


GMesh fillMesh, wireMesh, pointMesh;

fillMesh.set(SphereGeometry , fillMat);
wireMesh.set(SphereGeometry , wireMat);
pointMesh.set(SphereGeometry , pointMat);

// add to scene
fillMesh --> scene;
wireMesh --> scene;
pointMesh --> scene;

// set positions
fillMesh.position(@(-1, 0, 0));
wireMesh.position(@(0, 0, 0));
pointMesh.position(@(1, 0, 0));


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