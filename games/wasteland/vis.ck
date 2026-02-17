// TODO:
// add waveform history
// add spectrum history
// for bytebeat: visualize the bits!!

public class Vis {
    512 => int WINDOW_SIZE;
    8 => int WATERFALL_DEPTH;
    PoleZero dcbloke => FFT fft => blackhole;
    .95 => dcbloke.blockZero;

    Windowing.hann(WINDOW_SIZE) => fft.window;
    WINDOW_SIZE*2 => fft.size;

    complex response[0];  // spectrum
    float spectrum_hist_positions[WINDOW_SIZE]; // remapped spectrum

    int sample_accum_w_idx;
    UI_Int waveform_w_pixels(512);
    UI_Float window_dur_sec(1.0/60);
    float sample_accum[waveform_w_pixels.val()];

    UGen@ u;

    int paused;

    // =============================

    fun @construct( UGen u ) { 
        u => dcbloke;
        u => dac;
        u @=> this.u;
    }

    // map FFT output
    fun void map2spectrum( complex in[], float out[] )
    {
        for(int i; i < in.size(); i++)
            5 * Math.sqrt( (in[i]$polar).mag * 25 ) => out[i];
    }

    fun void waveformShred() {
        while (1) {
            if (!paused) {
                this.u.last() => sample_accum[sample_accum_w_idx++ % sample_accum.size()];
            }
            (window_dur_sec.val() / waveform_w_pixels.val())::second => now;
        }
    } spork ~ waveformShred();

    fun void audioAnalyzerShred()
    {
        while( true ) {
            if (!paused) {
                fft.upchuck();
                fft.spectrum( response );
            }
            WINDOW_SIZE::samp => now;
        }
    } spork ~ audioAnalyzerShred();

    fun void run() {
        while (1) {
            GG.nextFrame() => now;

            UI.begin("");
            // UI.plotLines("##Waveform", samples);
            UI.plotLines(
                "##Waveform", sample_accum, 0, 
                "Waveform", -1, 1, 
                @(waveform_w_pixels.val(), 200)
                // @(1000 * window_dur_sec, 200)
            );

            UI.sameLine();
            UI.vslider("Time##Time", @(50, 200), window_dur_sec, .01, 2);
            UI.sameLine();
            if (UI.vslider("Size##Size", @(50, 200), waveform_w_pixels, 50, 1000)) {
                sample_accum.clear();
                waveform_w_pixels.val() => sample_accum.size;
            }


            UI.dummy(@(0.0f, 20.0f)); // vertical spacing

            // plot spectrum as histogram
            map2spectrum( response, spectrum_hist_positions );
            UI.plotHistogram(
                "##Spectrum", spectrum_hist_positions, 0, "512 bins", 0, 8, @(0, 200)
            );

            if (UI.button(paused ? "play" : "pause")) {
                !paused => paused;
            }

            UI.end();
        }
    }
}

