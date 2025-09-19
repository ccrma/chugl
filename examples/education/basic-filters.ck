//-----------------------------------------------------------------------------
// name: basic-filters.ck
// desc: basic visualization of lowpass / highpass / bandpass filters
//       type "1" on keyboard for sqrwav demo
//       type "2" on keyboard for sndbuf demo
//       you can use your own sndbuf.
// 
// author: Kunwoo Kim (kunwoo@ccrma.stanford.edu)
//   date: fall 2024
//-----------------------------------------------------------------------------

// note: If you wnat to test this code with a wavfile,
// search '[UNCOMMENT]' and make adjustments

// mode - sqrwav or sndbuf
0 => int mode;
// Low pass filter
LPF lpf;
// High pass filter
HPF hpf;
// Overall gain
Gain input;

// initialize and set filter parameters
// low pass filter
20000 => float lpFreq => lpf.freq;
1 => float lpQ => lpf.Q;
0.5 => lpf.gain;
// high pass filter
50 => float hpFreq => hpf.freq;
1 => float hpQ => hpf.Q;
0.5 => hpf.gain;

// keep updating filters
fun void filters()
{
    while (true)
    {
        lpFreq => lpf.freq;
        hpFreq => hpf.freq;
        lpQ => lpf.Q;
        hpQ => hpf.Q;
        
        10::ms => now;
    }
}
spork ~filters();

// ---------SNDBUF----------//
// [UNCOMMENT] Uncomment below and update the filepath if you wish to test with your wavefile.
/*

// update filepath here!
me.dir() + "samples/elemental_short.wav" => string filepath;
SndBuf buffy => lpf => hpf => input => dac;

fun void playBuf()
{
    filepath => buffy.read;
    if( buffy.samples() == 0)
        return;
    
    while (true)
    {
        0 => buffy.pos;
        
        now + buffy.length() => time later;
        
        while (now < later)
        {
            if (mode == 1)
            {
                buffy.gain(0.8);
            }
            else if (mode == 0)
            {
                buffy.gain(0);
            }
            10::ms => now;
        }
    }
}
spork ~ playBuf();
*/

// ---------SQROSC----------//
// initialize SqrOsc and send it to dac.
SqrOsc foo => ADSR e => lpf => hpf => NRev r => input => dac;
// reverb
0.2 => r.mix;
// duration of each frequency
50::ms => dur d;
// set gain
.5 => foo.gain;
// pentatonic notes
[0, 2, 4, 5, 7, 9, 11] @=> int notes[];

fun void playSqrOsc()
{
    while (true)
    {
        // 50% of the time, play double length
        if (Math.random2f(0, 1) > 0.5)
        {
            d * 2 => now;
        }
        
        // set A, D, S, and R
        e.set( Math.random2(10, 70)::ms, 8::ms, .5, d );
        
        // set frequency
        Std.mtof(notes[Math.random2(0, notes.size() - 1)] + Math.random2(0, 2) * 12 + 36) => foo.freq;
        
        // based on the mode, play or not play the sqrosc
        if (mode == 1)
        {
            foo.gain(0);
        }
        else if (mode == 0)
        {
            foo.gain(0.3);
        }
        
        e.keyOn();
        d => now;
        e.keyOff();
        e.releaseTime() => now;
        d => now;
    }
}
spork ~ playSqrOsc();

// ---------UI----------//
// set UI parameters
UI_Float ui_lpfFreq(20000);
UI_Float ui_lpfQ(1);
UI_Float ui_hpfFreq(50.0);
UI_Float ui_hpfQ(1);

// create ChuGL UI
fun void filter_UI()
{
        while (true) {
        GG.nextFrame() => now;
        UI.setNextWindowSize(@(400, 800), UI_Cond.Once);
        if (UI.begin("Basic Filters (Type 1, 2, 3 to change lpf, hpf, bpf)", null, 0)) {
            if (UI.slider("Low-pass filter cutoff frequency", ui_lpfFreq, 50.0, 20000.0)) {
                ui_lpfFreq.val() => lpFreq;
            }
            if (UI.slider("Low-pass filter resonance", ui_lpfQ, 0.0, 5.0)) {
                ui_lpfQ.val() => lpQ;
            }
            if (UI.slider("High-pass filter cutoff frequency", ui_hpfFreq, 50.0, 20000.0)) {
                ui_hpfFreq.val() => hpFreq;
            }
            if (UI.slider("High-pass filter resonance", ui_hpfQ, 0.0, 5.0)) {
                ui_hpfQ.val() => hpQ;
            }
        }
        UI.end();
    }
}
spork ~ filter_UI();

// ---------KEYBOARD INPUT----------//
// listen to keyboard for mode changes
fun void modeChange()
{
    while (true)
    {
        if (GWindow.key(GWindow.Key_1))
        {
            0 => mode;
        }
        else if (GWindow.key(GWindow.Key_2))
        {
            1 => mode;
        }
        
        10::ms => now;
    }    
}
spork ~ modeChange();

// --------------- DRAW SPECTRUM --------------- //
// note: this code is taken from sndpeek.ck

// spectrum setup
// window size
2048 => int WINDOW_SIZE;
// y position of waveform
1.5 => float WAVEFORM_Y;
5 => float DISPLAY_WIDTH;
0.5 => float freqRange;
0.05 => float SLEW;

// accumulate samples from mic
input => Flip accum => blackhole;
// take the FFT
input => PoleZero dcbloke => FFT fft => blackhole;
// set DC blocker
.95 => dcbloke.blockZero;
// set size of flip
WINDOW_SIZE => accum.size;
// set window type and size
Windowing.hann(WINDOW_SIZE) => fft.window;
// set FFT size (will automatically zero pad)
WINDOW_SIZE*2 => fft.size;
// get a reference for our window for visual tapering of the waveform
Windowing.hann(WINDOW_SIZE) @=> float window[];

// sample array
float samples[0];
// FFT response
complex response[0];
// a vector to hold positions
vec2 spec_positions[WINDOW_SIZE];
vec2 prev_positions[WINDOW_SIZE];

// spectrum renderer
GLines spectrum --> GG.scene(); spectrum.width(.01);
// translate down
spectrum.posY(-WAVEFORM_Y);
spectrum.posX(0.5);
// color0
spectrum.color( @(.4, 1, .4) );

GWindow.fullscreen();

// map FFT output to 3D positions
fun void map2spectrum( complex in[], vec2 out[] )
{
    if( in.size() != out.size() )
    {
        <<< "size mismatch in map2spectrum()", "" >>>;
        return;
    }
    
    // mapping to xyz coordinate
    int i;
    DISPLAY_WIDTH => float width;
    
    (freqRange * WINDOW_SIZE) $ int => int numFreq;
    
    for( int s; s < numFreq; s++ )
    {
        // space evenly in X
        -width/2 + width/numFreq*s => out[s].x;
        // map frequency bin magnitide in Y        
        48 * Math.sqrt( (in[s]$polar).mag ) => out[s].y;
        // increment
        prev_positions[s] + (out[s] - prev_positions[s]) * SLEW => out[s];
        out[s] => prev_positions[s];
    }
}

// set waveform label
GText text_spectrum;
text_spectrum.text("Spectrum");
text_spectrum.sca(0.1);
text_spectrum.posY(-1.7);
text_spectrum.posX(0.4);
text_spectrum.color(Color.WHITE);
text_spectrum --> GG.scene();

// do audio stuff
fun void doAudio()
{
    while( true )
    {
        // upchuck to process accum
        accum.upchuck();
        // get the last window size samples (waveform)
        accum.output( samples );
        // upchuck to take FFT, get magnitude reposne
        fft.upchuck();
        // get spectrum (as complex values)
        fft.spectrum( response );
        // jump by samples
        WINDOW_SIZE::samp/4 => now;
    }
}
spork ~ doAudio();

// main game loop
while (true)
{
    // synchronize with graphics
    GG.nextFrame() => now;
    // map spectrum values for visualization
    map2spectrum(response, spec_positions);
    // draw as lines
    spectrum.positions(spec_positions);
}
