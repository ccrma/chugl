// find skymaps here: https://www.humus.name/index.php?page=Textures
// https://learnopengl.com/img/textures/skybox.zip

// controls ==============================================================
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// UI ===================================================================

UI_Window window;
window.text("skybox test");

UI_Dropdown dropdown;
dropdown.text("skybox");
["water", "yokohama", "none"] @=> string skyboxOptions[];
dropdown.options(skyboxOptions);

UI_Checkbox skyboxCheckbox;
skyboxCheckbox.text("skybox enabled");
skyboxCheckbox.val(true);

window.add(dropdown);
window.add(skyboxCheckbox);

CubeTexture waterEnvMap, yokohamaEnvMap, noneEnvMap;
waterEnvMap.paths(
    me.dir() + "./textures/skybox/water/right.jpg",
    me.dir() + "./textures/skybox/water/left.jpg",
    me.dir() + "./textures/skybox/water/top.jpg",
    me.dir() + "./textures/skybox/water/bottom.jpg",
    me.dir() + "./textures/skybox/water/front.jpg",
    me.dir() + "./textures/skybox/water/back.jpg"
);

yokohamaEnvMap.paths(
    me.dir() + "./textures/skybox/Yokohama3/right.jpg",
    me.dir() + "./textures/skybox/Yokohama3/left.jpg",
    me.dir() + "./textures/skybox/Yokohama3/top.jpg",
    me.dir() + "./textures/skybox/Yokohama3/bottom.jpg",
    me.dir() + "./textures/skybox/Yokohama3/front.jpg",
    me.dir() + "./textures/skybox/Yokohama3/back.jpg"
);

GG.scene() @=> GScene @ scene;

fun void DropdownListener() {
    while (true) {
        dropdown => now;
        dropdown.val() => int val;
        if (val == 0) {
            scene.skybox(waterEnvMap);
        } else if (val == 1) {
            scene.skybox(yokohamaEnvMap);
        } else if (val == skyboxOptions.size() - 1) {
            scene.skybox(noneEnvMap);
        }
        scene.skyboxEnabled(skyboxCheckbox.val());
    }
} spork ~ DropdownListener();

fun void SkyboxToggleListener() {
    while (true) {
        skyboxCheckbox => now;
        skyboxCheckbox.val() => int val;
        if (val) {
            scene.skyboxEnabled(true);
        } else {
            scene.skyboxEnabled(false);
        }
    }
} spork ~ SkyboxToggleListener();


// scene setup ===========================================================


GSphere sphere --> GG.scene();
GCube cube --> GG.scene();
cube.mat().color(Color.RED);
cube.posX(2);

// set skybox
GG.scene().skybox(waterEnvMap);

fun void fpser(dur d) {
    while (d => now) <<< GG.fps() >>>;
}
spork ~ fpser (1::second);

while (true) { 
    GG.nextFrame() => now; 
}