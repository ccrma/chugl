//-----------------------------------------------------------------------------
// name: spectrum-interp.ck
// desc: spectrum visualization with interpolation
//
// pressing "1" in keyboard => no interpolation
// pressing "2" in keyboard => yes interpolation
// pressing "3" in keyboard => different interpolation values based on direction
// 
// author: Kunwoo Kim (kunwoo@ccrma.stanford.edu)
//   date: Fall 2024
//-----------------------------------------------------------------------------

// window size
1024 => int WINDOW_SIZE;
// y position of waveform
0 => float WAVEFORM_Y;
// width of waveform and spectrum display
6 => float DISPLAY_WIDTH;
// number of frequency bins
12 => int NUM_SPECTRUMS;
// only use the following % of spectrum
0.1 => float MAX_FREQUENCY;
// mode change for interpolation
0 => int MODE;

// window title
GWindow.title( "spectrum interpolation" );
// uncomment to fullscreen
GWindow.fullscreen();
// position camera
GG.scene().camera().posZ(8.0);

// spectrum renderer
GPlane spectrum[NUM_SPECTRUMS];
// initialize spectrum
for (int i; i < NUM_SPECTRUMS; i++)
{
    spectrum[i] --> GG.scene();
    -DISPLAY_WIDTH/2 + i * DISPLAY_WIDTH / (NUM_SPECTRUMS - 1) => spectrum[i].posX;
    spectrum[i].posY(-WAVEFORM_Y);
    spectrum[i].color( @(.4, 1, .4) );
    spectrum[i].scaX(0.2);
}

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
float magnitude[NUM_SPECTRUMS];

// *interpolation stuff*
// percentage of interpolation for all interpolation
0.05 => float interp;
// customized percentage of interpolation for either directions
0.3 => float positive_interp;
0.05 => float negative_interp;
// float array to store previous magnitude
float prev_magnitude[NUM_SPECTRUMS];

// UI for playing around with the code
UI_Float ui_interpolate(interp);
UI_Float ui_positive_interpolate(positive_interp);
UI_Float ui_negative_interpolate(negative_interp);

fun void interp_UI()
{
    while (true) {
        GG.nextFrame() => now;
        UI.setNextWindowSize(@(400, 400), UI_Cond.Once);
        if (UI.begin("Interpolate Value", null, 0)) {
            if (UI.slider("interpolate %", ui_interpolate, 0.0, 1.0)) {
                ui_interpolate.val() => interp;
            }
            if (UI.slider("positive interpolate %", ui_positive_interpolate, 0.0, 1.0)) {
                ui_positive_interpolate.val() => positive_interp;
            }
            if (UI.slider("negative interpolate %", ui_negative_interpolate, 0.0, 1.0)) {
                ui_negative_interpolate.val() => negative_interp;
            }
        }
        UI.end();
    }
}
spork ~ interp_UI();

// map FFT output to 3D positions
fun void map2spectrum( complex in[] )
{
    // average spectrum strength
    (in.size() * MAX_FREQUENCY)$int / NUM_SPECTRUMS => int numBins;
    
    for (int i; i < NUM_SPECTRUMS; i++)
    {
        for (i * numBins => int j; j < (i + 1) * numBins; j++)
        {
            25 * Math.sqrt( (in[j]$polar).mag ) +=> magnitude[i];
        }
        // average magnitude
        magnitude[i] / NUM_SPECTRUMS => magnitude[i];
        
        // move spectrum
        magnitude[i] => spectrum[i].scaY;
        -0.5 + magnitude[i] / 2 => spectrum[i].posY;
    }
}

// interpolation added
fun void map2spectrum_interpolated( complex in[] )
{
    // average spectrum strength
    (in.size() * MAX_FREQUENCY)$int / NUM_SPECTRUMS => int numBins;
    
    // this version uses one interpolation value for both positive and negative
    for (int i; i < NUM_SPECTRUMS; i++)
    {
        for (i * numBins => int j; j < (i + 1) * numBins; j++)
        {
            25 * Math.sqrt( (in[j]$polar).mag ) +=> magnitude[i];
        }
        // average magnitude
        magnitude[i] / NUM_SPECTRUMS => magnitude[i];
        
        // interpolation
        prev_magnitude[i] + (magnitude[i] - prev_magnitude[i]) * interp => magnitude[i];
        
        // move spectrum
        magnitude[i] => spectrum[i].scaY;
        -0.5 + magnitude[i] / 2 => spectrum[i].posY;
        
        // update spectrum for next interpolation
        magnitude[i] => prev_magnitude[i];
    }
}

// different values added for interpolation.
fun void map2spectrum_direction_interpolated( complex in[] )
{
    // average spectrum strength
    (in.size() * MAX_FREQUENCY)$int / NUM_SPECTRUMS => int numBins;
    
    // this version uses different interpolation values for positive and negative
    for (int i; i < NUM_SPECTRUMS; i++)
    {
        for (i * numBins => int j; j < (i + 1) * numBins; j++)
        {
            25 * Math.sqrt( (in[j]$polar).mag ) +=> magnitude[i];
        }
        // average magnitude
        magnitude[i] / NUM_SPECTRUMS => magnitude[i];
        
        // if previous one was larger (negative interpolation)
        if (prev_magnitude[i] > magnitude[i])
        {
            prev_magnitude[i] + (magnitude[i] - prev_magnitude[i]) * negative_interp => magnitude[i];
        }
        else
        {
            prev_magnitude[i] + (magnitude[i] - prev_magnitude[i]) * positive_interp => magnitude[i];
        }
        
        
        // move spectrum
        magnitude[i] => spectrum[i].scaY;
        -0.5 + magnitude[i] / 2 => spectrum[i].posY;
        
        // update spectrum
        magnitude[i] => prev_magnitude[i];
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
        // upchuck to take FFT, get magnitude response
        fft.upchuck();
        // get spectrum (as complex values)
        fft.spectrum( response );
        // jump by samples
        WINDOW_SIZE::samp/2 => now;
    }
}
spork ~ doAudio();

// KEYBOARD STUFF
fun void modeChange()
{
    while (true)
    {
        if (GWindow.key(GWindow.Key_1))
            0 => MODE;
        else if (GWindow.key(GWindow.Key_2))
            1 => MODE;
        else if (GWindow.key(GWindow.Key_3))
            2 => MODE;
        else if (GWindow.key(GWindow.Key_F))
            GWindow.fullscreen();
        else if (GWindow.key(GWindow.Key_G))
            GWindow.windowed();
        10::ms => now;
    }
}
spork ~ modeChange();

GText text_spectrum --> GG.scene(); text_spectrum.posY(-0.8); text_spectrum.sca(0.2);
GText text_title --> GG.scene(); text_title.posY(-1.75); text_title.sca(0.2); 
GText text_instructions1 --> GG.scene(); text_instructions1.posY(-2.0); text_instructions1.posX(-2); text_instructions1.sca(0.1); text_instructions1.controlPoints(@(0.0, 0.5));
GText text_instructions2 --> GG.scene(); text_instructions2.posY(-2.15); text_instructions2.posX(-2); text_instructions2.sca(0.1); text_instructions2.controlPoints(@(0.0, 0.5));
GText text_instructions3 --> GG.scene(); text_instructions3.posY(-2.3); text_instructions3.posX(-2); text_instructions3.sca(0.1); text_instructions3.controlPoints(@(0.0, 0.5));
"Spectrum" => text_spectrum.text;
"<Zenos Interpolation Demo>" => text_title.text;
"* press 1: no interpolation" => text_instructions1.text;
"* press 2: yes interpolation - play with 'interpolate %'" => text_instructions2.text;
"* press 3: two-way interpolation - play with 'positive/negative %'" => text_instructions3.text;

// graphics render loop
while( true )
{
    // next graphics frame
    GG.nextFrame() => now;
    if (MODE == 0)
       map2spectrum( response );
    else if (MODE == 1)
       map2spectrum_interpolated( response );
    else if (MODE == 2)
       map2spectrum_direction_interpolated( response );
}
