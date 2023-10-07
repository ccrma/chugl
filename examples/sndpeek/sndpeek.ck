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
// width of waveform and spectrum display
10 => float DISPLAY_WIDTH;

// uncomment to fullscreen
GG.fullscreen();
// put camera on a dolly
GG.camera() --> GGen dolly --> GG.scene();
// position
GG.camera().posZ( 10 );
// set bg color
GG.scene().backgroundColor( @(0,0,0) );

// waveform renderer
GLines waveform --> GG.scene(); waveform.mat().lineWidth(1.0);
// translate up
waveform.posY(WAVEFORM_Y);
// color0
waveform.mat().color( @(.4, .4, 1) );

// spectrum renderer
GLines spectrum --> GG.scene(); spectrum.mat().lineWidth(1.0);
// translate down
spectrum.posY(-WAVEFORM_Y);
// color0
spectrum.mat().color( @(.4, 1, .4) );

// accumulate samples from mic
adc => Flip accum => blackhole;
// take the FFT
adc => PoleZero dcbloke => FFT fft => blackhole;
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

// map audio buffer to 3D positions
fun void map2waveform( float in[], float out[] )
{
    if( in.size()*3 != out.size() )
    {
        <<< "size mismatch in flatten()", "" >>>;
        return;
    }
    
    // mapping to xyz coordinate
    int i;
    DISPLAY_WIDTH => float width;
    for( auto s : in )
    {
        // map
        -width/2 + width/WINDOW_SIZE*i => out[i*3];
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
        <<< "size mismatch in flatten()", "" >>>;
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

// graphics render loop
while( true )
{
    // map to interleaved format
    map2waveform( samples, positions );
    // set the mesh position
    (waveform.geo() $ CustomGeometry).positions( positions );
    // map to spectrum display
    map2spectrum( response, positions );
    // set the mesh position
    (spectrum.geo() $ CustomGeometry).positions( positions );
    // next graphics frame
    GG.nextFrame() => now;
}
