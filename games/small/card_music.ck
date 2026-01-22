public class CardMusic {
    1::second => dur release_time;

    Blit osc => ADSR osc_env => ADSR env => Gain rev;
    Blit osc2 => ADSR osc2_env => env;
    env.set(1::ms, 10::ms, .6, 100::ms);
    // .05 => rev.mix;
    1 => osc.gain;
    1 => osc2.gain;

    // base
    Chorus chorus => rev;
    chorus.baseDelay( 10::ms );
    chorus.modDepth( .2 );
    chorus.modFreq( .8 );
    chorus.mix( .2 );

    TriOsc saw => LPF lpf => ADSR bass_env(100::ms, 0::ms, 1, release_time) => chorus;
    TriOsc saw2 => lpf;
    .8 => saw.gain => saw2.gain;
    300 => lpf.freq;
    .3 => lpf.gain;

    Blit tenor_sc => ADSR tenor_env(100::ms, 0::ms, 1, release_time) => chorus;
    Blit alto_osc => ADSR alto_env(100::ms, 0::ms, 1, release_time) => chorus;
    Blit soprano_osc => ADSR soprano_env(100::ms, 0::ms, 1, release_time) => chorus;
    0 => tenor_sc.gain => alto_osc.gain => soprano_osc.gain;
    2 => tenor_sc.harmonics;
    2 => alto_osc.harmonics;
    2 => soprano_osc.harmonics;

    Blit shimmer => ADSR shimmer_switch => ADSR shimmer_env => Gain shimmer_gain => rev;
    shimmer_env.set(1::ms, 10::ms, .4, 10::ms);
    shimmer_gain => DelayL shimmer_delay => shimmer_gain; 
    .15 => shimmer_delay.gain; second / 7 => shimmer_delay.max => shimmer_delay.delay;
    .5 => shimmer.gain;


    [0, 2, 4, 7, 9, 12] @=> int notes[];
    0 => int osc2_mode;
    second / 10 => dur seq_tempo;
    true => int playing;

    48 => int base_freq;
    fun void playBass() {
        while (playing) {
            // 6
            Std.mtof(base_freq + 9) => saw.freq;
            Std.mtof(base_freq + 9 - 12) => saw2.freq;
            2::second => now;

            // 5
            Std.mtof(base_freq + 7) => saw.freq;
            Std.mtof(base_freq + 7 - 12) => saw2.freq;
            2::second => now;

            // 1
            Std.mtof(base_freq + 0) => saw.freq;
            Std.mtof(base_freq + 0 - 12) => saw2.freq;
            4::second => now;

            // randomize params at end of phrase
            (osc2_mode + 1) % 3 => osc2_mode;

            // if (maybe) {
            // second / 10 => seq_tempo;
            // } else {
            // second / 12 => seq_tempo;
            // }
        }
    } 

fun void playTenor() {
    .3 => tenor_sc.gain;
    while (playing) {
        // 6
        Std.mtof(base_freq + 12) => tenor_sc.freq;
        2::second => now;

        // 5
        Std.mtof(base_freq + 12 + 2) => tenor_sc.freq;
        2::second => now;

        // 1
        Std.mtof(base_freq + 12 + 4) => tenor_sc.freq;
        4::second => now;
    }
}

fun void playAlto() {
    .3 => alto_osc.gain;
    while (playing) {
        // 6
        Std.mtof(base_freq + 12 + 4) => alto_osc.freq;
        2::second => now;

        // 5
        Std.mtof(base_freq + 12 + 7) => alto_osc.freq;
        2::second => now;

        // 1
        Std.mtof(base_freq + 12 + 7) => alto_osc.freq;
        4::second => now;
    }
}

fun void playSoprano() {
    .4 => soprano_osc.gain;
    while (playing) {
        // 6
        Std.mtof(base_freq + 12 + 12) => soprano_osc.freq;
        2::second => now;

        // 5
        Std.mtof(base_freq + 12 + 11) => soprano_osc.freq;
        2::second => now;

        // 1
        Std.mtof(base_freq + 12 + 12) => soprano_osc.freq;
        4::second => now;
    }
}


    float notebank[0];
    int notebank_idx;
    fun float chooseNote() {
        if (notebank.size() == 0) return notes[Math.random2(0, notes.size()-1)] + 60;
        notebank[notebank_idx++ % notebank.size()] => float note;
        return note;
    }

chooseNote() => float midi_last;
fun void playMelody() {
    while (playing) {
        chooseNote() => float midi;
        Std.mtof(midi) => osc.freq;

        if      (osc2_mode == 0) Std.mtof(midi_last) => osc2.freq;
        else if (osc2_mode == 1) {
            if (maybe) Std.mtof(midi_last + 0 ) => osc2.freq;
            else       Std.mtof(midi_last + 12) => osc2.freq;
        } 
        else if (osc2_mode == 2) Std.mtof(midi_last + 12) => osc2.freq;

        if (maybe) midi => midi_last;
        else chooseNote() => midi_last;

        Math.random2( 1, 5 ) => osc.harmonics;
        Math.random2( 1, 5 ) => osc2.harmonics;
        env.keyOn();
        seq_tempo => now;
        env.keyOff();
        seq_tempo => now;
    }
}


fun void playShimmer() {
    .33 => shimmer.gain;
    0 => int idx;
    1 => int dir;
    while (playing) {

        (1 + .9 * Math.sin(now/(3::second))) * .20 => shimmer.gain; 


        notes[idx] + 84 => float midi_note;
        Std.mtof(midi_note) => shimmer.freq;

        Math.random2(4,5) => shimmer.harmonics;
        
        shimmer_env.keyOn();
        seq_tempo / 3 => now;
        shimmer_env.keyOff();
        seq_tempo / 3 => now;

        if (dir) {
            idx + 1 => idx;
            if (idx == notes.size() - 1)  1 - dir => dir;
        } else {
            idx - 1 => idx;
            if (idx == 0) 1 - dir => dir;
        }
    }
}

    true => playing;
    spork ~ playBass();
    spork ~ playTenor();
    spork ~ playAlto();
    spork ~ playSoprano();
    spork ~ playMelody();
    spork ~ playShimmer();

    // 8::second => now;
    // 8::second => now;
    // 8::second => now;
    // 8::second => now;
    // 8::second => now;    

    fun void allOff() {
        this.bass_env.keyOff();
        this.alto_env.keyOff();
        this.tenor_env.keyOff();
        this.soprano_env.keyOff();
        this.osc_env.keyOff();
        this.osc2_env.keyOff();
        this.shimmer_switch.keyOff();
    }

    // fun void mainOn(float target) {
    //     repeat (1000) {
    //         .005 * (target - rev.gain()) + rev.gain() => rev.gain;
    //         2::ms => now;
    //     }
    //     target => rev.gain;
    // }

    fun void mainOff(dur d) {
        rev.gain() => float g;
        now => time start;
        now + d => time later;
        while (now < later) {
            g - ((now - start) / d) * g => rev.gain;
            5::ms => now;
        } 
        0 => rev.gain;
    }


}

// CardMusic music;
// music.rev => dac;
// music.bass_env.keyOn();
// music.alto_env.keyOn();
// music.tenor_env.keyOn();
// music.soprano_env.keyOn();
// music.osc_env.keyOn();
// music.osc2_env.keyOn();
// music.shimmer_switch.keyOn();
// eon => now;