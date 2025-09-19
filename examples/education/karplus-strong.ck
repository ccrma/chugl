//-----------------------------------------------------------------------------
// name: karplus-strong.ck
// desc: extends ks-chord.ck karplus strong comb filter bank (https://chuck.cs.princeton.edu/doc/examples/deep/ks-chord.ck)
//       maps keyboard input to change pitch.
//       press "space" to trigger noise bursts used for input.
//       spectrum visualized for fun.
//
// authors: Kunwoo Kim (kunwoo@ccrma.stanford.edu)
//          Madeline Huberth (mhuberth@ccrma.stanford.edu)
//          Ge Wang (ge@ccrma.stanford.edu)
// date: fall 2024
//-----------------------------------------------------------------------------

// -------- Karplus Strong Parameters --------- //
// number of voices
14 => int maxVoices;
// id (save midi notes here)
float id[maxVoices];
// start pitch
60 => int startPitch;
// counter for keeping track of midi notes
int counter;

// overall gain
Gain input;
0.5 => input.gain;
// reverb
NRev r;
0.05 => r.mix;
// Karplus Strong Algorithm Class
KeyPress keys[maxVoices];

// visual piano
piano pianoRoll --> GG.scene();
pianoRoll.sca(@(20, 20, 1));
pianoRoll.pos(@(-15, -15, -50));

// Background Color
@(1.0, 1.0, 1.0) * 0.05 => GG.scene().backgroundColor;

// Text for noise bursts
GText noiseText --> GG.scene(); noiseText.posX(0); noiseText.posY(-1.8); noiseText.sca(0.1);
"Press Space to hear noise burst input." => noiseText.text;

// Map keyboard input
fun void midiSynth()
{
    while (true)
    {   
        // keeps track of relative midi notes
        0 => counter;

        // Keyboard A = startpitch
        if (GWindow.key(GWindow.Key_A))
        {
            // store midi note into id
            startPitch + counter => id[counter];
            // change the piano key to SKYBLUE when pressed
            pianoRoll.key[counter].color(Color.SKYBLUE);
            // reset Audio (reset & play the note)
            keys[counter].resetAudio();
        }
        else
        {
            // if no key is pressed, change midi note to 0 (don't play anything)
            0 => id[counter];
            // reset the visual piano key color to white
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        // COPY AND PASTE FTW! Who cares about efficiency as long as it works?
        1 => counter;
        if (GWindow.key(GWindow.Key_W))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.BLACK);
        }
        
        2 => counter;
        if (GWindow.key(GWindow.Key_S))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        3 => counter;
        if (GWindow.key(GWindow.Key_E))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.BLACK);
        }
        
        4 => counter;
        if (GWindow.key(GWindow.Key_D))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        5 => counter;
        if (GWindow.key(GWindow.Key_F))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        6 => counter;
        if (GWindow.key(GWindow.Key_T))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter]; 
            pianoRoll.key[counter].color(Color.BLACK);   
        }
        
        7 => counter;
        if (GWindow.key(GWindow.Key_G))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        8 => counter;
        if (GWindow.key(GWindow.Key_Y))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];
            pianoRoll.key[counter].color(Color.BLACK);    
        }
        
        9 => counter;
        if (GWindow.key(GWindow.Key_H))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        10 => counter;
        if (GWindow.key(GWindow.Key_U))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];
            pianoRoll.key[counter].color(Color.BLACK);    
        }
        
        11 => counter;
        if (GWindow.key(GWindow.Key_J))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        12 => counter;
        if (GWindow.key(GWindow.Key_K))
        {
            startPitch + counter => id[counter];
            pianoRoll.key[counter].color(Color.SKYBLUE);
            keys[counter].resetAudio();
        }
        else
        {
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        if (GWindow.key(GWindow.Key_Space))
        {
            keys[13].playNoise();
        }
        
        for (int i; i < maxVoices; i++)
        {
            id[i] => keys[i].pitch;
            if (keys[i].pitch > 0)
            {
                spork ~ keys[i].playAudio();
            }
        }
        
        10::ms => now;
    }
}
spork ~ midiSynth();

// --------------- DRAW SPECTRUM --------------- //
// Note: This code is taken from sndpeek.ck

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
spectrum.posX(0);
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

// game loop
while( true )
{
    // sychronize graphics to system
    GG.nextFrame() => now;
    // map spectrum values for visualization
    map2spectrum(response, spec_positions);
    // draw spectrum
    spectrum.positions(spec_positions);
}

// ----------- CLASSES ------------ //
// single voice Karplus Strong chubgraph
class KS extends Chugraph
{
    // sample rate
    second / samp => float SRATE;
    
    // ugens!
    DelayA delay;
    OneZero lowpass;
    
    // noise, only for internal use
    Noise n => delay;
    // silence so it doesn't play
    0 => n.gain;
    
    // the feedback
    inlet => delay => lowpass => delay => outlet;
    // max delay
    1::second => delay.max;
    // set lowpass
    -1 => lowpass.zero;
    // set feedback attenuation
    .9 => lowpass.gain;

    // mostly for testing
    fun void play( float pitch, dur T )
    {
        tune( pitch ) => float length;
        // turn on noise
        1 => n.gain;
        // fill delay with length samples
        length::samp => now;
        // silence
        0 => n.gain;
        // let it play
        T-length::samp => now;
    }
    
    // tune the fundamental resonance
    fun float tune( float pitch )
    {
        // computes further pitch tuning for higher pitches
        pitch - 43 => float diff;
        0 => float adjust;
        if( diff > 0 ) diff * .0125 => adjust;
        // compute length
        computeDelay( Std.mtof(pitch+adjust) ) => float length;
        // set the delay
        length::samp => delay.delay;
        //return
        return length;
    }
    
    // set feedback attenuation
    fun float feedback( float att )
    {
        // sanity check
        if( att >= 1 || att < 0 )
        {
            <<< "set feedback value between 0 and 1 (non-inclusive)" >>>;
            return lowpass.gain();
        }
        
        // set it        
        att => lowpass.gain;
        // return
        return att;
    }
    
    // compute delay from frequency
    fun float computeDelay( float freq )
    {
        // compute delay length from srate and desired freq
        return SRATE / freq;
    }
}

// Class for visual piano on screen
class piano extends GGen
{
     // doubleWhite is a boolean to check if the next key is also white
     int doubleWhite;
     // offset between the keys
     int offset;
     // GPlane for key's visual & Text for keyboard to plane
     GPlane key[13];
     GText text[13];
     
     text[0].text("A");
     text[1].text("W");
     text[2].text("S");
     text[3].text("E");
     text[4].text("D");
     text[5].text("F");
     text[6].text("T");
     text[7].text("G");
     text[8].text("Y");
     text[9].text("H");
     text[10].text("U");
     text[11].text("J");
     text[12].text("K");
 
     // which keys are white?
     [0, 2, 4, 5, 7, 9, 11, 12] @=> int whiteKey[];
     
     int counter;
     for (auto s : key)
     {
         s --> this;
         text[counter] --> this;
         s.sca(@(0.2, 1, 1));
         
         1 => int blackWhite;
         
         for (int i; i < whiteKey.size(); i++)
         {
             if (counter == whiteKey[i])
             {
                 0 => blackWhite;
             }
         }
         
         // if black key
         if (blackWhite)
         {
             0 => doubleWhite;
             s.color(Color.BLACK);
             s.pos(@((counter + offset) * 0.108, 1.42, -5));
             s.sca(@(0.2, 0.6, 1));
             text[counter].pos(s.pos() + @(0, 0.2, 0.2));
             text[counter].sca(.1);
         }
         else
         {
             if (doubleWhite)
             {
                 offset++;
             }
             1 => doubleWhite;
             s.color(Color.WHITE);
             s.pos(@((counter + offset) * 0.108, 1.2, -5.1));
             s.scaY(1.1);
             text[counter].pos(s.pos() + @(0, -0.45, 0.5));
             text[counter].sca(0.1);
         }
         
         counter++;
     }  
}

// Class for playing keyboard stuff
class KeyPress
{
    Noise n => ADSR e => KS filter => r => input => dac;
    e.set(50::ms, 10::ms, 0.5, 50::ms);
    
    float pitch;
    float gain;
    int played;
    
    fun void playAudio()
    {
        if (played == 0)
        {
            1 => played;
            0.0005 => n.gain;
            filter.feedback(0.995);
            filter.tune(pitch);
            e.keyOn();
            50::ms => now;
            e.keyOff();
            50::ms => now;
        }
    }
    
    fun void playNoise()
    {
        0.005 => n.gain;
        e.keyOn();
        50::ms => now;
        e.keyOff();
        50::ms => now;
    }
    
    fun void resetAudio()
    {
        0 => played;
    }
}
