InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================
SphereGeometry sphereGeo;
BoxGeometry boxGeo;
CircleGeometry circleGeo;
PlaneGeometry planeGeo;
TorusGeometry torusGeo;
LatheGeometry latheGeo;

[
    sphereGeo,
    boxGeo,
    circleGeo,
    planeGeo,
    torusGeo,
    latheGeo,
] @=> Geometry geos[];

fun void circleSetter() {
    while (true) {
        Math.sin(now/second) * Math.PI + Math.PI => float thetaLength;
        circleGeo.set(1.0, 32, 0, thetaLength);
        GG.nextFrame() => now;
    }
}
spork ~ circleSetter();

fun void planeSetter() {
    1.0 => float width;
    1.0 => float height;
    5 => int widthSegments;
    5 => int heightSegments;

    1 => float size;
    while (true) {
        now / second => float ftime;
        Math.sin(.5 * ftime) * size + size => width;
        Math.sin(.7 * ftime) * size + size => height;
        planeGeo.set(width, height, widthSegments, heightSegments);
        GG.nextFrame() => now;
    }
}
spork ~ planeSetter();

fun void torusSetter() {
    1.0 => float radius;
	0.4 => float tubeRadius;
	12 => int radialSegments;
	48 => int tubularSegments;
    Math.PI * 2.0 => float arcLen;

    .5 => float size;

    while (true) {
        (now / second) => float ftime;
        Math.sin(.2 * ftime) * size + size => radius;
        Math.sin(.3 * ftime) * 0.2 + 0.22 => tubeRadius;
        (Math.sin(.1 * ftime) * 12 + 14) $ int => radialSegments;
        (Math.sin(.12 * ftime) * 40 + 45) $ int => tubularSegments;
        torusGeo.set(radius, tubeRadius, radialSegments, tubularSegments, arcLen);
        GG.nextFrame() => now;
    }
}
spork ~ torusSetter();

fun void latheSetter() {
    20 => int pathCount;
    1.0 / pathCount => float invPathCount;
    float points[0];
    for ( int i; i < pathCount; i ++ ) {
        points <<  .5 * Math.sin( Math.PI * invPathCount * i );  // x
        points << ( .1 * i - .5 );        // y
    }
    12 => int segments;
    0 => float phiStart;
    Math.PI * 2 => float phiLength;
    latheGeo.set(points, segments, phiStart, phiLength);
    while (true) {
        now / second => float ftime;
        (Math.sin(.2 * ftime) * 6 + 12) $ int => segments;
        Math.sin(.3 * ftime) * Math.PI + Math.PI => phiLength;

        latheGeo.set(segments, phiStart, phiLength);

        GG.nextFrame() => now;
    }
}
spork ~ latheSetter();

GScene scene;
MangoUVMaterial fillMat, wireMat, pointMat;
NormalsMaterial normMat;
fillMat.polygonMode(Material.POLYGON_FILL);
wireMat.polygonMode(Material.POLYGON_LINE);
pointMat.polygonMode(Material.POLYGON_POINT);
pointMat.pointSize(55.0);    // note: mac m1 doesn't support glPointSize. this becomes a no-op

GMesh meshes[geos.size()*4];

for (0 => int i; i < geos.size(); i++) {
    meshes[4*i] @=> GMesh @ fillMesh;
    meshes[4*i+1] @=> GMesh @ wireMesh;
    meshes[4*i+2] @=> GMesh @ pointMesh;
    meshes[4*i+3] @=> GMesh @ normMesh;

    geos[i] @=> Geometry @ geo;

    fillMesh.set(geo, fillMat);
    // wireMesh.set(geo, wireMat);
    wireMesh.set(geo, wireMat);
    pointMesh.set(geo, pointMat);  // TODO this is still crunk on mac
    normMesh.set(geo, normMat);  // TODO this is still crunk on mac

    // add to scene
    fillMesh --> scene;
    wireMesh --> scene;
    pointMesh --> scene;
    normMesh --> scene;

    // set positions
    normMesh.position(@(-5, 2*i, -5));
    fillMesh.position(@(-1, 2*i, -5));

    wireMesh.position(@(0, 2*i, -3));
    pointMesh.position(@(1, 2*i, -1));
}



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