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

UI_Dropdown colorSpaceDropdown;
colorSpaceDropdown.text("Skybox Color Space");
["linear", "srgb"] @=> string colorSpaceOptions[];
colorSpaceDropdown.options(colorSpaceOptions);

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
window.add(colorSpaceDropdown);
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



// scene setup ===========================================================


GSphere sphere --> GG.scene();
GCube cube --> GG.scene();
cube.mat().color(Color.RED);
cube.posX(2);

sphere.mat() @=> Material @ mat;

// Post Processing =======================================================
PP_PassThrough pass1, pass2, pass3, pass4;
PP_Output output;
// GG.renderPass().next(pass1); // .next(pass2).next(pass3).next(pass4);
GG.renderPass().next(pass1).next(pass2).next(output); // .next(pass2).next(pass3).next(pass4);

fun void CheckboxListener(UI_Checkbox @ checkbox, PP_Effect @ effect)
{
    while (true)
    {
        checkbox => now;
        effect.bypass(checkbox.val());
    }
}

fun void GammaListener(UI_SliderFloat @ gammaSlider, PP_Output @ output) {
    while (true) {
        gammaSlider => now;
        gammaSlider.val() => output.gamma;
        <<< "gamma set to", output.gamma() >>>;
    }
}

fun void ExposureListener(UI_SliderFloat @ exposureSlider, PP_Output @ output) {
    while (true) {
        exposureSlider => now;
        exposureSlider.val() => output.exposure;
        <<< "exposure set to", output.exposure() >>>;
    }
}
[
    "None",
    "Linear",
    "Reinhard",
    "Cineon",
    "ACES",
    "Uncharted"
] @=> string tonemapOptions[];

fun void TonemapListener(UI_Dropdown @ tonemapDropdown, PP_Output @ output) {
    while (true) {
        tonemapDropdown => now;
        tonemapDropdown.val() => int val;
        if (val == 0) {
            output.toneMap(PP_Output.TONEMAP_NONE);
        } else if (val == 1) {
            output.toneMap(PP_Output.TONEMAP_LINEAR);
        } else if (val == 2) {
            output.toneMap(PP_Output.TONEMAP_REINHARD);
        } else if (val == 3) {
            output.toneMap(PP_Output.TONEMAP_CINEON);
        } else if (val == 4) {
            output.toneMap(PP_Output.TONEMAP_ACES);
        } else if (val == 5) {
            output.toneMap(PP_Output.TONEMAP_UNCHARTED);
        }
    }
}

<<< "Texture.COLOR_SPACE_LINEAR", Texture.COLOR_SPACE_LINEAR >>>;
<<< "Texture.COLOR_SPACE_SRGB", Texture.COLOR_SPACE_SRGB >>>;

// create UI
GG.renderPass().next() @=> PP_Effect @ effect;
0 => int effectIndex;
while (effect != null) {
    // effect.UI() @=> UI_Element @ effectUI;
    // effectUI.text("pass " + effectIndex);
    // window.add(effectUI);

    <<< "Creating UI for pass", effectIndex, "Type:", Type.of(effect).baseName() >>>;

    UI_Checkbox checkbox;
    checkbox.text("pass " + effectIndex);
    checkbox.val(false);    
    window.add(checkbox);
    spork ~ CheckboxListener(checkbox, effect);

    // create UI based on type
    if (Type.of(effect).baseName() == "PP_Output") {
        // gamma
        UI_SliderFloat gammaSlider;
        gammaSlider.text("Gamma");
        gammaSlider.val((effect$PP_Output).gamma());
        <<< "init gamma: ", (effect$PP_Output).gamma() >>>;
        gammaSlider.range(0.1, 10);
        window.add(gammaSlider);
        spork ~ GammaListener(gammaSlider, effect$PP_Output);

        // exposure
        UI_SliderFloat exposureSlider;
        exposureSlider.text("Exposure");
        exposureSlider.val(1);
        exposureSlider.range(0.01, 16);
        window.add(exposureSlider);
        spork ~ ExposureListener(exposureSlider, effect$PP_Output);
        
        // tonemap
        UI_Dropdown tonemapDropdown;
        tonemapDropdown.text("Tone Mapping method");
        tonemapDropdown.options(tonemapOptions);
        tonemapDropdown.val(0);
        window.add(tonemapDropdown);
        spork ~ TonemapListener(tonemapDropdown, effect$PP_Output);

    }

    effect.next() @=> effect;
    effectIndex++;
}

// pass1.UI() @=> UI_Element @ effectUI;
// effectUI.text("passthrough1");
// window.add(effectUI);

// output.UI() @=> UI_Element @ outputUI;
// outputUI.text("output pass");
// window.add(outputUI);

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