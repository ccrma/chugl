// Example using CGL Audio-reactive shaders with custom shader materials

// Managers ========================================================
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

GCamera mainCamera;
FlyCam flycam;
flycam.init(IM, MM, mainCamera);
spork ~ flycam.selfUpdate();

GG.fullscreen();
GG.lockCursor();
mainCamera.clip(0.1, 1000);


// Audio ============================================================
512 => int WAVEFORM_LENGTH;  // openGL textures must be power of 2

adc => Gain g => dac;        // mic input
dac => FFT fft => blackhole;
WAVEFORM_LENGTH * 2 => fft.size;

float waveform[WAVEFORM_LENGTH];
complex spectrum[WAVEFORM_LENGTH];
float amplitudeHistory[WAVEFORM_LENGTH];
// waveform update
fun void WaveformWriter() {
    0 => int i;
    while (1::samp => now) {
        Math.remap(dac.last(), -1.0, 1.0, 0.0, 1.0) => waveform[i++];
        if (i >= waveform.size()) { 0 => i; }
    }
} 
spork ~ WaveformWriter();

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

// Scene Setup =============================================================
class Globals {
	// static vars
	0 => static int frameCounter;
	now => static time lastTime;
	0::samp => static dur deltaTime;
    second / samp => float srate;
} Globals G;

GScene scene;
BoxGeometry boxGeo;
1000 => int NUM_MESHES;
GMesh meshes[NUM_MESHES];
ShaderMaterial shaderMat; // custom shader material
shaderMat.shaders(
    "renderer/shaders/BasicLightingVert.glsl",
    "renderer/shaders/AudioFrag.glsl"
);
DataTexture tex;

// setup meshes
for ( 0 => int i; i < NUM_MESHES/10; i++) {
    for ( 0 => int j; j < 10; j++) {
        10 * i + j => int index;
        meshes[index] @=> GMesh @ mesh;
        mesh.set( boxGeo, shaderMat );
        mesh --> scene;
        mesh.pos(@(2*i, 2*j, 0));
    }
}

// update shader uniform
shaderMat.uniformTexture("u_AudioTexture", tex);

float audioTextureData[WAVEFORM_LENGTH * 4]; // 4 channels
fun void UpdateAudioTexture() {
    // currently CGL textures only support unsigned bytes
    // so values need to be remapped into range [0, 255]
    for (0 => int i; i < WAVEFORM_LENGTH; i++) {
        Math.pow((spectrum[i]$polar).mag, .25) => float scaledSpectrum;
        scaledSpectrum * 255 => audioTextureData[i * 4 + 0];       // fft
        waveform[i] * 255 =>    audioTextureData[i * 4 + 1];       // waveform
        amplitudeHistory[i] * 255 => audioTextureData[i * 4 + 2];  // amplitude history
        255 =>                  audioTextureData[i * 4 + 3];       // unused
    }

    // <<< spectrum[0]$polar, spectrum[50]$polar, spectrum[100]$polar, spectrum[150]$polar >>>;

    // pass into texture
    tex.data(audioTextureData, WAVEFORM_LENGTH, 1);
    shaderMat.uniformInt("u_AmplitudeIndex", amp_index);
}

    


// Game loop  =============================================================
fun void GameLoop(){
	while (true) {

		1 +=> G.frameCounter;
		
		// compute timing
		now - G.lastTime => G.deltaTime;
		now => G.lastTime;
        G.deltaTime/second => float dt;

		// Update logic
        // flycam.update(now, G.deltaTime);
        flycam.update(GG.dt());  // kind of looks smoother?
        UpdateAudioTexture();

        for ( GMesh @ mesh : meshes ) {
            mesh.rotateX( .27 * dt );
            mesh.rotateY( .15 * dt );
        }

		// End update, begin render
		GG.nextFrame() => now;
	}
} 

GameLoop();