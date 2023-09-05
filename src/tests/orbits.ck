/*
solar system example
tests scenegraph parent/child relations + local vs global transforms + rotation
*/

// initialize geos
SphereGeo sphereGeo;
BoxGeo boxGeo;
// init materials
NormMat normMat;
normMat.useLocal(1);

// scene setup
CglScene scene;
CglGroup sunSystem, earthSystem, moonSystem;
CglMesh  sun, earth, moon;

sun.set(boxGeo, normMat); 
earth.set(boxGeo, normMat);
moon.set(boxGeo, normMat);

earthSystem.SetPosition(@(2.2, 0.0, 0.0));
moonSystem.SetPosition(@(.55, 0.0, 0.0));

sun.SetScale(@(2.0, 2.0, 2.0));
earth.SetScale(@(0.4, 0.4, 0.4));
moon.SetScale(@(0.12, 0.12, 0.12));

// create graph
scene.AddChild(sunSystem);

sunSystem.AddChild(sun);
sunSystem.AddChild(earthSystem);

earthSystem.AddChild(earth);
earthSystem.AddChild(moonSystem);

moonSystem.AddChild(moon);


InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

CglUpdate UpdateEvent;
CglFrame FrameEvent;
CglCamera mainCamera;
0 => int frameCounter;
1 => int autoRender;
now => time lastTime;

mainCamera.SetPosition(@(0.0, 0.0, 3.0));

fun void Update(time t, dur dt) 
{
	t / second => float ftime;

	sunSystem.SetRotation(@(0.0, .5 * ftime, 0.0));
	earthSystem.SetRotation(@(0.0, .7 * ftime, 0.0));

	sun.SetRotation(@(0.0, .1 * ftime, 0.0));
	earth.SetRotation(@(0.0, .4 * ftime, 0.0));
	moon.SetRotation(@(0.0, .9 * ftime, 0.0));
}

fun void FreeUpdate() {
	while (10::ms => now) {
	}
} 
// spork ~ FreeUpdate();

// flycamera controls
@(0.0, 1.0, 0.0) => vec3 UP;
@(1.0, 0.0, 0.0) => vec3 RIGHT;
fun void cameraUpdate(time t, dur dt)
{
	2.5 * (dt / second) => float cameraSpeed;
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

	// mouse lookaround

	.001 => float mouseSpeed;
	MM.GetDeltas() * mouseSpeed => vec3 mouseDeltas;

	// <<< mouseDeltas >>>;

	// for mouse deltaY, rotate around GetRight axis
	mainCamera.RotateOnLocalAxis(RIGHT, -mouseDeltas.y);

	// for mouse deltaX, rotate around (0,1,0)
	mainCamera.RotateOnWorldAxis(UP, -mouseDeltas.x);
}


/*  wrap these into single event, good ergonomics
{
CGL.Render();  // tell renderer its safe to copy and draw
UpdateEvent => now;
}
*/
// Game loop 
CGL.Render(); // kick of the renderer
fun void GameLoop(){
	while (true) {
		// 10::ms => now;  // don't need events anymore!!!
		/*
		but still have problem: 
		if you are writing every event, command queue write rate will go out of sync with the
		renderer flush rate.
		E.g. say you want to write 2 commands every frame, on FrameEvent => now;
		sometimes the renderer will see 2 commands, somestimes 0, sometimes 4
			- results occassional stuttering :(
		The full work-around is to have the conditional_var sync mechanism + command queue
		but then this prevents us from writing CGL commands freely in chuck side

		maybe best case is to use command queue + conditional sync var, but 
		let the chuck VM take care of syncing? how would this work?

		*/
		UpdateEvent => now;
		// FrameEvent => now;
		frameCounter++;
		
		// compute timing
		now - lastTime => dur deltaTime;
		now => lastTime;

		// Update logic
		cameraUpdate(now, deltaTime);
		Update(now, deltaTime);

		// End update, begin render
		if (autoRender) { CGL.Render(); } // tell renderer its safe to copy and draw
	}
} spork ~ GameLoop();

// TODO: create Entity-component-system abstraction and move update logic into components
fun void TriggerRenderListener() {
	while (true) {
		IM.keyDownEvent(IM.KEY_SPACE) => now;
		CGL.Render();
	}
} spork ~ TriggerRenderListener();

fun void ToggleRenderListener() {
	while (true) {
		IM.keyDownEvent(IM.KEY_Z) => now;
		1 - autoRender => autoRender;
		CGL.Render();
	}
} spork ~ ToggleRenderListener();

while (true) {
	1::second => now;
}
