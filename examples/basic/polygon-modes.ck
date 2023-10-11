//-----------------------------------------------------------------------------
// name: polygon-modes.ck
// desc: animating geometry with various polygon modes (FILL, LINE, POINT)
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------
GG.fullscreen();
// position camera
GG.camera().position( @(0, 0, 12) );

// load geometries
CircleGeometry circleGeo;
PlaneGeometry planeGeo;
TorusGeometry torusGeo;
LatheGeometry latheGeo;

[
    circleGeo,
    planeGeo,
    torusGeo,
    latheGeo,
] @=> Geometry geos[];

// circle animator
fun void circleSetter() {
    1.0 => float radius;
    32 => int segments;
    0 => float thetaStart;
    Math.PI * 2 => float thetaLength;
    while (true) {
        Math.sin(now/second) * Math.PI + Math.PI => thetaLength;
        circleGeo.set(radius, segments, thetaStart, thetaLength);
        GG.nextFrame() => now;
    }
}
spork ~ circleSetter();

// plane animator
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

// torus animator
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

// lathe animator
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

// Scene setup ================================================================

GScene scene;  // reference to scene

// allocate materials
MangoUVMaterial wireMat, pointMat;  
NormalsMaterial normalMat; 

// set material polygon modes
normalMat.polygonMode(Material.POLYGON_FILL);  // this is the default
wireMat.polygonMode(Material.POLYGON_LINE);
pointMat.polygonMode(Material.POLYGON_POINT);
pointMat.pointSize(25.0);    // note: mac doesn't support glPointSize, only Windows does. this becomes a no-op on mac.

// create a mesh for each possible (geometry, material) pairing
GMesh meshes[geos.size()*4];

for (0 => int i; i < geos.size(); i++) {
    meshes[3*i+0] @=> GMesh @ wireMesh;
    meshes[3*i+1] @=> GMesh @ pointMesh;
    meshes[3*i+2] @=> GMesh @ normMesh;

    geos[i] @=> Geometry @ geo;

    // assign geometry and material to the mesh
    wireMesh.set(geo, wireMat);
    pointMesh.set(geo, pointMat);  
    normMesh.set(geo, normalMat);  

    // add to scene
    wireMesh --> scene;
    pointMesh --> scene;
    normMesh --> scene;

    // set positions
    -(geos.size() / 2)=> float y_off;
    normMesh.position(@(-4, 2*(i + y_off) + 1, 0));
    wireMesh.position(@(0, 2*(i + y_off) + 1, 0));
    pointMesh.position(@(4, 2*(i + y_off) + 1, 0));
}


// gameloop
while (true) { GG.nextFrame() => now; }