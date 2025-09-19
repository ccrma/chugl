//-----------------------------------------------------------------------------
// name: additive.ck
// desc: basic additive synthesis visualization;
//       9 harmonics with fundamental frequency
// 
// author: Kunwoo Kim (kunwoo@ccrma.stanford.edu)
//   date: Fall 2024
//-----------------------------------------------------------------------------

// ----------- AUDIO PARAMETERS ---------- //
// note: If you want to add more harmonics, you have to write more UIs for it. //

// total number of harmonics + fundamental_frequency.
10 => int numHarmonics;
// sine oscillator for each harmonic + fundamental frequency.
SinOsc sin[numHarmonics];
// store fundamental frequency in a variable
float fundamental;

// envelope + reverb for each sine wave
Envelope e;
NRev r;
0.05 => r.gain;
0.05 => r.mix;

// overall gain
Gain input;
0.1 => input.gain;
// individual gain
float gain[numHarmonics];
// minimum gain
0 => float minGain;
// maximum gain
0.025 => float maxGain;

// connect SinOscs into dac
for (int i; i < numHarmonics; i++)
{
    sin[i] => input => e => r => dac;
    0 => sin[i].gain;
}

// ----------- UI ---------- //
// note: UI for playing around with the code
UI_Float as_frequencies[numHarmonics];

// give name to each slider
string slider_str[numHarmonics];
"Fundamental Frequency" => slider_str[0];
for (1 => int i; i < numHarmonics; i++)
{ "Harmonics " + Std.itoa(i) => slider_str[i]; }

// create UIs
fun void additive_synthesis_UI()
{
    // a render loop
    while( true )
    {
        // synchronize!
        GG.nextFrame() => now;
        // begin UI
        if( UI.begin("Additive Synthesis", null, 0) )
        {
            // listen to values
            for( int i; i < numHarmonics; i++ )
            {
                if( UI.slider(slider_str[i], as_frequencies[i],
                    minGain, maxGain) )
                {
                    // store changes into parameters
                    as_frequencies[i].val() => gain[i];
                }
            }
        }
        // end UI
        UI.end();
    }
}
spork ~ additive_synthesis_UI();

// ------------ AUDIO ---------- //
// diatonic notes
[0, 2, 4, 5, 7, 9, 11, 12] @=> int notes[];

// play audio based on the gains set,
// and assign random fundamental frequency.
fun void playAudio()
{
    while (true)
    {
        // envelope
        100::ms => e.duration;
        
        // select a new fundamental frequency
        if (Math.random2f(0, 1) > 0.0)
            Std.mtof(notes[Math.random2(0, notes.size()-1)] + 12 * Math.random2(3, 4)) => fundamental;
        
        // set frequency and gain
        for (int i; i < numHarmonics; i++)
        {
            fundamental * (i + 1) => sin[i].freq;
            gain[i] => sin[i].gain;
        }

        // play
        e.keyOn();
        e.duration() => now;
        e.keyOff();
        e.duration() => now;
    }
}
spork ~ playAudio();

// ----------- GRAPHICS ----------- //
// planes used for visualizing frequency magnitude
GPlane g_harmonics[numHarmonics];
// parent object for gPlanes
GGen harmonics;
// display width of the total frequency magnitudes
4 => int DISPLAY_WIDTH;
// individual width of each frequency magnitude
0.1 => float width;
// text Labels
GText text_harmonics;
GText text_waveform;
GText text_spectrum;

// window title
GWindow.title( "basic additive synthesis" );
// uncomment to fullscreen
GWindow.fullscreen();

// initialize planes
for (int i; i < numHarmonics; i++)
{
    g_harmonics[i] --> harmonics;
    -DISPLAY_WIDTH/2 * 0.5 + 1.0 * DISPLAY_WIDTH / numHarmonics * i => g_harmonics[i].posX;
    width => g_harmonics[i].scaX;
}

// set harmonics label
text_harmonics.text("Fundamental Frequency and Harmonics");
text_harmonics.sca(0.1);
text_harmonics.posY(1.2);
text_harmonics.color(Color.WHITE);
text_harmonics --> harmonics;

// set harmonics (parent object) to scene
harmonics --> GG.scene(); harmonics.posY(0.6);

// called from main loop - update magnitude changes accordingly
fun void mapSinOsc()
{
    for (int i; i < numHarmonics; i++)
    {
        0.01 + sin[i].gain() * 40 => g_harmonics[i].scaY;
        sin[i].gain() * 20 => g_harmonics[i].posY;
    }
}

// ---------- DRAW WAVEFORM ---------- //
// line used for oscillator waveform
GLines lines --> GG.scene(); lines.width(0.01);
250 => int numSegments;
-0.6 => lines.posY;
-5 => lines.posZ;
vec2 positions[numSegments];
Color.WHITE => lines.color;

// this functions draws the oscillator
fun void drawOscillator()
{
    // array to sum up all the signal amplitude from each harmonic
    float add_Gain[numSegments];
    // used for normalization
    float maxValue;
    // update add_Gain array
    for (int i; i < numSegments; i++)
    {
        // set x pos
        -DISPLAY_WIDTH / 2 + 2.0 * DISPLAY_WIDTH/numSegments * i => positions[i].x;
        // add up all signal amplitudes from each harmonic
        for (int j; j < numHarmonics; j++)
        {
            Math.sin(Math.PI * 4.0 * i / numSegments * (j + 1)) * gain[j] * 10.0 +=> add_Gain[i];
        }
        // update maxValue if necessary
        if (add_Gain[i] > maxValue)
            add_Gain[i] => maxValue;
    }
    
    // normalize value if necessary
    for (int i; i < numSegments; i++)
    {
        if (maxValue > 1)
            Math.remap(add_Gain[i], -maxValue, maxValue, -1, 1) => positions[i].y;
        else
            add_Gain[i] => positions[i].y;
    }

    // update line position
    lines.positions(positions);
}

// set waveform label
text_waveform.text("Waveform");
text_waveform.sca(0.1);
text_waveform.posY(0.35);
text_waveform.posX(-0.8);
text_waveform.color(Color.WHITE);
text_waveform --> GG.scene();

// --------------- DRAW SPECTRUM --------------- //
// note: this code is mostly taken from sndpeek.ck

// spectrum setup
// window size
2048 => int WINDOW_SIZE;
// y position of waveform
1.7 => float WAVEFORM_Y;
0.12 => float freqRange;
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
// vector to hold positions
vec2 spec_positions[WINDOW_SIZE];
vec2 prev_positions[WINDOW_SIZE];

// spectrum renderer
GLines spectrum --> GG.scene(); spectrum.width(.01);
// translate down
spectrum.posY(-WAVEFORM_Y);
spectrum.posX(1);
// color0
spectrum.color( @(.4, 1, .4) );

// set waveform label
text_spectrum.text("Spectrum");
text_spectrum.sca(0.1);
text_spectrum.posY(-0.9);
text_spectrum.posX(-0.8);
text_spectrum.color(Color.WHITE);
text_spectrum --> GG.scene();

// map FFT output to 3D positions
fun void map2spectrum( complex in[], vec2 out[] )
{
    if( in.size() != out.size() )
    {
        <<< "size mismatch in map2spectrum()", "" >>>;
        return;
    }
    
    // Mapping to xyz coordinate
    int i;
    DISPLAY_WIDTH => float width;
    
    (freqRange * WINDOW_SIZE) $ int => int numFreq;
    
    for( int s; s < numFreq; s++ )
    {
        // Space evenly in X
        -width/2 + width/numFreq*s => out[s].x;
        // Map frequency bin magnitide in Y        
        48 * Math.sqrt( (in[s]$polar).mag ) => out[s].y;
        // Increment
        prev_positions[s] + (out[s] - prev_positions[s]) * SLEW => out[s];
        out[s] => prev_positions[s];
    }
}

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
while( true )
{
    // synchronize graphics
    GG.nextFrame() => now;
    // map oscillator values for visualization
    mapSinOsc();
    // draw oscillator
    drawOscillator();
    // map spectrum values for visualization
    map2spectrum(response, spec_positions);
    // draw spectrum
    spectrum.positions(spec_positions);
}
