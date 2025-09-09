//-----------------------------------------------------------------------------
// name: lissajous.ck
// desc: Lissajous visualizer. Oscilloscope music player.
//
// To play oscilloscope music, download the audio file at:
// https://ccrma.stanford.edu/~azaday/music/khrang.wav
// and place in the same directory as this file.
// Then press <space> or the "Play oscilloscope music" button.
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//   date: Fall 2024
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// setting up input
GG.bloom( true );
// FYI this sets up bloom by modifying the default rendergraph
//   default: GG.rootPass() --> GG.scenePass() --> GG.outputPass()
//  modified: insert GG.BloomPass() between GG.scenePass() and GG.outputPass()
// FYI this also performs the necessary connections for passing textures
//   including GG.bloomPass().input(GG.renderPass().colorOutput());
//             GG.outputPass().input(GG.bloomPass().colorOutput());
//-----------------------------------------------------------------------------
// sets bloom intensity
GG.bloomPass().intensity(1.0);

// set up camera mode
GG.camera().orthographic();
// (orthographic) control height of view box (effectively scales)
GG.camera().viewSize(4);
// (perspective) camera position in Z
// GG.camera().posZ(30.0);

// unit analyzer setup
1024 => int WINDOW_SIZE;
// one way to audio samples, window by window
dac.chan(0) => Flip accum_r => blackhole;
dac.chan(1) => Flip accum_l => blackhole;
WINDOW_SIZE => accum_l.size;
WINDOW_SIZE => accum_r.size;
float left_waveform[WINDOW_SIZE];
float right_waveform[WINDOW_SIZE];
// function to fill left and right audio buffers
fun void audio()
{
    while (true) {
        WINDOW_SIZE::samp => now;

        accum_l.upchuck();
        accum_l.output(left_waveform);

        accum_r.upchuck();
        accum_r.output(right_waveform);
    }
} spork ~ audio();

// ugen setup
SinOsc left_osc => dac.chan(0);
SinOsc right_osc => dac.chan(1);
261 => left_osc.freq; 326 => right_osc.freq;
.1 => left_osc.gain => right_osc.gain;

// for playback from file
SndBuf2 buf => blackhole;
me.dir() + "./khrang.wav" => buf.read;
0 => buf.rate;
.5 => buf.gain;

// scenegraph setup
GLines lissajous --> GG.scene();
// line width
lissajous.width(.05);
// line color
lissajous.color(@(1./32., 1.0, 1./32.));
vec2 positions[WINDOW_SIZE];

// UI variables to control left and right frequencies
UI_Float freq_l(left_osc.freq());
UI_Float freq_r(right_osc.freq());
// UI variable for toggling glow
UI_Bool glow(true);

// rende rloop
while (true)
{
    // synchronize
    GG.nextFrame() => now;

    // update lines with audio data
    for (int i; i < WINDOW_SIZE; i++) {
        10 * @(left_waveform[i], right_waveform[i]) => positions[i];
        lissajous.positions(positions);
    }

    // begin UI
    if (UI.begin("Lissajous"))
    {
        if (UI.drag("Left Frequency", freq_l)) left_osc.freq(freq_l.val());
        if (UI.drag("Right Frequency", freq_r)) right_osc.freq(freq_r.val());
        if (UI.button("Sync phase")) { left_osc.phase(0); right_osc.phase(0); }

        // play file
        if (UI.button("Play oscilloscope music") || GWindow.keyDown(GWindow.KEY_SPACE)) {
            // disconnect the oscillators
            left_osc =< dac.chan(0);
            right_osc =< dac.chan(1);

            // connect and start sndbuf
            if (buf.pos() == 0) {
                // set to specific i
                GG.camera().viewSize(10.0);
                1.0 => buf.rate;
                buf => dac;
            }
        }

        // check bloom toggle
        if (UI.checkbox("Glow", glow)) GG.bloom(glow.val());
    }
    // end UI
    UI.end();
}
