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
CglUpdate UpdateEvent;
CglFrame FrameEvent;
CglCamera mainCamera; mainCamera.SetPosition(3 * BACK);
CglScene scene;

BoxGeo boxGeo;
SphereGeo sphereGeo;
NormMat normMat;  

CglMesh waveformBoxMeshes[WAVEFORM_LENGTH];
CglMesh spectrumBoxMeshes[WAVEFORM_LENGTH];
50.0 / WAVEFORM_LENGTH => float boxScale;

// initialize boxes for waveform
for (0 => int i; i < WAVEFORM_LENGTH; i++) {
    waveformBoxMeshes[i].set(boxGeo, normMat);
    waveformBoxMeshes[i].SetScale(boxScale * UNIFORM);
    waveformBoxMeshes[i].SetPosition(((-WAVEFORM_LENGTH/2) + i) * RIGHT * boxScale);
    scene.AddChild(waveformBoxMeshes[i]);

    spectrumBoxMeshes[i].set(sphereGeo, normMat);  // TODO add different material for spectrum
    spectrumBoxMeshes[i].SetScale(boxScale * UNIFORM);
    spectrumBoxMeshes[i].SetPosition(((-WAVEFORM_LENGTH/2) + i) * RIGHT * boxScale + FORWARD);
    scene.AddChild(spectrumBoxMeshes[i]);
}

fun void UpdateVisualizer() {
    for (0 => int i; i < WAVEFORM_LENGTH; i++) {
        waveformBoxMeshes[i].PosY((5 * (waveform[i]))); // waveform

        // no interpolation
        // spectrumBoxMeshes[i].PosY((5 * Math.pow((spectrum[i]$polar).mag, .1))); // spectrum

        // add interpolation
        spectrumBoxMeshes[i].GetPosition() => vec3 spectrumBoxPos;
        5 * Math.pow((spectrum[i]$polar).mag, .1) => float spectrumBoxTargetY;
        spectrumBoxPos.y => float spectrumBoxCurrentY;
        0.14 => float interpSpeed;
        spectrumBoxMeshes[i].PosY(spectrumBoxCurrentY + (spectrumBoxTargetY - spectrumBoxCurrentY) * interpSpeed); // spectrum
    }
}


// flycamera controls
fun void cameraUpdate(time t, dur dt)
{
	// mouse lookaround
	.001 => float mouseSpeed;
	MM.GetDeltas() * mouseSpeed => vec3 mouseDeltas;

	// for mouse deltaY, rotate around GetRight axis
	mainCamera.RotateOnLocalAxis(RIGHT, -mouseDeltas.y);

	// for mouse deltaX, rotate around (0,1,0)
	mainCamera.RotateOnWorldAxis(UP, -mouseDeltas.x);

	2.5 * (dt / second) => float cameraSpeed;
	if (IM.isKeyDown(IM.KEY_LEFTSHIFT))
		2.5 *=> cameraSpeed;
	// camera movement
	if (IM.isKeyDown(IM.KEY_W))
		mainCamera.TranslateBy(cameraSpeed * mainCamera.GetForward());
	if (IM.isKeyDown(IM.KEY_S))
		mainCamera.TranslateBy(-cameraSpeed * mainCamera.GetForward());
	if (IM.isKeyDown(IM.KEY_D))
		mainCamera.TranslateBy(cameraSpeed * mainCamera.GetRight());
	if (IM.isKeyDown(IM.KEY_A))
		mainCamera.TranslateBy(-cameraSpeed * mainCamera.GetRight());
	if (IM.isKeyDown(IM.KEY_Q))
		mainCamera.TranslateBy(cameraSpeed * UP);
	if (IM.isKeyDown(IM.KEY_E))
		mainCamera.TranslateBy(-cameraSpeed * UP);

}


// Game loop 
fun void GameLoop(){
	CGL.Render(); // kick of the renderer
	while (true) {
		UpdateEvent => now; // will deadlock if UpdateEvent is broadcast before this shred begins waiting 

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
		CGL.Render();  // TODO: CGL.Render() should also block shred on UpdateEvent, to prevent deadlock
		// 17::ms => now;  // forces deadlock, bc of bug I have written about. with this delay, shred is not
		// getting on the UpdateEvent waitqueue in time before renderer broadcasts it.
		// solution is to somehow get on it before calling Render(), something like CGL.Render() => now;
	}
} 

GameLoop();