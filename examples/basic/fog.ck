//-----------------------------------------------------------------------------
// name: fog.ck
// desc: basic fog usage
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// scene setup ===================
GG.scene() @=> GScene @ scene;   // scene reference
GG.camera().position(@(0, 1, 0));

20 => int AXIS_LENGTH;

GG.camera().lookAt(AXIS_LENGTH * @(1, 0, -1));  // angle camera
GMesh meshes[AXIS_LENGTH * AXIS_LENGTH];
NormalsMaterial normMat;
SphereGeometry SphereGeometry;

// set up grid of spheres
for (0 => int i; i < AXIS_LENGTH; i++) {
    for (0 => int j; j < AXIS_LENGTH; j++) {
        meshes[i * AXIS_LENGTH + j] @=> GMesh @ mesh;
        mesh.set(SphereGeometry , normMat);
        mesh.position(2.0 * @(i, 0, -j));
        mesh --> scene;
    }
}

// fog setup =====================
scene.enableFog();
scene.fogDensity(.03);  // density is typically between [0, 1]
@(0.3, 0.3, 0.3) => vec3 fogColor;

// important! match fog color and background color for more realistic effect
scene.fogColor(fogColor);
scene.backgroundColor(fogColor);

// oscillate fog density between [0 and 0.6]
fun void pingPongFogDensity() {
    while (true) {
        scene.fogDensity(Math.sin(0.5 * (now/second)) * 0.3 + 0.3);
        GG.nextFrame() => now;
    }
}
spork ~ pingPongFogDensity();


// spork this to cycle through fog types
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
// spork ~ cycleFogType(); 

// spork this to smoothly cycle through fog colors
fun void lerpFogCol() {
    while (true) {
        Math.sin(0.7 * (now/second)) * 0.5 + 0.5 => float r;
        Math.sin(0.4 * (now/second)) * 0.5 + 0.5 => float g;
        Math.sin(0.3 * (now/second)) * 0.5 + 0.5 => float b;
        scene.fogColor(@(r, g, b));
        scene.backgroundColor(@(r, g, b));
        GG.nextFrame() => now;
    }
}
spork ~ lerpFogCol();

// Game loop =====================
while (true) { GG.nextFrame() => now; }