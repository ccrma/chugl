InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ===============================

CglScene scene;
CglMesh meshes[3];
SphereGeo sphereGeo;
BoxGeo boxGeo;
MangoMat mangoMat;

meshes[0].set(sphereGeo, mangoMat);
meshes[1].set(boxGeo, mangoMat);
meshes[2].set(sphereGeo, mangoMat);

for (auto mesh : meshes) {
    mesh --> scene;
}

meshes[0].SetPosition(@(-2, 0, -5));
meshes[1].SetPosition(@(0, 0, -5));
meshes[2].SetPosition(@(2, 0, -5));

// camera setup ==================

CGL.mainCam() @=> CglCamera @ cam;
// cam.orthographic();

int mode;
fun void cycleCameraModes() {
    while (true) {
        2::second => now;
        <<< "persp mode" >>>;
        cam.perspective();
        CglCamera.MODE_PERSP => mode;
        2::second => now;
        <<< "ortho mode" >>>;
        cam.orthographic();
        CglCamera.MODE_ORTHO => mode;
    }

} 
spork ~ cycleCameraModes();

fun void scrollZoom() {
    MM.GetScrollDelta() / 300.0 => float scroll_delta;
    if (scroll_delta == 0) return;

    <<< "cam mode:  ", cam.mode(), "mode persp", CglCamera.MODE_PERSP, 
        "mode ortho", CglCamera.MODE_ORTHO, "fov", cam.fov(), "size", cam.viewSize() >>>;

    // TODO why does this integer comparison not work??
    if (mode == CglCamera.MODE_PERSP) {
        <<< "setting  fov" >>>;
        cam.fov() - scroll_delta => cam.fov;
    } else if (mode == CglCamera.MODE_ORTHO) {
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

    CGL.nextFrame() => now;
}