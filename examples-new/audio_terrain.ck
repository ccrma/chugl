// window size
1024 => int WINDOW_SIZE;
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
// mapped FFT response
float spectrum[WINDOW_SIZE];

// do audio stuff
fun void doAudio()
{
    while( true )
    {
        // upchuck to process accum
        accum.upchuck();
        // get the last window size samples (waveform)
        accum.output( samples );
        // upchuck to take FFT, get magnitude response
        fft.upchuck();
        // get spectrum (as complex values)
        fft.spectrum( response );
        // jump by samples
        WINDOW_SIZE::samp/2 => now;
    }
}
spork ~ doAudio();

// map FFT output to 3D positions
fun void map2spectrum( complex in[], float out[] )
{
    if( in.size() != out.size() )
    {
        <<< "size mismatch in map2spectrum()", "" >>>;
        return;
    }
    
    // mapping to scalar value
    for (int i; i < in.size(); i++)
    {
        // map frequency bin magnitude
        25 * Math.sqrt( (in[i]$polar).mag ) => out[i].y;
    }
}

Texture spectrum_texture(Texture.Dimension_2D, Texture.Format_R32Float);

while (true) {
    GG.nextFrame() => now;
    // map FFT response to scalar values
    map2spectrum( response, spectrum );
    // write to texture

}