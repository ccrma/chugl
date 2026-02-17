@import "vis.ck"

// SndBuf buf(me.dir() + "./voice-sample.wav");
SndBuf buf(me.dir() + "./villager.wav");
1 => buf.loop;
6 => buf.gain;

public class iRobot extends Chugraph {
    // our patch - feedforward part
    inlet => Bitcrusher bc => Gain g => DelayL d => HPF hpf(1000, 2) => outlet;
    inlet => Gain g2 => outlet;
    // Gain g2; // unused
    // feedback
    d => Gain g3 => d;

    .5::second => d.max;
    // 6 => bc.bits;
    // 10 => bc.downsample;

    // set parameters
    15::ms => d.delay;
    0.5 => g.gain;
    0.1 => g2.gain;
    0.7 => g3.gain;

    UI_Float g_gain(g.gain());
    UI_Float g2_gain(g2.gain());
    UI_Float g3_gain(g3.gain());

    UI_Float delay_ms(d.delay() / ms);

    fun void ui() { 
        if (UI.slider("delay gain", g_gain, 0, 5)) g_gain.val() => g.gain;
        if (UI.slider("dry gain", g2_gain, 0, 5)) g2_gain.val() => g2.gain;
        if (UI.slider("feedback gain", g3_gain, 0, .99)) g3_gain.val() => g3.gain;

        d.delay()/ms => delay_ms.val;
        if (UI.slider("delay(ms)", delay_ms, 0, 100)) delay_ms.val()::ms => d.delay;
    }
}

buf => iRobot irobot => NRev rev => dac;
.025 => rev.mix;

SinOsc lfo => blackhole;
.2 => lfo.freq;

while (1) {
    GG.nextFrame() => now;
    irobot.ui();

    // Flanging (range: 1~10ms)
    Math.remap(
        lfo.last(),
        -1, 1,
        2, 10
    )::ms => irobot.d.delay;
    <<< lfo.last(), irobot.d.delay() >>>;
}