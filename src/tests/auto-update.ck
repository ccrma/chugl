CGL.fullscreen(); // gotta go fastral

BoxGeo geo;
NormMat mat;

class covfefe extends GGen {

    CglMesh mesh;
    mesh.set(geo, mat);

    mesh --> this;

    fun void update(float dt) {
        // <<< "covfefe ", dt >>>;
        mesh.RotateX( .27 * dt );
    }
}

// covfefe c --> CGL.scene();
covfefe c --> CGL.scene();

while (true) {
    CGL.nextFrame() => now;
}