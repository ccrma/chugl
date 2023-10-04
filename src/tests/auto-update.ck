GG.fullscreen(); // gotta go fastral

BoxGeometry geo;
NormMat mat;

now => time lastTime;
dur deltaTime;
class covfefe extends GGen {

    GMesh meshWindowTime;
    meshWindowTime.set(geo, mat);

    GMesh meshChuckTime;
    meshChuckTime.set(geo, mat);

    meshWindowTime --> this;
    meshChuckTime --> this;

    @(-1.0, 0.0, 0.0) => meshWindowTime.position;
    @(1, 0.0, 0.0) => meshChuckTime.position;

    1.8 => float rotSpeed;

    fun void update(float dt) {
        deltaTime / second => float chuck_dt;
        <<< "window dt: ", dt,  " | chuck dt:",  chuck_dt >>>;
        meshWindowTime.rotX( rotSpeed * dt );
        meshChuckTime.rotX( rotSpeed * chuck_dt );
    }
}

// covfefe c; 
// 0 => int connected;
covfefe c --> GG.scene(); 

while (true) {
    // <<< "next frame" >>>;
    // <<< "main now", now >>>;
    now - lastTime => deltaTime;
    now => lastTime;

    // if (now / second > 2 && !connected) {
    //     c --> GG.scene();
    // }

    GG.nextFrame() => now;
}