@import "vis.ck"

// SndBuf buf(me.dir() + "./voice-sample.wav") => BPF bpf(1500, 7) => Bitcrusher bc => Gain g => dac;
SndBuf buf(me.dir() + "./voice-sample.wav") => LPF lpf(2200, 1) => HPF hpf(1200, 3) => Bitcrusher bc => LPF bpf(4000, 1) => Gain g => dac;
true => buf.loop;
3 => hpf.gain;

7 => bc.bits;
12 => bc.downsample;

UI_Float bpf_gain(bpf.gain());
UI_Float q(bpf.Q());
UI_Float freq(bpf.freq());

UI_Float lpf_gain(lpf.gain());
UI_Float lpf_q(lpf.Q());
UI_Float lpf_freq(lpf.freq());

UI_Float hpf_gain(hpf.gain());
UI_Float hpf_q(hpf.Q());
UI_Float hpf_freq(hpf.freq());

UI_Int bits(bc.bits());
UI_Int ds(bc.downsample());

Vis v(g);
spork ~ v.run();

while (1) {
    GG.nextFrame() => now;

    if (UI.slider("lpf Q", lpf_q, .15, 10)) lpf.Q(lpf_q.val());
    if (UI.slider("lpf freq", lpf_freq, 20, 8000)) lpf.freq(lpf_freq.val());
    if (UI.slider("lpf gain", lpf_gain, .15, 10)) lpf.gain(lpf_gain.val());

    if (UI.slider("hpf Q", hpf_q, .15, 10)) hpf.Q(hpf_q.val());
    if (UI.slider("hpf freq", hpf_freq, 20, 8000)) hpf.freq(hpf_freq.val());
    if (UI.slider("hpf gain", hpf_gain, .15, 10)) hpf.gain(hpf_gain.val());

    if (UI.slider("bits", bits, 1, 32)) bc.bits(bits.val());
    if (UI.slider("downsample", ds, 1, 32)) bc.downsample(ds.val());

    if (UI.slider("Q", q, .15, 10)) bpf.Q(q.val());
    if (UI.slider("freq", freq, 20, 8000)) bpf.freq(freq.val());
    if (UI.slider("bpf_gain", bpf_gain, .15, 10)) bpf.gain(bpf_gain.val());
}