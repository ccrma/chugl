// Example of using CGL to visualize waveform and spectrum

// Managers ========================================================
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

// Globals ==============================================
class Globals {
	// static vars
	0 => static int frameCounter;
	now => static time lastTime;
	0::samp => static dur deltaTime;
    second / samp => float srate;


} Globals G;

@(0.0, 0.0, 0.0) => vec3 ORIGIN;
@(0.0, 1.0, 0.0) => vec3 UP;
@(0.0, -1.0, 0.0) => vec3 DOWN;
@(1.0, 0.0, 0.0) => vec3 RIGHT;
@(-1.0, 0.0, 0.0) => vec3 LEFT;
@(0.0, 0.0, -1.0) => vec3 FORWARD;  // openGL uses right-handed sytem
@(0.0, 0.0, 1.0) => vec3 BACK;
@(1.0, 1.0, 1.0) => vec3 UNIFORM;

fun string VecToString(vec3 v) {
	return v.x + "," + v.y + "," + v.z;
}

fun int VecEquals(vec3 a, vec3 b) {
	b - a => vec3 c;
	return c.magnitude() < .01;
}

// chuck ugen setup
// 512 => int WAVEFORM_LENGTH;
256 => int WAVEFORM_LENGTH;

adc => Gain g => dac;
dac => FFT fft => blackhole;
WAVEFORM_LENGTH * 2 => fft.size;

float waveform[WAVEFORM_LENGTH];
complex spectrum[WAVEFORM_LENGTH];
// waveform update
fun void WaveformWriter() {
    0 => int i;
    while (true) {
        dac.last() => waveform[i++];
        // g.last() => waveform[i++];
        if (i >= waveform.size()) { 0 => i; }
        1::samp => now;
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

// Scene Setup =============================================================
NextFrameEvent UpdateEvent;

GCamera mainCamera; mainCamera.pos(3 * BACK);
GScene scene;

BoxGeometry boxGeo;
SphereGeometry  SphereGeometry ;
NormalsMaterial normMat;  

GMesh waveformBoxMeshes[WAVEFORM_LENGTH];
GMesh spectrumBoxMeshes[WAVEFORM_LENGTH];
50.0 / WAVEFORM_LENGTH => float boxScale;

// initialize boxes for waveform
for (0 => int i; i < WAVEFORM_LENGTH; i++) {
    waveformBoxMeshes[i].set(boxGeo, normMat);
    waveformBoxMeshes[i].scale(boxScale * UNIFORM);
    waveformBoxMeshes[i].pos(((-WAVEFORM_LENGTH/2) + i) * RIGHT * boxScale);
    waveformBoxMeshes[i] --> scene;

    spectrumBoxMeshes[i].set(SphereGeometry , normMat);  // TODO add different material for spectrum
    spectrumBoxMeshes[i].scale(boxScale * UNIFORM);
    spectrumBoxMeshes[i].pos(((-WAVEFORM_LENGTH/2) + i) * RIGHT * boxScale + FORWARD);
    (spectrumBoxMeshes[i]) --> scene;
}

fun void UpdateVisualizer() {
    for (0 => int i; i < WAVEFORM_LENGTH; i++) {
        waveformBoxMeshes[i].posY((5 * (waveform[i]))); // waveform

        // no interpolation
        // spectrumBoxMeshes[i].posY((5 * Math.pow((spectrum[i]$polar).mag, .1))); // spectrum

        // add interpolation
        spectrumBoxMeshes[i].pos() => vec3 spectrumBoxPos;
        5 * Math.pow((spectrum[i]$polar).mag, .1) => float spectrumBoxTargetY;
        spectrumBoxPos.y => float spectrumBoxCurrentY;
        0.14 => float interpSpeed;
        spectrumBoxMeshes[i].posY(spectrumBoxCurrentY + (spectrumBoxTargetY - spectrumBoxCurrentY) * interpSpeed); // spectrum
    }
}


// flycamera controls
fun void cameraUpdate(time t, dur dt)
{
	// mouse lookaround
	.001 => float mouseSpeed;
	MM.GetDeltas() * mouseSpeed => vec3 mouseDeltas;

	// for mouse deltaY, rotate around right axis
	mainCamera.rotateOnLocalAxis(RIGHT, -mouseDeltas.y);

	// for mouse deltaX, rotate around (0,1,0)
	mainCamera.rotateOnWorldAxis(UP, -mouseDeltas.x);

	2.5 * (dt / second) => float cameraSpeed;
	if (IM.isKeyDown(IM.KEY_LEFTSHIFT))
		2.5 *=> cameraSpeed;
	// camera movement
	if (IM.isKeyDown(IM.KEY_W))
		mainCamera.translate(cameraSpeed * mainCamera.forward());
	if (IM.isKeyDown(IM.KEY_S))
		mainCamera.translate(-cameraSpeed * mainCamera.forward());
	if (IM.isKeyDown(IM.KEY_D))
		mainCamera.translate(cameraSpeed * mainCamera.right());
	if (IM.isKeyDown(IM.KEY_A))
		mainCamera.translate(-cameraSpeed * mainCamera.right());
	if (IM.isKeyDown(IM.KEY_Q))
		mainCamera.translate(cameraSpeed * UP);
	if (IM.isKeyDown(IM.KEY_E))
		mainCamera.translate(-cameraSpeed * UP);

}


// Game loop 
fun void GameLoop(){
	while (true) {

		// 70::ms => now;  // why does this not deadlock???
		// FrameEvent => now;
		<<< "==== update loop " >>>;
		1 +=> G.frameCounter;
		
		// compute timing
		now - G.lastTime => G.deltaTime;
		now => G.lastTime;

		// Update logic
		cameraUpdate(now, G.deltaTime);
        UpdateVisualizer();

		// End update, begin render
		GG.nextFrame() => now;  // TODO: GG.Render() should also block shred on UpdateEvent, to prevent deadlock
		// 17::ms => now;  // forces deadlock, bc of bug I have written about. with this delay, shred is not
		// getting on the UpdateEvent waitqueue in time before renderer broadcasts it.
		// solution is to somehow get on it before calling Render(), something like GG.Render() => now;
	}
} 

GameLoop();