// GG.fullscreen(); // gotta go fastral
// GG.useChuckTime(true); 

5 => GG.camera().posZ;

BoxGeometry geo;
NormalsMaterial mat;

now => time lastTime;
dur deltaTime;
class covfefe extends GGen {

    GMesh meshWindowTime;
    meshWindowTime.set(geo, mat);

    GMesh meshChuckTime;
    meshChuckTime.set(geo, mat);

    meshWindowTime --> this;
    meshChuckTime --> this;

    @(-1.0, 0.0, 0.0) => meshWindowTime.pos;
    @(1, 0.0, 0.0) => meshChuckTime.pos;

    .8 => float rotSpeed;

    fun void update(float dt) {
        <<< "update now: ", now >>>;
        deltaTime / second => float chuck_dt;
        <<< "window dt: ", dt,  " | chuck dt:",  chuck_dt >>>;
        // <<< "ckdt: ", dt,  " | chuck-side chuck dt:",  chuck_dt >>>;
        meshWindowTime.rotateX( rotSpeed * dt );
        meshChuckTime.rotateX( rotSpeed * chuck_dt );
    }
}

// covfefe c; 
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