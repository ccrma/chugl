// Example using Audio-reactive shaders with custom shader materials

// Audio ============================================================
512 => int WAVEFORM_LENGTH;  // openGL textures must be power of 2

adc => Gain g => dac;        // mic input
dac => FFT fft => blackhole;
WAVEFORM_LENGTH * 2 => fft.size;

float waveform[WAVEFORM_LENGTH];
complex spectrum[WAVEFORM_LENGTH];
float amplitudeHistory[WAVEFORM_LENGTH];

// buffer waveform samples
fun void WaveformWriter() {
    0 => int i;
    while (1::samp => now) {
        Math.remap(dac.last(), -1.0, 1.0, 0.0, 1.0) => waveform[i++];
        if (i >= waveform.size()) { 0 => i; }
    }
} 
spork ~ WaveformWriter();

// buffer fft values
fun void SpectrumWriter() {
    while (WAVEFORM_LENGTH::samp => now) {
        // take fft
        fft.upchuck();
        // get contents
        fft.spectrum( spectrum );
        // <<< spectrum[0]$polar, spectrum[50]$polar, spectrum[100]$polar, spectrum[150]$polar >>>;
    }
}
spork ~ SpectrumWriter();

// buffer amplitude history
0 => int amp_index;
fun void AmplitudeWriter() {
    // patch
    dac => Gain amp_g => OnePole p => blackhole;
    // square the input
    dac => amp_g;
    // multiply
    3 => amp_g.op;

    // set filter pole position (between 0 and 1)
    // NOTE: this controls how smooth the output is
    // closer to 1 == smoother but less responsive
    // closer to 0 == more jumpy but also more responsive
    0.999 => p.pole;

    while( WAVEFORM_LENGTH::samp => now ) {
        Math.min(Math.pow(p.last(), .3), 1.0) => float amp;
        amp => amplitudeHistory[amp_index++];
        if (amp_index >= amplitudeHistory.size()) { 0 => amp_index; }
    }

} spork ~ AmplitudeWriter();

fun void fpsPrinter(dur d) {
    while (d => now)
        <<< GG.fps() >>>;
}
spork ~ fpsPrinter(1::second);

// Graphics =============================================================

// position camera
7 => GG.camera().posZ;

// set number of meshes in x and y directions
3 => int NUM_MESHES_X;
3 => int NUM_MESHES_Y;
NUM_MESHES_X * NUM_MESHES_Y => int NUM_MESHES;

// setup geometry and materials
BoxGeometry boxGeo;
ShaderMaterial shaderMat; // custom shader material
shaderMat.fragShader(
    me.dir() + "AudioFrag.glsl"  // path to fragment shader
);
DataTexture tex;

// create meshes
GMesh meshes[NUM_MESHES];

// setup meshes
for ( 0 => int i; i < NUM_MESHES_X; i++) {
    for ( 0 => int j; j < NUM_MESHES_Y; j++) {
        i * NUM_MESHES_Y + j => int index;
        meshes[index] @=> GMesh @ mesh;
        mesh.set( boxGeo, shaderMat );
        mesh --> GG.scene();
        // center position

        mesh.pos(2.0 * @((i - NUM_MESHES_X / 2), (j - NUM_MESHES_Y / 2), 0));
    }
}

// update shader uniform
shaderMat.uniformTexture("u_AudioTexture", tex);

// update the data texture with audio data
// note: each pixel in the texture is 4 bytes, so the data array needs to be 4x the length of the waveform
// and every 4 values in the array correspond to a single pixel's RGBA values
float audioTextureData[WAVEFORM_LENGTH * 4]; // 4 channels
fun void UpdateAudioTexture() {
    // currently CGL textures only support unsigned bytes
    // so values need to be remapped into range [0, 255]
    for (0 => int i; i < WAVEFORM_LENGTH; i++) {
        Math.pow((spectrum[i]$polar).mag, .25) => float scaledSpectrum;
        scaledSpectrum * 255      => audioTextureData[i * 4 + 0];       // fft
        waveform[i] * 255         => audioTextureData[i * 4 + 1];       // waveform
        amplitudeHistory[i] * 255 => audioTextureData[i * 4 + 2];       // amplitude history
        255                       => audioTextureData[i * 4 + 3];       // unused
    }

    // <<< spectrum[0]$polar, spectrum[50]$polar, spectrum[100]$polar, spectrum[150]$polar >>>;

    // pass into texture with dimensions WAVEFORM_LENGTH x 1
    tex.data(audioTextureData, WAVEFORM_LENGTH, 1);
    // tell shader where we are in the amplitude history buffer (for drawing red line)
    shaderMat.uniformInt("u_AmplitudeIndex", amp_index);
}

// Game loop  =============================================================
while (true) {
    GG.dt() => float dt;
    // Update logic
    UpdateAudioTexture();

    for ( GMesh @ mesh : meshes ) {
        mesh.rotateX( .27 * dt );
        mesh.rotateY( .15 * dt );
    }

    // End update, begin render
    GG.nextFrame() => now;
}