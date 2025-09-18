//-----------------------------------------------------------------------------
// name: fm-synthesis.ck
// desc: basic visualization of FM Synthesis
//       uses Madeline Huberth's FMFS code from 2015
// 
// author: Kunwoo Kim (kunwoo@ccrma.stanford.edu)
//   date: Fall 2024
//-----------------------------------------------------------------------------

// -------- FM Synthesis Parameters -------- //
// array to hold midi pitches (key numbers) - these will be converted into the carrier frequencies
float signalFrequencies[100];
// carrier frequency amplitdue
float cAmp;
// 2d array, second dimension will hold an array of float values for ADSR
float cADSR[0];
// modulator ratio
float mRatio;
// modulator index
float mIndex;
// 2d array, second dimension will hold an array of float values for ADSR
float mADSR[0];
// signal frequency counter
int signalFreqCounter;

// initialize values
[0.01,0.2,0.8,0.1] @=> cADSR;
2 => mRatio;
2 => mIndex;
[.01,.2,1.0,.1] @=> mADSR;
float cFreq;

// variable for which pitch is next
0 => int p;
// variable for which instrument is next                         
0 => int i;
0 => int counter;

// overall gain
Gain input;

// custom class for FM synthesis
FMFS melFM;
melFM.out => input => dac;

// ---------- DRAW WAVEFORM ---------- //
// line used for oscillator waveform
8 => int DISPLAY_WIDTH;
GWindow.fullscreen();

// custom class for drawing oscillators
// draw carrier frequency
drawOscillator drawCar;
drawCar.init(1.7, 3.5, -7);
// draw modulating frequency
drawOscillator drawMod;
drawMod.init(1.7, 1, -7);
// draw signal
drawOscillatorCumulative drawSig;
drawSig.init(1.7, -1.5, -7);

// ---------- DRAW SPECTRUM ---------- //
// Note: This code is mostly taken from sndpeek.ck

// window size
512 => int WINDOW_SIZE;
// y position of waveform
2 => float WAVEFORM_Y;
// y position of spectrum
-2.5 => float SPECTRUM_Y;
1.0 => float freqRange;
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
vec2 positions[WINDOW_SIZE];
vec2 prev_positions[WINDOW_SIZE];

// spectrum renderer
GLines spectrum --> GG.scene(); spectrum.width(.01);
// translate down
spectrum.posY(-WAVEFORM_Y);
spectrum.posX(1.1);
// color0
spectrum.color( @(.4, 1, .4) );

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
    DISPLAY_WIDTH / 2.3 => float width;
    
    (freqRange * WINDOW_SIZE) $ int => int numFreq;
    
    for( int s; s < numFreq; s++ )
    {
        // space evenly in X
        -width/2 + width/numFreq*s => out[s].x;
        // map frequency bin magnitide in Y        
        12 * Math.sqrt( (in[s]$polar).mag ) => out[s].y;
        // increment
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
        WINDOW_SIZE::samp/2 => now;
    }
}
spork ~ doAudio();

// This function plays audio in random frequencies;
fun void doAudio2()
{
    // infinite loop
    while (true) {
        Std.mtof(Math.random2(60, 72)) => cFreq;
        spork ~melFM.playFM(500::ms, cFreq, cADSR, mRatio, mIndex, mADSR);
        // increment note and instrument
        p++;
        i++;
        
        // advance time by interval and calculate the next time interval
        500::ms => now;
        counter++;
    }
}
spork ~doAudio2();

// ----------- UI ------------ //
// UI for playing around with the code
UI_Float ui_carrier(mRatio);
UI_Float ui_modulation(mIndex);

fun void FM_synthesis_UI()
{
    while (true) {
        GG.nextFrame() => now;
        UI.setNextWindowSize(@(400, 200), UI_Cond.Once);
        if (UI.begin("FM Synthesis", null, 0)) {
            if (UI.slider("Ratio", ui_carrier, 0.0, 50)) {
                ui_carrier.val() => mRatio;
            }
            if (UI.slider("Index", ui_modulation, 0.0, 50)) {
                ui_modulation.val() => mIndex;
            }
        }
        UI.end();
    }
}
spork ~ FM_synthesis_UI();

// --------- TEXT & LABELS -------- //
GText text_car --> GG.scene(); text_car.sca(0.1); text_car.text("Carrier"); text_car.color(Color.WHITE);
GText text_mod --> GG.scene(); text_mod.sca(0.1); text_mod.text("Modulating"); text_mod.color(Color.WHITE);
GText text_cumul --> GG.scene(); text_cumul.sca(0.1); text_cumul.text("Actual"); text_cumul.color(Color.WHITE);
GText text_spec --> GG.scene(); text_spec.sca(0.1); text_spec.text("Spectrum"); text_spec.color(Color.WHITE);

text_car.posX(-1.2); text_car.posY(1.4);
text_mod.posX(-1.2); text_mod.posY(0.4);
text_cumul.posX(-1.2); text_cumul.posY(-0.6);
text_spec.posX(-1.2); text_spec.posY(-1.6);

// main game loop
while (true)
{
    // synchronize graphics to system
    GG.nextFrame() => now;
    // Draw Carrier Signal
    drawCar.draw(cFreq);
    // Draw Modulating Signal
    drawMod.draw(melFM.mod.freq());
    // Draw Actual Signal
    drawSig.draw(melFM.car.freq());
    // map to spectrum display
    map2spectrum( response, positions );
    // set the mesh position
    spectrum.positions( positions );
}

// --------------------------CLASSES--------------------------
// Custom class for drawing oscillators
class drawOscillator
{
    // segments of line
    1000 => int numSegments;
    // line positions
    vec2 positions[numSegments];
    
    // Initialize Line
    GLines oscillator --> GG.scene(); oscillator.width(0.01);

    // Initialize positions
    fun void init(float x, float y, float z)
    {
        x => oscillator.posX;
        y => oscillator.posY;
        z => oscillator.posZ;
    }
    
    // Draw waveform from amplitude of each segment
    fun void draw(float freq)
    {   
        float add_Gain[numSegments];
        float maxValue;
        for (int i; i < numSegments; i++)
        {
            -DISPLAY_WIDTH / 2.5 + 1.0 * DISPLAY_WIDTH/numSegments * i => positions[i].x;
            Math.sin(2 * Math.PI * freq / 100.0 * i / numSegments) * 2.0 +=> add_Gain[i];
            if (add_Gain[i] > maxValue)
                add_Gain[i] => maxValue;
        }
        
        for (int i; i < numSegments; i++)
        {
            // normalize value if necessary
            if (maxValue > 1)
                Math.remap(add_Gain[i], -maxValue, maxValue, -1, 1) => positions[i].y;
            else
                add_Gain[i] => positions[i].y;
        }

        oscillator.positions(positions);
    }
}

// This class draws cumulative waveform (similar to drawOscillator)
class drawOscillatorCumulative
{
    100 => int numSegments;
    vec2 positions[numSegments];
    
    GLines oscillator --> GG.scene(); oscillator.width(0.01);
    fun void init(float x, float y, float z)
    {
        x => oscillator.posX;
        y => oscillator.posY;
        z => oscillator.posZ;
    }
    
    fun void draw(float freq)
    {   
        float add_Gain[numSegments];
        float maxValue;
        for (int i; i < numSegments; i++)
        {
            -DISPLAY_WIDTH / 2.5 + 1.0 * DISPLAY_WIDTH/numSegments * i => positions[i].x;
            Math.sin(2 * Math.PI * signalFrequencies[i] / 100.0 * i / numSegments) * 2.0 +=> add_Gain[i];
            if (add_Gain[i] > maxValue)
                add_Gain[i] => maxValue;
        }
        
        for (int i; i < numSegments; i++)
        {
            // normalize value if necessary
            if (maxValue > 1)
                Math.remap(add_Gain[i], -maxValue, maxValue, -1, 1) => positions[i].y;
            else
                add_Gain[i] => positions[i].y;
        }

        oscillator.positions(positions);
    }
}

// -------------------------------------------------------------
// @class FMFS
// fm implementation from scratch with envelopes
// @author 2015 Madeline Huberth, 2016 version by CC
class FMFS
{ // two typical uses of the ADSR envelope unit generator...
    Step unity => ADSR envM => blackhole; //...as a separate signal
    SinOsc mod => blackhole;
    <<< unity.next() >>>; // default 1
    SinOsc car => ADSR envC => JCRev r => Gain out;  //...as an inline modifier of a signal
    car.gain(0.2);
    float freq, index, ratio; // the parameters for our FM patch
    0.1 => r.mix;
    0.3 => r.gain;
    
    fun void fm() // this patch is where the work is
    {
        while (true)
        {
            envM.last() * index => float currentIndex; // time-varying index
            mod.gain( freq * currentIndex );    // modulator gain (index depends on frequency)
            mod.freq( freq * ratio );           // modulator frequency (a ratio of frequency) 
            car.freq( freq + mod.last() );      // frequency + modulator signal = FM 
            
            /*
            envM.last() * index => float currentIndex;
            mod.freq(freq / ratio);
            mod.gain(currentIndex);
            car.freq(freq);
            car.phase(mod.last());
            */

            car.freq() => signalFrequencies[counter % 100];
            1::samp => now;
            counter++;
            if (counter > 100)
            {
                0 => counter;
            }
        }
    }
    spork ~fm(); // run the FM patch
    
    // function to play a note on our FM patch
    fun void playFM( dur length, float pitch, float cADSR[], float mRatio, float mGain, float mADSR[] ) 
    {
        // set patch values
        pitch => freq;
        mRatio => ratio;
        mGain => index;
        // run the envelopes
        spork ~ playEnv( envC, length, cADSR );
        spork ~ playEnv( envM, length, mADSR );
        length => now; // wait until the note is done
    }
    
    fun void playEnv( ADSR env, dur length, float adsrValues[] )
    {
        // set values for ADSR envelope depending on length
        length * adsrValues[0] => dur A;
        length * adsrValues[1] => dur D;
        adsrValues[2] => float S;
        length * adsrValues[3] => dur R;
        
        // set up ADSR envelope for this note
        env.set( A, D, S, R );
        // start envelope (attack is first segment)
        env.keyOn();
        // wait through A+D+S, before R
        length-env.releaseTime() => now;
        // trigger release segment
        env.keyOff();
        // wait for release to finish
        env.releaseTime() => now;
    }
    
} // end of class FMFS
