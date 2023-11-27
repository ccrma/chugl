// controls ==============================================================
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// Scene setup ============================================================

GSphere sphere --> GG.scene();
GCube cube --> GG.scene();

@(-1, 0, 0) => sphere.translate;
@(1, 0, 0) => cube.translate;

@(5, 5, 5) => sphere.mat().color;
@(0, 5, 0) => cube.mat().color;

// FX Chain ===============================================================
// GG.renderPass() --> PP_Bloom bloom --> PP_Output output;
PP_Bloom bloom; PP_Output output;
GG.renderPass().next(bloom).next(output);

// UI =====================================================================
UI_Window window;
window.text("bloom test");
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

fun void BloomStrengthListener(UI_SliderFloat @ strengthSlider, PP_Bloom @ bloom) {
    while (true) {
        strengthSlider => now;
        strengthSlider.val() => bloom.strength;
        <<< "bloom strength set to", bloom.strength() >>>;
    }
}

fun void BloomRadiusListener(UI_SliderFloat @ radiusSlider, PP_Bloom @ bloom) {
    while (true) {
        radiusSlider => now;
        radiusSlider.val() => bloom.radius;
        <<< "bloom radius set to", bloom.radius() >>>;
    }
}

fun void BloomThresholdListener(UI_SliderFloat @ thresholdSlider, PP_Bloom @ bloom) {
    while (true) {
        thresholdSlider => now;
        thresholdSlider.val() => bloom.threshold;
        <<< "bloom threshold set to", bloom.threshold() >>>;
    }
}

fun void BloomLevelsListener(UI_SliderInt @ levelsSlider, PP_Bloom @ bloom) {
    while (true) {
        levelsSlider => now;
        levelsSlider.val() => bloom.levels;
        <<< "bloom levels set to", bloom.levels() >>>;
    }
}

fun void BloomBlendListener(UI_Dropdown @ blendDropdown, PP_Bloom @ bloom) {
    while (true) {
        blendDropdown => now;
        blendDropdown.val() => int val;
        if (val == 0) {
            bloom.blend(PP_Bloom.BLEND_MIX);
        } else if (val == 1) {
            bloom.blend(PP_Bloom.BLEND_ADD);
        } 
    }
}


fun void KarisEnabledListener(UI_Checkbox @ karisCheckbox, PP_Bloom @ bloom) {
    while (true) {
        karisCheckbox => now;
        karisCheckbox.val() => bloom.karisAverage;
    }
}

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
    Type.of(effect).baseName() => string baseName;
    if (baseName == "PP_Output") {
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
        tonemapDropdown.val(output.toneMap());
        window.add(tonemapDropdown);
        spork ~ TonemapListener(tonemapDropdown, effect$PP_Output);

    } else if (baseName == "PP_Bloom") {
        // strength
        UI_SliderFloat strengthSlider;
        strengthSlider.text("Bloom Strength");
        strengthSlider.val((effect$PP_Bloom).strength());
        strengthSlider.range(0, 2);
        window.add(strengthSlider);
        spork ~ BloomStrengthListener(strengthSlider, effect$PP_Bloom);

        // radius
        UI_SliderFloat radiusSlider;
        radiusSlider.text("Bloom Radius");
        radiusSlider.val((effect$PP_Bloom).radius());
        radiusSlider.range(0.001, .01);
        window.add(radiusSlider);
        spork ~ BloomRadiusListener(radiusSlider, effect$PP_Bloom);

        // threshold
        UI_SliderFloat thresholdSlider;
        thresholdSlider.text("Bloom Threshold");
        thresholdSlider.val((effect$PP_Bloom).threshold());
        thresholdSlider.range(0, 2);
        window.add(thresholdSlider);
        spork ~ BloomThresholdListener(thresholdSlider, effect$PP_Bloom);

        // levels
        UI_SliderInt levelsSlider;
        levelsSlider.text("Bloom #Passes");
        levelsSlider.val((effect$PP_Bloom).levels());
        levelsSlider.range(1, 10);
        window.add(levelsSlider);
        spork ~ BloomLevelsListener(levelsSlider, effect$PP_Bloom);

        // blend mode
        UI_Dropdown blendDropdown;
        blendDropdown.text("Bloom Blend Mode");
        ["mix", "add"] @=> string blendOptions[];
        blendDropdown.options(blendOptions);
        blendDropdown.val(0);
        window.add(blendDropdown);
        spork ~ BloomBlendListener(blendDropdown, effect$PP_Bloom);

        // karis enabled
        UI_Checkbox karisCheckbox;
        karisCheckbox.text("Karis Bloom");
        window.add(karisCheckbox);
        spork ~ KarisEnabledListener(karisCheckbox, effect$PP_Bloom);

    }

    effect.next() @=> effect;
    effectIndex++;
}

// Main loop ==============================================================
while (true) { GG.nextFrame() => now; }