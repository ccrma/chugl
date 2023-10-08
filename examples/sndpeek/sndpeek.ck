//-----------------------------------------------------------------------------
// name: sndpeek.ck
// desc: sndpeek in ChuGL!
// 
// author: Ge Wang (https://ccrma.stanford.edu/~ge/)
//         Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// window size
1024 => int WINDOW_SIZE;
// y position of waveform
2 => float WAVEFORM_Y;
// y position of spectrum
-2.5 => float SPECTRUM_Y;
// width of waveform and spectrum display
10 => float DISPLAY_WIDTH;
// waterfall depth
128 => int WATERFALL_DEPTH;

// uncomment to fullscreen
GG.fullscreen();
// put camera on a dolly
GG.camera() --> GGen dolly --> GG.scene();
// position
GG.camera().posZ( 10 );
// set clipping plane
GG.camera().clip( .05, WATERFALL_DEPTH * 10 );
// set bg color
GG.scene().backgroundColor( @(0,0,0) );

// waveform renderer
GLines waveform --> GG.scene(); waveform.mat().lineWidth(1.0);
// translate up
waveform.posY( WAVEFORM_Y );
// color0
waveform.mat().color( @(.4, .4, 1)*1.5 );

// make a waterfall
Waterfall waterfall --> GG.scene();
// translate down
waterfall.posY( SPECTRUM_Y );

// which input?
adc => Gain input;
// SinOsc sine => Gain input => dac; .15 => sine.gain;
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
vec3 vertices[WINDOW_SIZE];
float positions[WINDOW_SIZE*3];

// custom GGen to render waterfall
class Waterfall extends GGen
{
    // waterfall playhead
    0 => int playhead;
    // lines
    GLines wfl[WATERFALL_DEPTH];
    // color
    @(.4, 1, .4) => vec3 color;

    // iterate over line GGens
    for( GLines w : wfl )
    {
        // aww yea, connect as a child of this GGen
        w --> this;
        // color
        w.mat().color( @(.4, 1, .4) );
    }

    // copy
    fun void latest( float positions[] )
    {
        // set into
        positions => wfl[playhead].geo().positions;
        // advance playhead
        playhead++;
        // wrap it
        WATERFALL_DEPTH %=> playhead;
    }

    // update
    fun void update( float dt )
    {
        // position
        playhead => int pos;
        // for color
        WATERFALL_DEPTH/2.5 => float thresh;
        // depth
        WATERFALL_DEPTH - thresh => float fadeChunk;
        // so good
        for( int i; i < wfl.size(); i++ )
        {
            // start with playhead-1 and go backwards
            pos--; if( pos < 0 ) WATERFALL_DEPTH-1 => pos;
            // offset Z
            wfl[pos].posZ( -i );
            if( i > thresh )
            {
                wfl[pos].mat().color( ((fadeChunk-(i-thresh))/fadeChunk) * color );
            }
            else
            {
                wfl[pos].mat().color( color );
            }
        }
    }
}

// map audio buffer to 3D positions
fun void map2waveform( float in[], float out[] )
{
    if( in.size()*3 != out.size() )
    {
        <<< "size mismatch in map2waveform()", "" >>>;
        return;
    }
    
    // mapping to xyz coordinate
    int i;
    DISPLAY_WIDTH => float width;
    for( auto s : in )
    {
        // map
        -width/2 + width/WINDOW_SIZE*i => out[i*3];
        // map y, using window function to taper the ends
        s*6 * window[i] => out[i*3+1];
        0 => out[i*3+2];
        // increment
        i++;
    }
}

// map FFT output to 3D positions
fun void map2spectrum( complex in[], float out[] )
{
    if( in.size()*3 != out.size() )
    {
        <<< "size mismatch in map2spectrum()", "" >>>;
        return;
    }
    
    // mapping to xyz coordinate
    int i;
    DISPLAY_WIDTH => float width;
    for( auto s : in )
    {
        // map
        -width/2 + width/WINDOW_SIZE*i => out[i*3];
        5 * Math.sqrt( (s$polar).mag * 25 ) => out[i*3+1];
        0 => out[i*3+2];
        // increment
        i++;
    }

    waterfall.latest( out );
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

// fps printer
fun void printFPS( dur howOften )
{
    while( true )
    {
        <<< "fps:", GG.fps() >>>;
        howOften => now;
    }
}
spork ~ printFPS(.25::second);

fun void controlSine( Osc s )
{
    while( true )
    {
        100 + (Math.sin(now/second*5)+1)/2*20000 => s.freq;
        10::ms => now;
    }
}
// spork ~ controlSine( sine );

// graphics render loop
while( true )
{
    // map to interleaved format
    map2waveform( samples, positions );
    // set the mesh position
    waveform.geo().positions( positions );
    // map to spectrum display
    map2spectrum( response, positions );
    // next graphics frame
    GG.nextFrame() => now;
}
