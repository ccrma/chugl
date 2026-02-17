/*

pl_synth_song_t {
    int row_len; // length of 1 row in tracker in samples, at 44100 srate. 
    pl_synth_track[] tracks
}

pl_synth_track_t {
	pl_synth_t synth = {7,0,0,0,130,1,7,0,0,0,0,0,0,200,1103,4942,157,0,0,0,3,71,6,16,0,0,1,18},
    int[] sequence 
    pl_synth_pattern_t[] patterns
}

pl_synth_pattern_t {
	u8 notes[32]; // TODO what do note values correspond to?
} 

pl_synth_t {
	u8 osc0_oct;
	u8 osc0_det;
	u8 osc0_detune;
	u8 osc0_xenv;
	u8 osc0_vol;
	u8 osc0_waveform;

	u8 osc1_oct;
	u8 osc1_det;
	u8 osc1_detune;
	u8 osc1_xenv;
	u8 osc1_vol;
	u8 osc1_waveform;

	u8 noise_fader;

	u32 env_attack;
	u32 env_sustain;
	u32 env_release;
	u32 env_master;

	u8 fx_filter;
	u32 fx_freq;
	u8 fx_resonance;
	u8 fx_delay_time;
	u8 fx_delay_amt;
	u8 fx_pan_freq;
	u8 fx_pan_amt;

	u8 lfo_osc_freq;
	u8 lfo_fx_freq;
	u8 lfo_freq;
	u8 lfo_amt;
	u8 lfo_waveform;
} pl_synth_t;


((1/256) * powf(1.059463094, note - 128 + (oct - 8) * 12 + semi))

row_len to bpm formula:
- 15 * srate / row_len

pl_synth_pattern_t note to midi: 
- (note - 128) + ((oct - 8) * 12) + semi

PROBLEMS:
- chuck doesn't have a state-variable filter


*/

public class pl_synth extends Chugen {
    [new SinOsc, new PulseOsc, new SawOsc, new TriOsc] @=> UGen osc0_bank;
    [new SinOsc, new PulseOsc, new SawOsc, new TriOsc] @=> UGen osc1_bank;

    UGen@ osc0;
    UGen@ osc1;

	int osc0_oct;
	int osc0_semi;
    int osc0_detune;
	int osc0_xenv;
	int osc0_vol;
	int osc0_waveform;

	int osc1_oct;
	int osc1_semi;
	int osc1_detune;
	int osc1_xenv;
	int osc1_vol;
	int osc1_waveform;

	int noise_fader;

	int env_attack;
	int env_sustain;
	int env_release;
	int env_master;

	int fx_filter;
	int fx_freq;
	int fx_resonance;
	int fx_delay_time;
	int fx_delay_amt;
	int fx_pan_freq;
	int fx_pan_amt;

	int lfo_osc_freq;
	int lfo_fx_freq;
	int lfo_freq;
	int lfo_amt;
	int lfo_waveform;

    // call every row_len samples
    fun void pl_synth_gen(int row_len, int note) {
        Math.powf(2, fx_pan_freq - 8) / row_len => float fx_pan_freq;
        Math.powf(2, s->lfo_freq - 8) / row_len => float lfo_freq;
        
        // We need higher precision here, because the oscilator positions may be 
        // advanced by tiny values and error accumulates over time
        float osc0_pos;
        float osc1_pos;

        fx_resonance / 255.0 => float fx_resonance;

        float noise_vol = s->noise_fader * 4.6566129e-010f;  // TODO

        float low;
        float band;
        float high;

        float inv_attack = 1.0f / s->env_attack;
        float inv_release = 1.0f / s->env_release;
        float lfo_amt = s->lfo_amt / 512.0f;
        float pan_amt = s->fx_pan_amt / 512.0f;

        float osc0_freq = pl_synth_note_freq(note, s->osc0_oct, s->osc0_det, s->osc0_detune);
        float osc1_freq = pl_synth_note_freq(note, s->osc1_oct, s->osc1_det, s->osc1_detune);

        int num_samples = s->env_attack + s->env_sustain + s->env_release - 1;
        
        for (int j = num_samples; j >= 0; j--) {
            int k = j + write_pos;

            // LFO
            float lfor = PL_SYNTH_TAB(s->lfo_waveform, k * lfo_freq) * lfo_amt + 0.5f;

            float sample = 0;
            float filter_f = s->fx_freq;
            float temp_f;
            float envelope = 1;

            // Envelope
            if (j < s->env_attack) {
                envelope = (float)j * inv_attack;
            }
            else if (j >= s->env_attack + s->env_sustain) {
                envelope -= (float)(j - s->env_attack - s->env_sustain) * inv_release;
            }

            // Oscillator 1
            temp_f = osc0_freq;
            if (s->lfo_osc_freq) {
                temp_f *= lfor;
            }
            if (s->osc0_xenv) {
                temp_f *= envelope * envelope;
            }
            osc0_pos += temp_f;
            sample += PL_SYNTH_TAB(s->osc0_waveform, osc0_pos) * s->osc0_vol;

            // Oscillator 2
            temp_f = osc1_freq;
            if (s->osc1_xenv) {
                temp_f *= envelope * envelope;
            }
            osc1_pos += temp_f;
            sample += PL_SYNTH_TAB(s->osc1_waveform, osc1_pos) * s->osc1_vol;

            // Noise oscillator
            if (noise_vol > 0) {
                int32_t r = (int32_t)pl_synth_rand;
                sample += (float)r * noise_vol * envelope;
                pl_synth_rand ^= pl_synth_rand << 13;
                pl_synth_rand ^= pl_synth_rand >> 17;
                pl_synth_rand ^= pl_synth_rand << 5;
            }

            sample *= envelope * (1.0f / 255.0f);

            // State variable filter
            if (s->fx_filter) {
                if (s->lfo_fx_freq) {
                    filter_f *= lfor;
                }

                filter_f = PL_SYNTH_TAB(0, filter_f * (0.5f / PL_SYNTH_SAMPLERATE)) * 1.5f;
                low += filter_f * band;
                high = fx_resonance * (sample - band) - low;
                band += filter_f * high;
                sample = (float[5]){sample, high, low, band, low + high}[s->fx_filter];
            }

            // Panning & master volume
            temp_f = PL_SYNTH_TAB(0, k * fx_pan_freq) * pan_amt + 0.5f;
            sample *= 78 * s->env_master;


            samples[k * 2 + 0] += sample * (1-temp_f);
            samples[k * 2 + 1] += sample * temp_f;
        }
    }
}