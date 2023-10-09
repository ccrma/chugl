InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

GScene scene;
GMesh meshes[3];
SphereGeometry  SphereGeometry ;
BoxGeometry boxGeo;
MangoUVMaterial mangoMat;

meshes[0].set(SphereGeometry , mangoMat);
meshes[1].set(boxGeo, mangoMat);
meshes[2].set(SphereGeometry , mangoMat);

for (auto mesh : meshes) {
    // mesh --> GG.scene();
    mesh --> scene;
}

meshes[0].position(@(-2, 0, -5));
meshes[1].position(@(0, 0, -5));
meshes[2].position(@(2, 0, -5));

// camera setup ==================

GG.camera() @=> GCamera @ cam;
// cam.orthographic();

int mode;
fun void cycleCameraModes() {
    while (true) {
        2::second => now;
        <<< "persp mode" >>>;
        cam.perspective();
        GCamera.MODE_PERSP => mode;
        2::second => now;
        <<< "ortho mode" >>>;
        cam.orthographic();
        GCamera.MODE_ORTHO => mode;
    }

} 
spork ~ cycleCameraModes();

fun void scrollZoom() {
    MM.GetScrollDelta() / 300.0 => float scroll_delta;
    if (scroll_delta == 0) return;

    <<< "cam mode:  ", cam.mode(), "mode persp", GCamera.MODE_PERSP, 
        "mode ortho", GCamera.MODE_ORTHO, "fov", cam.fov(), "size", cam.viewSize() >>>;

    // TODO why does this integer comparison not work??
    if (mode == GCamera.MODE_PERSP) {
        <<< "setting  fov" >>>;
        cam.fov() - scroll_delta => cam.fov;
    } else if (mode == GCamera.MODE_ORTHO) {
        <<< "setting  size" >>>;
        cam.viewSize() - scroll_delta => cam.viewSize;
    } else {
        <<< "unknown mode" >>>;
    }
}

// Game loop =====================

0 => int frameCounter;
now => time lastTime;
while (true) {
    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    deltaTime/second => float dt;

    scrollZoom();

    GG.nextFrame() => now;
}