InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// GG.fullscreen();
GG.lockCursor();
GG.camera().clip(0.1, 1000);

// ===============================

GG.scene() @=> GScene @ scene;

20 => int AXIS_LENGTH;

GMesh meshes[AXIS_LENGTH * AXIS_LENGTH];
NormalsMaterial normMat;
SphereGeometry  SphereGeometry ;

for (0 => int i; i < AXIS_LENGTH; i++) {
    for (0 => int j; j < AXIS_LENGTH; j++) {
        meshes[i * AXIS_LENGTH + j] @=> GMesh @ mesh;
        mesh.set(SphereGeometry , normMat);
        mesh.pos(2.0 * @(i, 0, -j));
        mesh --> scene;
    }
}

// fog setup =====================

scene.enableFog();
scene.fogDensity(.03);
@(0.3, 0.3, 0.3) => vec3 fogColor;
scene.fogColor(fogColor);
scene.backgroundColor(fogColor);

fun void pingPongFogDensity() {
    scene.fogDensity() => float density;

    while (true) {
        Math.sin(0.5 * (now/second)) * 0.3 + 0.3 => density;
        scene.fogDensity(density);
        GG.nextFrame() => now;
    }
}
spork ~ pingPongFogDensity();

fun void cycleFogType() {
    while (true) {
        <<< "fog type: EXP" >>>;
        scene.fogType(scene.FOG_EXP);
        2::second => now;
        <<< "fog type: EXP2" >>>;
        scene.fogType(scene.FOG_EXP2);
        2::second => now;
    }
}
spork ~ cycleFogType(); 

fun void lerpFogCol() {
    repeat (300) {
        Math.sin(0.3 * (now/second)) * 0.5 + 0.5 => float b;
        Math.sin(0.4 * (now/second)) * 0.5 + 0.5 => float g;
        Math.sin(0.7 * (now/second)) * 0.5 + 0.5 => float r;
        scene.fogColor(@(r, g, b));
        scene.backgroundColor(@(r, g, b));
        GG.nextFrame() => now;
    }
    <<< "lerp Fog color shred dying :(" >>>;
}
spork ~ lerpFogCol();

fun void fogToggle() {
    0 => int enabled;
    while (true) {
        IM.keyDownEvent(IM.KEY_T) => now;
        if (enabled == 1) {
            <<< "fog disabled" >>>;
            scene.disableFog();
            0 => enabled;
        } else {
            <<< "fog enabled" >>>;
            scene.enableFog();
            1 => enabled;
        }
    }
}

spork ~ fogToggle();

// Game loop =====================

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