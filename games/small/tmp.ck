class Kick extends Chugraph {
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


// unit test
if (1) {
    Kick kick => dac;

    while (1) {
        GG.nextFrame() => now;
        kick.ui();
        if (GWindow.keyDown(GWindow.KEY_SPACE)) {
            kick.play();
        }
    }

}
