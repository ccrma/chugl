//--------------------------------------------------------------------
// name: skybox.ck
// desc: skybox and environment mapping
//
// requires: ChuGL + chuck-1.5.5.5 or higher
// 
// download: this example uses the bridge cubemap:
//   https://chuck.stanford.edu/chugl/examples/data/textures/bridge.zip
//   (place unzipped "bridge" folder in the same directory as this program)
// 
// author: Andrew Zhu Aday
//   date: Fall 2024
//--------------------------------------------------------------------

// load the cubemap
Texture.load(
    me.dir() + "./bridge/posx.jpg", // right
    me.dir() + "./bridge/negx.jpg", // left
    me.dir() + "./bridge/posy.jpg", // top
    me.dir() + "./bridge/negy.jpg", // bottom
    me.dir() + "./bridge/posz.jpg", // back
    me.dir() + "./bridge/negz.jpg"  // front
) @=> Texture cubemap;

// apply the cubemap to the scene
GG.scene().envMap(cubemap);
// IMPORTANT!!! Final color skybox *times* background color
GG.scene().backgroundColor(Color.WHITE);

// use orbit camera
GOrbitCamera camera => GG.scene().camera;

// connect suzanne to scene
GSuzanne suzanne --> GG.scene();
// enable environment mapping on suzanne
suzanne.envmapBlend(PhongMaterial.ENVMAP_BLEND_MULTIPLY);

[
    "None",
    "Reflect",
    "Refract",
] @=> string envmap_methods[];

[
    "None",
    "Add",
    "Multiply",
    "Mix",
] @=> string envmap_blend_modes[];

// UI variables
UI_Float3 background_color(GG.scene().backgroundColor());
UI_Float envmap_intensity(suzanne.envmapIntensity());
UI_Float refraction_ratio(suzanne.refractionRatio());
UI_Int envmap_method_index(suzanne.envmapMethod());
UI_Int envmap_blend_mode_index(suzanne.envmapBlend());

// render loop
while (true)
{
    // synchronize
    GG.nextFrame() => now;

    // begin UI
    if (UI.begin("Skybox"))
    {
        if (UI.colorEdit("Background Color", background_color, 0)) {
            GG.scene().backgroundColor(background_color.val());
        }

        if (UI.slider("Envmap Intensity", envmap_intensity, 0.0, 1.0)) {
            suzanne.envmapIntensity(envmap_intensity.val());
        }

        if (UI.slider("Refraction Ratio", refraction_ratio, 0.0, 1.0)) {
            suzanne.refractionRatio(refraction_ratio.val());
        }

        if (UI.listBox("Envmap Method", envmap_method_index, envmap_methods)) {
            suzanne.envmapMethod(envmap_method_index.val());
        }

        if (UI.listBox("Envmap Blend Mode", envmap_blend_mode_index, envmap_blend_modes)) {
            suzanne.envmapBlend(envmap_blend_mode_index.val());
        }
    }
    // end UI
    UI.end();
}
