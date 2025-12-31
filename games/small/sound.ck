// CKFXR custom Chugraph UGen

// delay-based effects including flange, chorus, doubling
//   https://github.com/ccrma/music220a/blob/main/07-time-and-space/delay-based-efx/script.js
//   https://ccrma.stanford.edu/~dattorro/EffectDesignPart2.pdf
public class DelayFX extends Chugraph
{
    // audio signal
    inlet => DelayL delay => outlet;
    delay => Gain feedback => delay;

    delay.max(1::second); // max delay time

    // control signal
    SinOsc lfo_delay_mod => blackhole;
    0 => float delay_samps;

    // modulator
    spork ~ modulate();
    fun void modulate()
    {
        // remap from [-1, 1] to [0, 2*delay_samps]
        while (1::ms => now) {
            lfo_delay_mod.last() + 1 => float mod;
            (1 + mod * delay_samps)::samp => delay.delay;
        }
    }

    // default to flanger
    flange();  

    // params
    // float feedback_gain 
    // delay_base (center delay duration)
    // delay_mod_freq (lfo to modulate delay length)
    // delay_mod_depth (delay_base is modulated between [delay_base*(1 - depth), delay_base * (1 + depth)])

    // public API ---------------------------------------------
    fun void flange() {
        // typical range: 1~10ms
        delayBase(5::ms);
        feedbackGain(.85);
        delayModFreq(.1);
        delayModDepth(.80);  // 80% depth. oscillate between (1ms, 9ms)
    }

    fun void chorus() {
        // typical range: 5~30ms
        delayBase(12::ms);
        feedbackGain(.55);
        delayModFreq(.2);
        delayModDepth(.50);  // 50% depth. oscillate between (6ms, 18ms)
    }

    fun void double() {
        // typical range: 20~100ms
        delayBase(60::ms);
        feedbackGain(.45);
        delayModFreq(.1);
        delayModDepth(.2);  // 20% depth. oscillate between (48ms, 72ms)
    }

    // Setters
    fun void feedbackGain(float f) { f => this.feedback.gain; }
    fun void delayBase(dur d) {
        delay.delay(d);
        d / samp => delay_samps;
    }
    fun void delayModFreq(float f) { lfo_delay_mod.freq(f); }
    fun void delayModDepth(float f) { lfo_delay_mod.gain(f); }

    // Getters 
    fun float feedbackGain() { return feedback.gain(); }
    fun dur delayBase() { return delay.delay(); }
    fun float delayModFreq() { return lfo_delay_mod.freq(); }
    fun float delayModDepth() { return lfo_delay_mod.gain(); }
}

// custom CKFXR UGen
public class CKFXR extends Chugraph
{
    LPF lpf => HPF hpf => DelayFX delayfx => ADSR adsr => outlet;
    PulseOsc square; SawOsc saw; SinOsc sin; Noise noise;
    [square, saw, sin, noise] @=> UGen waveforms[];

    // modulators
    SinOsc lfo_vibrato => blackhole;
    TriOsc lfo_pwm(0.0) => blackhole;

    // WaveType enum
    0 => static int WaveType_SQUARE;
    1 => static int WaveType_SAW;
    2 => static int WaveType_SIN;
    3 => static int WaveType_NOISE;

    // Synth params parameters are on [0,1] unless noted SIGNED & thus
    // on [-1,1]
    0 => int p_wave_type;

    // env
    dur p_attack_dur;
    dur p_release_dur; 
    dur p_sustain_dur;  // noteOn - noteOff
    float p_sustain_level;

    // freq (in midi for linear properties) 
    float p_freq_base_midi;    // Start frequency. doing midi from [12, 127] ~ [16.345hz, 12543.85hz]
    float p_freq_limit_midi;   // Min frequency cutoff, anything below is stopped 
    float p_freq_ramp;         // Slide in semitones / s (SIGNED)
    float p_freq_dramp;        // Delta slide semitones / s^2 (SIGNED)

    // Vibrato
    float p_vib_depth; // Vibrato depth [0, 1]
    float p_vib_freq;    // Vibrato speed

    // Tonal change
    float p_arp_mod_midi;      // Freq Change amount (SIGNED)
    dur p_arp_time;          // how long after attack to apply arp_mod

    // PWM
    float p_pwm_depth;         
    float p_pwm_freq;          

    // Flanger
    float p_feedback_gain; 
    dur p_delay_base_dur;
    float p_delay_mod_freq;
    float p_delay_mod_depth;

    // Low-pass filter
    float p_lpf_freq;      // Low-pass filter cutoff
    float p_lpf_ramp;      // Low-pass filter cutoff sweep (semitones / s)
    float p_lpf_resonance; // Low-pass filter resonance

    // High-pass filter
    float p_hpf_freq;     // High-pass filter cutoff
    float p_hpf_ramp;     // High-pass filter cutoff sweep (semitones / s)
    
    // initialize values
    resetParams();

    // pwm modulator
    spork ~ pwmModulate();
    fun void pwmModulate()
    {
        while (1::ms => now) {
            Math.remap(lfo_pwm.last(), -1, 1, .001, .999) => square.width;
        }
    }
    // public API
    fun void resetParams()
    {
        WaveType_SQUARE => p_wave_type;

        // env
        1::ms => p_attack_dur;
        1::ms => p_release_dur;
        1::second => p_sustain_dur;
        1.0 => p_sustain_level;

        // freq 
        60.0 => p_freq_base_midi;    
        0 => p_freq_limit_midi;
        0 => p_freq_ramp;    
        0 => p_freq_dramp;   

        // vibrato
        0 => p_vib_depth;
        0 => p_vib_freq; 

        // arp
        0 => p_arp_mod_midi;      
        0::ms => p_arp_time;      

        // pwm 
        0 => p_pwm_depth;         
        0 => p_pwm_freq;          

        // delay 
        0 => p_feedback_gain; 
        0::ms => p_delay_base_dur;
        0 => p_delay_mod_freq;
        0 => p_delay_mod_depth;

        // lpf
        20000 => p_lpf_freq;    
        0 => p_lpf_ramp;      
        1 => p_lpf_resonance; 

        // hpf
        20 => p_hpf_freq;    
        0 => p_hpf_ramp;    
    }

    fun void printParams() {
        <<< "wavetype", p_wave_type >>>;

        // env
        <<< "--envelope------------" >>>;
        <<< "attack_dur", p_attack_dur >>>;
        <<< "release_dur", p_release_dur >>>;
        <<< "sustain_dur", p_sustain_dur >>>;
        <<< "sustain_level", p_sustain_level >>>;

        <<< "--freq------------" >>>;
        <<< "p_freq_base_midi", p_freq_base_midi >>>;    
        <<< "p_freq_limit_midi", p_freq_limit_midi >>>;
        <<< "p_freq_ramp", p_freq_ramp >>>;    
        <<< "p_freq_dramp", p_freq_dramp >>>;   

        <<< "--vibrato------------" >>>;
        <<< "p_vib_depth", p_vib_depth >>>;
        <<< "p_vib_freq", p_vib_freq >>>; 

        <<< "--arp------------" >>>;
        <<< "p_arp_mod_midi", p_arp_mod_midi >>>;      
        <<< "p_arp_time", p_arp_time >>>;      

        <<< "--pwm------------" >>>;
        <<< "p_pwm_depth", p_pwm_depth >>>;         
        <<< "p_pwm_freq", p_pwm_freq >>>;          

        <<< "--delay------------" >>>;
        <<< "p_feedback_gain", p_feedback_gain >>>; 
        <<< "p_delay_base_dur", p_delay_base_dur >>>;
        <<< "p_delay_mod_freq", p_delay_mod_freq >>>;
        <<< "p_delay_mod_depth", p_delay_mod_depth >>>;

        <<< "--filter------------" >>>;
        <<< "p_lpf_freq", p_lpf_freq >>>;    
        <<< "p_lpf_ramp", p_lpf_ramp >>>;      
        <<< "p_lpf_resonance", p_lpf_resonance >>>; 
        <<< "p_hpf_freq", p_hpf_freq >>>;    
        <<< "p_hpf_ramp", p_hpf_ramp >>>;    
    }

    fun void play()
    {   
        // printParams();
        _play(++_play_count);
    }

    fun void coin(float midi_start, float midi_end) { spork ~ pickupCoinShred(midi_start, midi_end - midi_start); }
    fun void laser() { spork ~ shootLaserShred(); }
    fun void explosion(dur d) { spork ~ explosionShred(d); }
    fun void powerup() { spork ~ powerupShred(); }
    fun void hit() { spork ~ hitShred(); }
    fun void jump() { spork ~ jumpShred(); }
    fun void blip() { spork ~ blipShred(); }

    fun void pickupCoinShred(float midi_freq, float midi_ival) {
        resetParams();

        // if (rnd(1)) CKFXR.WaveType_SAW => p_wave_type;
        // else CKFXR.WaveType_SQUARE => p_wave_type;
        CKFXR.WaveType_SQUARE => p_wave_type;

        // Math.random2f(60, 84) => p_freq_base_midi;
        midi_freq => p_freq_base_midi;

        frnd(0.05, 0.15)::second => p_sustain_dur;
        frnd(0.15, 0.3)::second => p_release_dur;
        frnd(.6, .9) => p_sustain_level;

        frnd(.6, 1.2) * p_sustain_dur => p_arp_time;

        // Math.random2f(2, 24) $ int => p_arp_mod_midi;
        midi_ival => p_arp_mod_midi;

        play();
    }

    fun void shootLaserShred() {
        resetParams();

        rnd(2) => p_wave_type;
        
        frnd(-120, -12) => p_freq_ramp;
        frnd(-80, -12) => p_freq_dramp;
        frnd(60, 127) => p_freq_base_midi;

        frnd(0.05, 0.2)::second => p_sustain_dur;
        frnd(0.05, 0.3)::second => p_release_dur;
        frnd(.6, .9) => p_sustain_level;

        play();
    }

    fun void explosionShred(dur d) {
        resetParams();

        CKFXR.WaveType_NOISE => p_wave_type;

        frnd(0.01, 0.1)::second => p_sustain_dur;
        d - p_sustain_dur => p_release_dur;
        // frnd(0.1, 0.7)::second => p_release_dur;

        if (rnd(1)) {
            frnd(5,65)::ms => p_delay_base_dur;
            frnd(0.1, 15) => p_delay_mod_freq;
            frnd(0.1, 0.5) => p_delay_mod_depth;
            frnd(0.1, 0.9) => p_feedback_gain;
        }

        frnd(2000, 10000) => p_lpf_freq;
        frnd(-48, 0) => p_lpf_ramp;
        frnd(.1, 4) => p_lpf_resonance;

        play();
    }

    fun void powerupShred() {
        resetParams ();

        if (rnd(1)) CKFXR.WaveType_SAW => p_wave_type;
        else CKFXR.WaveType_SQUARE => p_wave_type;

        frnd(50, 70) => p_freq_base_midi;

        if (rnd(1)) {
            frnd(10, 60) => p_freq_ramp;
        } else {
            frnd(2, 15) => p_freq_ramp;
            frnd(2, 15) => p_freq_dramp;
        }

        if (rnd(1)) {
            frnd(0.2) => p_vib_depth;
            frnd(7,20) => p_vib_freq;
        }

        frnd(100, 400)::ms => p_sustain_dur;
        frnd(100, 500)::ms => p_release_dur;

        play();
    }

    fun void hitShred() {
        resetParams();

        rnd(2) => p_wave_type;
        if (p_wave_type == CKFXR.WaveType_SIN) CKFXR.WaveType_NOISE => p_wave_type;

        if (p_wave_type == CKFXR.WaveType_SQUARE) {
            frnd(0.2) => p_pwm_depth;
            frnd(30) => p_pwm_freq;
        }

        frnd(20, 80) => p_freq_base_midi;
        frnd(-55, -5) => p_freq_ramp;

        frnd(10, 100)::ms => p_sustain_dur;
        frnd(100, 300)::ms => p_release_dur;

        if (rnd(1)) frnd(30, 3000) => p_hpf_freq;

        play();
    }

    fun void jumpShred() {
        resetParams();

        CKFXR.WaveType_SQUARE => p_wave_type;

        if (rnd(1)) {
            frnd(0.1, 0.6) => p_pwm_depth;
            frnd(2, 20) => p_pwm_freq;
        }

        frnd(40, 80) => p_freq_base_midi;
        frnd(12, 48) => p_freq_ramp;
        frnd(12, 48) => p_freq_dramp;

        frnd(100, 300)::ms => p_sustain_dur;
        frnd(100, 300)::ms => p_release_dur;

        if (rnd(1)) frnd(30, 3000) => p_hpf_freq;
        if (rnd(1)) frnd(p_hpf_freq, 18000) => p_lpf_freq;

        play();
    }

    fun void blipShred() {
        resetParams();

        rnd(1) => p_wave_type;
        if (p_wave_type == CKFXR.WaveType_SQUARE) {
            frnd(0.2) => p_pwm_depth;
            frnd(20) => p_pwm_freq;
        }

        frnd(36, 96) => p_freq_base_midi;

        frnd(50, 100)::ms => p_sustain_dur;
        frnd(10, 100)::ms => p_release_dur;

        frnd(100, 200) => p_hpf_freq;

        play();
    }


    // Internal -----------------------------------------------------------------
    // used to prevent overlapping play calls so that the synth can be 
    // retriggered before a previous shred ends. 
    // DO NOT MODIFY _play_count directly. Use play() instead.
    0 => int _play_count; 

    // map user params to synthesis 
    fun void _assign()
    {
        // reset lfo phase
        0 => lfo_vibrato.phase;
        0 => lfo_pwm.phase;

        // p_wave_type
        for (auto ugen : waveforms) ugen =< lpf;
        waveforms[p_wave_type] => lpf;

        // envelope
        adsr.set(p_attack_dur, 1::ms, p_sustain_level, p_release_dur);

        // frequency
        _setFreq(p_freq_base_midi);

        // bibrato
        lfo_vibrato.freq(p_vib_freq);
        lfo_vibrato.gain(p_vib_depth);

        // duty
        lfo_pwm.freq(p_pwm_freq);
        lfo_pwm.gain(p_pwm_depth);

        // delay effect
        delayfx.feedbackGain(p_feedback_gain);
        delayfx.delayBase(p_delay_base_dur);
        delayfx.delayModFreq(p_delay_mod_freq);
        delayfx.delayModDepth(p_delay_mod_depth);

        // Low-pass filter
        lpf.freq(p_lpf_freq);
        lpf.Q(p_lpf_resonance);

        // High-pass filter
        hpf.freq(p_hpf_freq);
    }

    fun void _play(int pc)
    {
        if (pc != _play_count) return;

        _assign();

        spork ~ _playArp(pc);
        spork ~_playMod(pc);

        adsr.keyOn();
        adsr.attackTime() + adsr.decayTime() + p_sustain_dur => now;

        if (pc != this._play_count) return;

        this.adsr.keyOff();
        this.adsr.releaseTime() => now;

        // wait for env to finish. if, after release, pc is the same, bump pc again. 
        // this means no other incoming play requests were made
        // and bumping pc again will stop any inifinite loop sporks
        if (pc == _play_count) _play_count++;
    }

    fun int _setFreq(float freq_midi)
    {
        // only set if it's above the cutoff
        if (freq_midi < p_freq_limit_midi) return false;

        Std.mtof(freq_midi) => float freq_hz;

        freq_hz => sin.freq;
        freq_hz => saw.freq;
        freq_hz => square.freq;
        return true;
    }

    fun void _playArp(int pc)
    {
        p_arp_time => now;
        if (pc != _play_count) return;

        // adjust frequency
        p_arp_mod_midi +=> p_freq_base_midi;
    }

    // frequency + vibrato mod + filter mod
    fun void _playMod(int pc)
    {
        now => time t;

        while (1::ms => now) {
            if (pc != _play_count) return;

            // dt
            (now - t) / second => float dt; // elapsed time in seconds
            now => t;

            // frequency modulation
            (dt * p_freq_ramp) +=> p_freq_base_midi;
            (dt * p_freq_dramp) +=> p_freq_ramp;

            // vibrato (remap from [-1, 1] to [1-depth, 1+depth] )
            (lfo_vibrato.last() + 1) * p_freq_base_midi => float freq_midi;

            // set frequency
            _setFreq(freq_midi);
            
            // lpf ramp
            Std.mtof((dt * p_lpf_ramp) + Std.ftom(lpf.freq())) => lpf.freq;

            // hpf ramp
            Std.mtof((dt * p_hpf_ramp) + Std.ftom(hpf.freq())) => hpf.freq;
        }
    }

    // return random float in range [0, range]
    fun float frnd(float range) { return range * Math.randomf(); }
    fun float frnd(float l, float h) { return Math.random2f(l, h); }

    // return random int in range [0, n]
    fun int rnd(int n) { return Math.random() % (n + 1); }
}


// shitty kick
public class Kick extends Chugraph {
    SinOsc kTri => 
    ADSR kTriEnv(
        5::ms,
        0::ms,
        1.0,
        100::ms
    ) => outlet;

    UI_Float atk(kTriEnv.attackTime()/second);
    UI_Float rel(kTriEnv.releaseTime()/second);
    UI_Float kick_len(.1);
    UI_Float freq_midi(Std.ftom(kTri.freq()));
    UI_Float end_note(0);
    UI_Float pitch_slew_time(.1);

    int generation;
    float start_midi, end_midi;
    time start;
    dur pitch_slew_dur;
    spork ~ pitch();

    fun void play() { spork ~ playShred(); }

    fun void playShred() {
        ++generation => int my_gen;
        Std.mtof(freq_midi.val()) => kTri.freq;
        freq_midi.val() => start_midi;
        end_note.val() => end_midi;
        now => start;
        pitch_slew_time.val()::second => pitch_slew_dur;

        kTriEnv.keyOn();
        kick_len.val()::second => now;
        if (my_gen == generation) kTriEnv.keyOff();
    }

    fun void pitch() {
        while (1) {
            1::ms => now;
            if (now > start + pitch_slew_dur) continue;
            (now - start) / pitch_slew_dur => float t;
            start_midi + t * (end_midi - start_midi) => float midi;
            Std.mtof(midi) => kTri.freq;
        }
    }

    fun void ui() {
        UI.slider("len", kick_len, 0, 1);
        UI.slider("midi", freq_midi, 12, 120);
        UI.slider("end note", end_note, 0, 60);
        UI.slider("slew time", pitch_slew_time, 0, .5);
        if (UI.slider("atk", atk, 0, .02)) atk.val()::second => kTriEnv.attackTime;
        if (UI.slider("rel", rel, 0, 1)) rel.val()::second => kTriEnv.releaseTime;
    }
}


if (1) {
    CKFXR ckfxr => dac;
    while (1) {
        GG.nextFrame() => now;

        // if (GWindow.keyDown(GWindow.Key_1)) ckfxr.coin();
        if (GWindow.keyDown(GWindow.Key_2)) ckfxr.laser();
        if (GWindow.keyDown(GWindow.Key_3)) ckfxr.explosion();
        if (GWindow.keyDown(GWindow.Key_4)) ckfxr.powerup();
        if (GWindow.keyDown(GWindow.Key_5)) ckfxr.hit();
        if (GWindow.keyDown(GWindow.Key_6)) ckfxr.jump();
        if (GWindow.keyDown(GWindow.Key_7)) ckfxr.blip();
    }
}
