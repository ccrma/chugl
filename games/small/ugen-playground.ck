@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "PlinkyRev"

/*
PhISEM (Physically Informed Stochastic Event Modeling) is an algorithmic approach for simulating collisions of multiple independent sound producing objects. 
This class is a meta-model that can simulate a Maraca, Sekere, Cabasa, Bamboo Wind Chimes, Water Drops, Tambourine, Sleighbells, and a Guiro. 
PhOLIES (Physically-Oriented Library of Imitated Environmental Sounds) is a similar approach for the synthesis of environmental sounds. 
This class implements simulations of breaking sticks, crunchy snow (or not), a wrench, sandpaper, and more. 
Control Change Numbers: 
    - Shake Energy = 2 
    - System Decay = 4 
    - Number Of Objects = 11 
    - Resonance Frequency = 1 
    - Shake Energy = 128 
    - Instrument Selection = 1071 
        - Maraca = 0 
        - Cabasa = 1 
        - Sekere = 2 
        - Guiro = 3 
        - Water Drops = 4 
        - Bamboo Chimes = 5 
        - Tambourine = 6 
        - Sleigh Bells = 7 
        - Sticks = 8 
        - Crunch = 9 
        - Wrench = 10 
        - Sand Paper = 11 
        - Coke Can = 12 
        - Next Mug = 13 
        - Penny + Mug = 14 
        - Nickle + Mug = 15 
        - Dime + Mug = 16 
        - Quarter + Mug = 17 
        - Franc + Mug = 18 
        - Peso + Mug = 19 
        - Big Rocks = 20 
        - Little Rocks = 21 
        - Tuned Bamboo Chimes = 22 
ModalBar
This class implements a number of different struck bar instruments. 
It inherits from the Modal class. 
Control Change Numbers: 
- Stick Hardness = 2 
- Stick Position = 4 
- Vibrato Gain = 11 
- Vibrato Frequency = 7 
- Direct Stick Mix = 1 
- Volume = 128 
- Modal Presets = 16 
    - Marimba = 0 
    - Vibraphone = 1 
    - Agogo = 2 
    - Wood1 = 3 
    - Reso = 4 
    - Wood2 = 5 
    - Beats = 6 
    - Two Fixed = 7 
    - Clump = 8 

*/

[
    "Maraca",
    "Cabasa",
    "Sekere",
    "Guiro",
    "Water Drops",
    "Bamboo Chimes",
    "Tambourine",
    "Sleigh Bells",
    "Sticks",
    "Crunch",
    "Wrench",
    "Sand Paper",
    "Coke Can",
    "Next Mug",
    "Penny + Mug",
    "Nickle + Mug",
    "Dime + Mug",
    "Quarter + Mug",
    "Franc + Mug",
    "Peso + Mug",
    "Big Rocks",
    "Little Rocks",
    "Tuned Bamboo Chimes",
] @=> string shaker_presets[];

PlinkyRev rev;
rev.help();

Bamboo boo => dac;

UI_Int shaker_preset_idx(boo.shake.which());
UI_Float shaker_gain(1.0);
UI_Float shaker_objects(boo.shake.objects());
UI_Float shaker_freq(Std.ftom(boo.shake.freq()));


// modal bar params
ModalBar bar => dac;
[
    "Marimba",
    "Vibraphone",
    "Agogo",
    "Wood1",
    "Reso",
    "Wood2",
    "Beats",
    "Two Fixed",
    "Clump",
] @=> string bar_presets[];

UI_Int  bar_preset(bar.preset());
UI_Float  bar_stickHardness(bar.stickHardness());
UI_Float  bar_strikePosition(bar.strikePosition());
UI_Float  bar_vibratoGain(bar.vibratoGain());
UI_Float  bar_vibratoFreq(bar.vibratoFreq());
UI_Float  bar_volume(bar.volume());
UI_Float  bar_freq(Std.ftom(bar.freq()));


class Bamboo extends Chugraph
{
    // Shakers shake => Dyno d => outlet;

    // shake =< d;
    // shake => outlet;
    // 8 => shake.gain;

    Dyno d;
    Shakers shake => LPF lpf(4000) => outlet;
    8 => shake.gain;

    // Trickly kind of shaker
    shake.which(22);

    // Compressor
    d.compress();
    2000::ms => d.releaseTime;
    0.2 => d.thresh;
    0.33 => d.slopeAbove;
    8 => d.gain;

    fun void noteOn(float gain) 
    {
        // Inverse gain to number of objects, max 30
        // 30 - (29*gain) => shake.objects;
        // gain * 2 => shake.noteOn;

        gain => shake.noteOn;
    }

    fun void noteOff() { 
        shake.noteOff(.1);
    }

    fun void freq(float freq) 
    {
        freq => shake.freq;
    }

    fun float freq() 
    {
        return shake.freq();
    }
}

while (1) {
    GG.nextFrame() => now;

    // shaker
    if (UI.listBox("preset", shaker_preset_idx, shaker_presets)) shaker_preset_idx.val() => boo.shake.preset;
    UI.slider("shaker_gain", shaker_gain, 0, 1);
    if (UI.slider("shaker_objects", shaker_objects, 0, 128)) shaker_objects.val() => boo.shake.objects;
    if (UI.slider("shaker_freq", shaker_freq, 0, 120)) Std.mtof(shaker_freq.val()) => boo.freq;

    // modal bar
    if (UI.listBox("bar preset", bar_preset, bar_presets)) bar_preset.val() => bar.preset;
    if (UI.slider("bar.stickHardness", bar_stickHardness, 0, 1 )) bar_stickHardness.val() => bar.stickHardness;
    if (UI.slider("bar.strikePosition", bar_strikePosition, 0, 1 )) bar_strikePosition.val() => bar.strikePosition;
    if (UI.slider("bar.vibratoGain", bar_vibratoGain, 0, 1 )) bar_vibratoGain.val() => bar.vibratoGain;
    if (UI.slider("bar.vibratoFreq", bar_vibratoFreq, 0, 60 )) bar_vibratoFreq.val() => bar.vibratoFreq;
    if (UI.slider("bar.volume", bar_volume, 0, 1 )) bar_volume.val() => bar.volume;
    if (UI.slider("bar.freq", bar_freq, 0, 120)) Std.mtof(bar_freq.val()) => bar.freq;

    // if (GWindow.mouseLeftDown()) boo.noteOn(shaker_gain.val());
    // if (GWindow.mouseLeftUp()) boo.noteOff();

    if (GWindow.keyDown(GWindow.Key_Right)) boo.noteOn(shaker_gain.val());
    if (GWindow.keyUp(GWindow.Key_Right)) boo.noteOff();

    if (GWindow.keyDown(GWindow.Key_Space)) bar.noteOn(1.0);
    // if (GWindow.keyUp(GWindow.Key_Space)) bar.noteOff();
}