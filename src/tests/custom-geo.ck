InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

CustomGeometry CustomGeometry;
CustomGeometrypositions(
    // vertex positions for a plane
    [
        // left triangle
        -1.0,  1.0,  0.0,
        -1.0, -1.0,  0.0,
         1.0, -1.0,  0.0,

        // right triangle
        -1.0,  1.0,  0.0,
         1.0, -1.0,  0.0,
         1.0,  1.0,  0.0
    ]
);

CustomGeometry.setNormals(
    // vertex normals for a plane
    [
        // left triangle
        0.0,  0.0,  1.0,
        0.0,  0.0,  1.0,
        0.0,  0.0,  1.0,

        // right triangle
        0.0,  0.0,  1.0,
        0.0,  0.0,  1.0,
        0.0,  0.0,  1.0
    ]
);

CustomGeometry.setUVs(
    // vertex uvs for a plane
    [
        // left triangle
        0.0,  1.0,
        0.0,  0.0,
        1.0,  0.0,

        // right triangle
        0.0,  1.0,
        1.0,  0.0,
        1.0,  1.0
    ]
);

CustomGeometry.setIndices(
    [
        0, 1, 2,
        3, 4, 5
    ]
);


GScene scene;
NormMat normMat;
MangoMat mangoMat;
GMesh mesh;

mesh.set(CustomGeometry, mangoMat);
mesh.position(@(0.0, 0.0, -5.0));

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

    mangoMat.uniformFloat("u_Time", now/second);
    5::ms => now;  // need to put this so it doesn't hang 

    GG.nextFrame() => now;
}