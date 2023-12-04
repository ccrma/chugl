[
    // "gc",  
    // TODO re-enable after resolving Machine.remove() not clearing children
    // which are sporked via Machine.add()
    // TODO: gc test causes segfault
    "gtext",
    // "skybox",
    // "lights",
    // "alpha",
    // "gameloop",
    // "shred-remove",
    // "auto-update",
    // "sne",
    // "cameras",
    // "clone-and-dup", 
    // "lines",
    // "colors",
    // "next-frame-no-chuck",
    // "textures",
    // "glfw",
    // "transform",
    // "world-pos-world-sca",
    // "visualizer",
] @=> string unit_test_paths[];

[   
    // fx
    "./basic/circles",
    "./fx/bloom",
    "./basic/polygon-modes",
    "./fx/custom-fx",
    // basic
    "./basic/skybox",
    "./basic/gtext",
    "./basic/gameloop",
    "./basic/mousecast",
    "./basic/custom-geo",
    "./basic/ggen-primitives",
    "./basic/orbits",
    "./basic/transparency",
    "./basic/fog",
    "./basic/light",
    "./basic/points",

    "./audioshader/audio-texture",

    "./geometry/cylinder",
    "./geometry/triangle",

    "./models/obj-loader",
    // "./models/backpa."

    "./sndpeek/sndpeek",

    "./ui/basic-ui",

    "./textures/snowstorm",
    "./textures/textures-1",
] @=> string example_paths[];

// Input setup ==================================
InputManager IM;
spork ~ IM.start(0);

// TODO: add a way to clone any chugl object
// useful for re-initializing to default values
GG.scene().light() @=> GLight @ light;
light.ambient() => vec3 defaultAmbient;
light.diffuse() => vec3 defaultDiffuse;
light.specular() => vec3 defaultSpecular;
light.pos() => vec3 defaultPosition;
light.rot() => vec3 defaultRotation;
light.intensity() => float defaultIntensity;


true => int testing;
fun void reset() {
    // reset scene
    GG.scene().posWorld(@(0,0,0));
    GG.scene().rot(@(0,0,0));
    GG.scene().sca(1);

    // reset camera
    GG.camera().perspective();
    GG.camera().posWorld(@(0,0,5));
    GG.camera().rot(@(0,0,0));
    GG.camera().sca(1);
    GG.camera() --> GG.scene();

    // reset window
    GG.windowed(1200, 900);
    GG.windowTitle("Chugl Unit Tests");

    // reset mouse
    GG.mouseMode(GG.MOUSE_NORMAL);

    // reset fog
    GG.scene().enableFog();
    GG.scene().fogDensity(0);
    GG.scene().fogColor(Color.BLACK);

    // reset background color
    GG.scene().backgroundColor(Color.BLACK);

    // reset skybox
    GG.scene().skyboxEnabled(false);

    // reset FX
    GG.fx().removeNext();

    // reset default light
    GG.scene().light() @=> GLight light;
    light.lookAt(@(0,0,1));
    light.ambient(defaultAmbient);
    light.diffuse(defaultDiffuse);
    light.specular(defaultSpecular);
    light.pos(defaultPosition);
    light.rot(defaultRotation);
    light.intensity(defaultIntensity);
}

fun void runTest(string path) {
    reset();

    Machine.add(path) => int unit_test_shred_id;
    GG.windowTitle(path);

    chout <= "=========================================================" <= IO.newline();
    chout <= "Running test: " <= path <= IO.newline();
    chout <= "=========================================================" <= IO.newline();
    chout.flush();

    // wait for spacebar to be pressed
    IM.keyDownEvent(IM.KEY_SPACE) => now;

    // remove unit test shred
    Machine.remove(unit_test_shred_id);
}

fun void run() {
    string source_path;
    int unit_test_shred_id;

    // run unit tests
    for (auto path : unit_test_paths) {
        me.dir() + path + ".ck" => source_path; 
        runTest(source_path);
    }

    // run examples
    for (auto path : example_paths) {
        me.dir() + "../../examples/" + path + ".ck" => source_path; 
        runTest(source_path);
    }

    false => testing;
}

// Game loop ====================================

spork ~ run();

while (testing) {
    GG.nextFrame() => now;
}




