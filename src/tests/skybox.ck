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

UI_Text separator;
separator.text("PhongMaterial environment mapping params");
separator.mode(UI_Text.MODE_SEPARATOR);

UI_Checkbox envMapEnabledCheckbox;
envMapEnabledCheckbox.text("Environment Mapping enabled");
envMapEnabledCheckbox.val(false);

UI_SliderFloat envMapIntensitySlider;
envMapIntensitySlider.text("Intensity");
envMapIntensitySlider.val(0);
envMapIntensitySlider.range(0, 1);

UI_Dropdown envMapMethodDropdown;
envMapMethodDropdown.text("Type");
["reflection", "refraction"] @=> string envMapMethodOptions[];
envMapMethodDropdown.options(envMapMethodOptions);

UI_Dropdown envMapBlendDropdown;
envMapBlendDropdown.text("Blend");
["mult", "add", "mix"] @=> string envMapBlendOptions[];
envMapBlendDropdown.options(envMapBlendOptions);

UI_SliderFloat envMapRatioSlider;
envMapRatioSlider.text("Refraction Ratio");
envMapRatioSlider.val(0);
envMapRatioSlider.range(0, 1);


window.add(dropdown);
window.add(skyboxCheckbox);
window.add(separator);
window.add(envMapEnabledCheckbox);
window.add(envMapIntensitySlider);
window.add(envMapMethodDropdown);
window.add(envMapBlendDropdown);
window.add(envMapRatioSlider);


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

sphere.mat() @=> Material @ mat;

// Material UI options =================================================== 

fun void EnvMapToggleListener() {
    while (true) {
        envMapEnabledCheckbox => now;
        envMapEnabledCheckbox.val() => int val;
        mat.envMapEnabled(val);
    }
} spork ~ EnvMapToggleListener();

fun void EnvMapIntensityListener() {
    while (true) {
        envMapIntensitySlider => now;
        envMapIntensitySlider.val() => float val;
        mat.envMapIntensity(val);
    }
} spork ~ EnvMapIntensityListener();

fun void envMapMethodListener() {
    while (true) {
        envMapMethodDropdown => now;
        envMapMethodDropdown.val() => int val;
        if (val == 0) {
            mat.envMapMethod(Material.ENV_MAP_REFLECT);
        } else if (val == 1) {
            mat.envMapMethod(Material.ENV_MAP_REFRACT);
        }
    }
} spork ~ envMapMethodListener();

fun void EnvMapBlendListener() {
    while (true) {
        envMapBlendDropdown => now;
        envMapBlendDropdown.val() => int val;
        if (val == 0) {
            mat.envMapBlend(Material.BLEND_MODE_MULTIPLY);
        } else if (val == 1) {
            mat.envMapBlend(Material.BLEND_MODE_ADD);
        } else if (val == 2) {
            mat.envMapBlend(Material.BLEND_MODE_MIX);
        }
    }
} spork ~ EnvMapBlendListener();

fun void EnvMapRatioListener() {
    while (true) {
        envMapRatioSlider => now;
        envMapRatioSlider.val() => float val;
        mat.envMapRefractionRatio(val);
    }
} spork ~ EnvMapRatioListener();

// set skybox
// if refcounting happens correctly, this should be fine
fun void setSkybox() {
    CubeTexture refCountedEnvMap;
    refCountedEnvMap.paths(
        me.dir() + "./textures/skybox/water/right.jpg",
        me.dir() + "./textures/skybox/water/left.jpg",
        me.dir() + "./textures/skybox/water/top.jpg",
        me.dir() + "./textures/skybox/water/bottom.jpg",
        me.dir() + "./textures/skybox/water/front.jpg",
        me.dir() + "./textures/skybox/water/back.jpg"
    );
    GG.scene().skybox(refCountedEnvMap);
} 
setSkybox();

fun void fpser(dur d) {
    while (d => now) <<< GG.fps() >>>;
}
spork ~ fpser (1::second);

while (true) { 
    GG.nextFrame() => now; 
}