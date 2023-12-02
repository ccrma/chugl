//-----------------------------------------------------------------------------
// name: bloom.ck
// desc: Showcases BloomFX in the ChuGL Post Processing Pipeline
//       
// requires: ChuGL 0.1.6 + chuck-1.5.2.0 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// Scene setup ============================================================
GCube cubeL --> GG.scene();
GSphere sphere --> GG.scene();
GCube cubeR --> GG.scene();

@(-1.5, 0, 0) => cubeL.translate;
@(0, 0, 0) => sphere.translate;
@(1.5, 0, 0) => cubeR.translate;

// take advantage of HDR colors be setting intensity > 1
5 => float intensity;

intensity * Color.RED => cubeL.mat().color;
intensity * Color.GREEN => sphere.mat().color;
intensity * Color.BLUE => cubeR.mat().color;

// FX Chain ===============================================================
GG.fx() --> BloomFX bloom --> OutputFX output;  // wasn't that simple?
2.0 => bloom.strength;
10 => bloom.levels;

// UI =====================================================================
UI_Window window;
window.text("Bloom Example");

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

// Generate UI based on FX chain =============================================
GG.fx().next() @=> FX @ effect;
0 => int effectIndex;
while (effect != null) {

    // get class name of effect
    Type.of(effect).baseName() => string baseName;

    <<< "Creating UI for pass", effectIndex, " | Type:", baseName >>>;

    UI_Text header;
    header.mode(UI_Text.MODE_SEPARATOR);
    header.text(baseName + " Pass: " + effectIndex);
    window.add(header);

    UI_Checkbox checkbox;
    checkbox.text("Bypass " + baseName);
    checkbox.val(false);    
    window.add(checkbox);
    spork ~ CheckboxListener(checkbox, effect);

    // create UI based on type
    if (baseName == "OutputFX") {
        // gamma
        UI_SliderFloat gammaSlider;
        gammaSlider.text("Gamma");
        <<< "init gamma: ", (effect$OutputFX).gamma() >>>;
        gammaSlider.range(0.1, 10);
        gammaSlider.val((effect$OutputFX).gamma());
        window.add(gammaSlider);
        spork ~ GammaListener(gammaSlider, effect$OutputFX);

        // exposure
        UI_SliderFloat exposureSlider;
        exposureSlider.text("Exposure");
        exposureSlider.range(0.01, 16);
        exposureSlider.val(1);
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
        strengthSlider.range(0, 2);
        strengthSlider.val((effect$BloomFX).strength());
        window.add(strengthSlider);
        spork ~ BloomStrengthListener(strengthSlider, effect$BloomFX);

        // radius
        UI_SliderFloat radiusSlider;
        radiusSlider.text("Bloom Radius");
        radiusSlider.range(0.001, .5);
        radiusSlider.val((effect$BloomFX).radius());
        window.add(radiusSlider);
        spork ~ BloomRadiusListener(radiusSlider, effect$BloomFX);

        // threshold
        UI_SliderFloat thresholdSlider;
        thresholdSlider.text("Bloom Threshold");
        thresholdSlider.range(0, 2);
        thresholdSlider.val((effect$BloomFX).threshold());
        window.add(thresholdSlider);
        spork ~ BloomThresholdListener(thresholdSlider, effect$BloomFX);

        // levels
        UI_SliderInt levelsSlider;
        levelsSlider.text("Bloom #Passes");
        levelsSlider.range(0, 16);
        levelsSlider.val((effect$BloomFX).levels());
        window.add(levelsSlider);
        spork ~ BloomLevelsListener(levelsSlider, effect$BloomFX);

        // blend mode
        UI_Dropdown blendDropdown;
        blendDropdown.text("Bloom Blend Mode");
        ["mix", "add"] @=> string blendOptions[];
        blendDropdown.options(blendOptions);
        blendDropdown.val((effect$BloomFX).blend());
        window.add(blendDropdown);
        spork ~ BloomBlendListener(blendDropdown, effect$BloomFX);

        // karis enabled
        UI_Checkbox karisCheckbox;
        karisCheckbox.text("Karis Bloom");
        window.add(karisCheckbox);
        spork ~ KarisEnabledListener(karisCheckbox, effect$BloomFX);

    }

    effect.next() @=> effect;
    effectIndex++;
}

// Main loop ==============================================================
while (true) { GG.nextFrame() => now; }