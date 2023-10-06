// GGen ggen;
SphereGeometry sphereGeo;
MangoMat mat;

GMesh mesh;
GMesh mesh2;
GMesh mesh3;

mesh.geo(sphereGeo);
mesh.mat(mat);
mesh2.set(sphereGeo, mat);
mesh3.set(sphereGeo, mat);

// duplicate the mat so only one changes color!
mesh2.dupMat();
mesh2.dupGeo();
mesh2.mat().polygonMode(Material.POLYGON_LINE);

mesh2.mat().clone() @=> Material @ clonedMat;

mesh3.dup();
(mesh3.geo() $ SphereGeometry).set(0.2, 32, 16, 0.0, Math.PI * 2.0, 0.0, Math.PI);
mesh3.mat(clonedMat);
mesh3.geo().clone() @=> Geometry @ clonedGeo;
mesh.geo(clonedGeo);

// <<< "mesh2 duplicated mat", mesh2.mat() >>>;
// <<< "mesh original mat", mesh.mat() >>>;


// test mesh getters / setters
fun void matSetter() {
    while (true) {
        mesh.mat().uniformFloat("u_Time", 10* (now/second));
        GG.nextFrame() => now;
    }
}
spork ~ matSetter();

fun void geoSetter() {
    0.5 => float radius;
    32 => int widthSegments;
    16 => int heightSegments;
    0.0 => float phiStart;
    Math.PI * 2.0 => float phiLength;
    0.0 => float thetaStart;
    Math.PI => float thetaLength;

    .7 => float size;
    while (true) {
        // TODO how to cast into specific geometry type in chuck?
        now / second => float ftime;
        Math.sin(.2 * ftime) * size + size => radius;
        mesh.geo() $ SphereGeometry @=> SphereGeometry @ geo;
        geo.set(radius, widthSegments, heightSegments, phiStart, phiLength, thetaStart, thetaLength);
        GG.nextFrame() => now;
    }
}
spork ~ geoSetter();

mesh --> GG.scene();
mesh2 --> GG.scene();
mesh3 --> GG.scene();

@(2.0, 0.0, -4.0) => mesh.position;
@(-2.0, 0.0, -4.0) => mesh2.position;
@(0.0, 0.0, -4.0) => mesh3.position;

while (true) {

    GG.nextFrame() => now;
}