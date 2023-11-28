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
colorSpaceDropdown.val(1);

UI_Text separator;
separator.text("PhongMaterial environment mapping params");
separator.mode(UI_Text.MODE_SEPARATOR);

UI_Checkbox envMapEnabledCheckbox;
envMapEnabledCheckbox.text("Environment Mapping enabled");
envMapEnabledCheckbox.val(false);

UI_SliderFloat envMapIntensitySlider;
envMapIntensitySlider.text("Intensity");
envMapIntensitySlider.val(0);
envMapIntensitySlider.range(0, 4);

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
waterEnvMap.colorSpace(Texture.COLOR_SPACE_SRGB);

yokohamaEnvMap.paths(
    me.dir() + "./textures/skybox/Yokohama3/right.jpg",
    me.dir() + "./textures/skybox/Yokohama3/left.jpg",
    me.dir() + "./textures/skybox/Yokohama3/top.jpg",
    me.dir() + "./textures/skybox/Yokohama3/bottom.jpg",
    me.dir() + "./textures/skybox/Yokohama3/front.jpg",
    me.dir() + "./textures/skybox/Yokohama3/back.jpg"
);
yokohamaEnvMap.colorSpace(Texture.COLOR_SPACE_SRGB);

GG.scene() @=> GScene @ scene;
scene.skybox(waterEnvMap);

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

// GG.mouseMode(GG.MOUSE_LOCKED);

GSphere sphere --> GG.scene();
GCube cube --> GG.scene();
cube.mat().color(Color.RED);
cube.posX(2);

sphere.mat() @=> Material @ mat;

// Post Processing =======================================================
PassThroughFX pass1, pass2, pass3, pass4;
OutputFX output;
BloomFX bloom;
InvertFX invert;  // want to do AFTER tonemapping.
MonochromeFX monochrome;
GG.fx().next(bloom).next(output).next(invert).next(monochrome);

fun void CheckboxListener(UI_Checkbox @ checkbox, FX @ effect)
{
    while (true)
    {
        checkbox => now;
        effect.bypass(checkbox.val());
    }
}

fun void GammaListener(UI_SliderFloat @ gammaSlider, OutputFX @ output) {
    while (true) {
        gammaSlider => now;
        gammaSlider.val() => output.gamma;
        <<< "gamma set to", output.gamma() >>>;
    }
}

fun void ExposureListener(UI_SliderFloat @ exposureSlider, OutputFX @ output) {
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

fun void TonemapListener(UI_Dropdown @ tonemapDropdown, OutputFX @ output) {
    while (true) {
        tonemapDropdown => now;
        tonemapDropdown.val() => int val;
        if (val == 0) {
            output.toneMap(OutputFX.TONEMAP_NONE);
        } else if (val == 1) {
            output.toneMap(OutputFX.TONEMAP_LINEAR);
        } else if (val == 2) {
            output.toneMap(OutputFX.TONEMAP_REINHARD);
        } else if (val == 3) {
            output.toneMap(OutputFX.TONEMAP_CINEON);
        } else if (val == 4) {
            output.toneMap(OutputFX.TONEMAP_ACES);
        } else if (val == 5) {
            output.toneMap(OutputFX.TONEMAP_UNCHARTED);
        }
    }
}

fun void BloomStrengthListener(UI_SliderFloat @ strengthSlider, BloomFX @ bloom) {
    while (true) {
        strengthSlider => now;
        strengthSlider.val() => bloom.strength;
        <<< "bloom strength set to", bloom.strength() >>>;
    }
}

fun void BloomRadiusListener(UI_SliderFloat @ radiusSlider, BloomFX @ bloom) {
    while (true) {
        radiusSlider => now;
        radiusSlider.val() => bloom.radius;
        <<< "bloom radius set to", bloom.radius() >>>;
    }
}

fun void BloomThresholdListener(UI_SliderFloat @ thresholdSlider, BloomFX @ bloom) {
    while (true) {
        thresholdSlider => now;
        thresholdSlider.val() => bloom.threshold;
        <<< "bloom threshold set to", bloom.threshold() >>>;
    }
}

fun void BloomLevelsListener(UI_SliderInt @ levelsSlider, BloomFX @ bloom) {
    while (true) {
        levelsSlider => now;
        levelsSlider.val() => bloom.levels;
        <<< "bloom levels set to", bloom.levels() >>>;
    }
}

fun void BloomBlendListener(UI_Dropdown @ blendDropdown, BloomFX @ bloom) {
    while (true) {
        blendDropdown => now;
        blendDropdown.val() => int val;
        if (val == 0) {
            bloom.blend(BloomFX.BLEND_MIX);
        } else if (val == 1) {
            bloom.blend(BloomFX.BLEND_ADD);
        } 
    }
}


fun void KarisEnabledListener(UI_Checkbox @ karisCheckbox, BloomFX @ bloom) {
    while (true) {
        karisCheckbox => now;
        karisCheckbox.val() => bloom.karisAverage;
    }
}

fun void InvertMixSlider(UI_SliderFloat @ mixSlider, InvertFX @ invert) {
    while (true) {
        mixSlider => now;
        mixSlider.val() => invert.mix;
        <<< "invert mix set to", invert.mix() >>>;
    }
}

fun void MonochromeMixSlider(UI_SliderFloat @ mixSlider, MonochromeFX @ monochrome) {
    while (true) {
        mixSlider => now;
        mixSlider.val() => monochrome.mix;
        <<< "monochrome mix set to", monochrome.mix() >>>;
    }
}

fun void MonochromeColorListener(UI_Color3 @ colorPicker, MonochromeFX @ monochrome) {
    while (true) {
        colorPicker => now;
        colorPicker.val() => monochrome.color;
        <<< "monochrome color set to", monochrome.color() >>>;
    }
}

// create UI
GG.fx().next() @=> FX @ effect;
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
    Type.of(effect).baseName() => string baseName;
    if (baseName == "OutputFX") {
        // gamma
        UI_SliderFloat gammaSlider;
        gammaSlider.text("Gamma");
        gammaSlider.val((effect$OutputFX).gamma());
        <<< "init gamma: ", (effect$OutputFX).gamma() >>>;
        gammaSlider.range(0.1, 10);
        window.add(gammaSlider);
        spork ~ GammaListener(gammaSlider, effect$OutputFX);

        // exposure
        UI_SliderFloat exposureSlider;
        exposureSlider.text("Exposure");
        exposureSlider.val(1);
        exposureSlider.range(0.01, 16);
        window.add(exposureSlider);
        spork ~ ExposureListener(exposureSlider, effect$OutputFX);
        
        // tonemap
        UI_Dropdown tonemapDropdown;
        tonemapDropdown.text("Tone Mapping method");
        tonemapDropdown.options(tonemapOptions);
        tonemapDropdown.val(output.toneMap());
        window.add(tonemapDropdown);
        spork ~ TonemapListener(tonemapDropdown, effect$OutputFX);

    } else if (baseName == "BloomFX") {
        // strength
        UI_SliderFloat strengthSlider;
        strengthSlider.text("Bloom Strength");
        strengthSlider.val((effect$BloomFX).strength());
        strengthSlider.range(0, 2);
        window.add(strengthSlider);
        spork ~ BloomStrengthListener(strengthSlider, effect$BloomFX);

        // radius
        UI_SliderFloat radiusSlider;
        radiusSlider.text("Bloom Radius");
        radiusSlider.val((effect$BloomFX).radius());
        radiusSlider.range(0.001, 4);
        window.add(radiusSlider);
        spork ~ BloomRadiusListener(radiusSlider, effect$BloomFX);

        // threshold
        UI_SliderFloat thresholdSlider;
        thresholdSlider.text("Bloom Threshold");
        thresholdSlider.val((effect$BloomFX).threshold());
        thresholdSlider.range(0, 2);
        window.add(thresholdSlider);
        spork ~ BloomThresholdListener(thresholdSlider, effect$BloomFX);

        // levels
        UI_SliderInt levelsSlider;
        levelsSlider.text("Bloom #Passes");
        levelsSlider.val((effect$BloomFX).levels());
        levelsSlider.range(1, 10);
        window.add(levelsSlider);
        spork ~ BloomLevelsListener(levelsSlider, effect$BloomFX);

        // blend mode
        UI_Dropdown blendDropdown;
        blendDropdown.text("Bloom Blend Mode");
        ["mix", "add"] @=> string blendOptions[];
        blendDropdown.options(blendOptions);
        blendDropdown.val(0);
        window.add(blendDropdown);
        spork ~ BloomBlendListener(blendDropdown, effect$BloomFX);

        // karis enabled
        UI_Checkbox karisCheckbox;
        karisCheckbox.text("Karis Bloom");
        window.add(karisCheckbox);
        spork ~ KarisEnabledListener(karisCheckbox, effect$BloomFX);

    } else if (baseName == "InvertFX") {
        // mix
        UI_SliderFloat mixSlider;
        mixSlider.text("Invert Mix");
        mixSlider.val((effect$InvertFX).mix());
        mixSlider.range(0, 1);
        window.add(mixSlider);
        spork ~ InvertMixSlider(mixSlider, effect$InvertFX);
    } else if (baseName == "MonochromeFX") {
        // mix
        UI_SliderFloat mixSlider;
        mixSlider.text("Monochrome Mix");
        mixSlider.range(0, 1);
        mixSlider.val((effect$MonochromeFX).mix());
        window.add(mixSlider);
        spork ~ MonochromeMixSlider(mixSlider, effect$MonochromeFX);

        // color
        UI_Color3 colorPicker;
        colorPicker.text("Monochrome Color");
        window.add(colorPicker);
        spork ~ MonochromeColorListener(colorPicker, effect$MonochromeFX);
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
fun void setRefCountedSkybox() {
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
// setRefCountedSkybox();

fun void fpser(dur d) {
    while (d => now) <<< GG.fps() >>>;
}
spork ~ fpser (1::second);

while (true) { 
    GG.nextFrame() => now; 
}