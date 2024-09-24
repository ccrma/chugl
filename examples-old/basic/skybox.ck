//-----------------------------------------------------------------------------
// name: gtext.ck
// desc: showcases GText features
//       
// requires: ChuGL 0.1.6 + chuck-1.5.2.0 or higher
// skyboxes: https://www.humus.name/Textures/Yokohama3.zip
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// To run this example, download the skybox in the links above, and 
// put it in the ../assets directory

// Post Processing =======================================================
GG.fx() --> OutputFX output;  // enable tone-mapping so colors aren't blown out

// Scene setup ===========================================================

GSphere sphere --> GG.scene();
sphere.mat() @=> Material @ mat;

// enable environment mapping
mat.envMapEnabled(true);
mat.envMapIntensity(4.0);

// initialize cubemap textures for skybox
CubeTexture yokohamaEnvMap;

// set filepaths, must be in this order (right, left, top, bottom, front, back)
yokohamaEnvMap.paths(
    me.dir() + "../assets/Yokohama3/right.jpg",
    me.dir() + "../assets/Yokohama3/left.jpg",
    me.dir() + "../assets/Yokohama3/top.jpg",
    me.dir() + "../assets/Yokohama3/bottom.jpg",
    me.dir() + "../assets/Yokohama3/front.jpg",
    me.dir() + "../assets/Yokohama3/back.jpg"
);

// IMPORTANT: need to set SRGB color space if applying gamma correction via OutputFX
// so that the final output is not double gamma corrected
yokohamaEnvMap.colorSpace(Texture.COLOR_SPACE_SRGB);

GG.camera() --> GGen dolly --> GG.scene();
GG.scene() @=> GScene @ scene;
scene.skybox(yokohamaEnvMap);     // set the scene's skybox

// UI ===================================================================

UI_Window window;
window.text("Skybox Example");

UI_Text skyboxSeparator;
skyboxSeparator.text("Skybox params");
skyboxSeparator.mode(UI_Text.MODE_SEPARATOR);

// Skybox params
UI_Checkbox skyboxCheckbox;
skyboxCheckbox.text("Enabled Skybox");
skyboxCheckbox.val(true);

UI_Dropdown colorSpaceDropdown;
colorSpaceDropdown.text("Skybox Color Space");
["linear", "srgb"] @=> string colorSpaceOptions[];
colorSpaceDropdown.options(colorSpaceOptions);
colorSpaceDropdown.val(yokohamaEnvMap.colorSpace());

UI_Text separator;
separator.text("PhongMaterial environment mapping params");
separator.mode(UI_Text.MODE_SEPARATOR);

// Environment Mapping params
UI_Checkbox envMapEnabledCheckbox;
envMapEnabledCheckbox.text("Environment Mapping enabled");
envMapEnabledCheckbox.val(mat.envMapEnabled());

UI_SliderFloat envMapIntensitySlider;
envMapIntensitySlider.text("Intensity");
envMapIntensitySlider.val(mat.envMapIntensity());
envMapIntensitySlider.range(0, 8);

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
envMapRatioSlider.val(mat.envMapRefractionRatio());
envMapRatioSlider.range(0, 1);


window.add(skyboxSeparator);
window.add(colorSpaceDropdown);
window.add(skyboxCheckbox);
window.add(separator);
window.add(envMapEnabledCheckbox);
window.add(envMapIntensitySlider);
window.add(envMapMethodDropdown);
window.add(envMapBlendDropdown);
window.add(envMapRatioSlider);

// Skybox UI Listeners ==========================================================

fun void ColorSpaceDropdownListener() {
    while (true) {
        colorSpaceDropdown => now;
        colorSpaceDropdown.val() => int val;
        if (val == 0) {
            scene.skybox().colorSpace(Texture.COLOR_SPACE_LINEAR);
        } else if (val == 1) {
            scene.skybox().colorSpace(Texture.COLOR_SPACE_SRGB);
        }
    }
} spork ~ ColorSpaceDropdownListener();

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


// Material UI Listeners =================================================== 

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

// Game Loop =============================================================
while (true) { 
    .2 * GG.dt() => dolly.rotateY;
    GG.nextFrame() => now; 
}
