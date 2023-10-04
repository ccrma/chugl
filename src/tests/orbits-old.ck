/*
solar system example
tests scenegraph parent/child relations + local vs global transforms + rotation
*/

// initialize geos
SphereGeometry  SphereGeometry ;
BoxGeometry boxGeo;
// init materials
NormMat normMat;
normMat.useLocal(1);

NormMat worldspaceNormMat;
worldspaceNormMat.useLocal(0);

// scene setup
GScene scene;
CglGroup sunSystem, earthSystem, moonSystem;
GMesh  sun, earth, moon;

sun.set(boxGeo, normMat); 
earth.set(boxGeo, worldspaceNormMat);
moon.set(boxGeo, normMat);

earthSystem.position(@(2.2, 0.0, 0.0));
moonSystem.position(@(.55, 0.0, 0.0));

sun.scale(@(2.0, 2.0, 2.0));
earth.scale(@(0.4, 0.4, 0.4));
moon.scale(@(0.12, 0.12, 0.12));

moonSystem --> earthSystem --> sunSystem --> scene;
sun --> sunSystem;
earth --> earthSystem;
moon --> moonSystem;

InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

NextFrameEvent UpdateEvent;

GCamera mainCamera;
0 => int frameCounter;
1 => int autoRender;
now => time lastTime;

mainCamera.position(@(0.0, 0.0, 3.0));


fun void grucker() {
	while (true) {
		earthSystem --> sunSystem;
		1::second => now;
		earthSystem --< sunSystem;
		1::second => now;
	}
}
spork ~ grucker();


fun void Update(time t, dur dt) 
{
	t / second => float ftime;

	sunSystem.rotation(@(0.0, .5 * ftime, 0.0));
	earthSystem.rotation(@(0.0, .7 * ftime, 0.0));

	sun.rotation(@(0.0, .1 * ftime, 0.0));
	earth.rotation(@(0.0, .4 * ftime, 0.0));
	moon.rotation(@(0.0, .9 * ftime, 0.0));

	// <<< "sun pos", sun.GetRotation() >>>;
	// <<< "moon pos", moon.GetWorldPosition() >>>;
}

// flycamera controls
@(0.0, 1.0, 0.0) => vec3 UP;
@(1.0, 0.0, 0.0) => vec3 RIGHT;
fun void cameraUpdate(time t, dur dt)
{
	2.5 * (dt / second) => float cameraSpeed;
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

	// <<< "pos", mainCamera.pos() >>>;
	// <<< "rot", mainCamera.GetRotation() >>>;

	// mouse lookaround

	.001 => float mouseSpeed;
	MM.GetDeltas() * mouseSpeed => vec3 mouseDeltas;

	// <<< mouseDeltas >>>;

	// for mouse deltaY, rotate around right axis
	mainCamera.rotateOnLocalAxis(RIGHT, -mouseDeltas.y);

	// for mouse deltaX, rotate around (0,1,0)
	mainCamera.rotateOnWorldAxis(UP, -mouseDeltas.x);
}


/*  wrap these into single event, good ergonomics
{
GG.Render();  // tell renderer its safe to copy and draw
UpdateEvent => now;
}
*/
// Game loop 
// GG.Render(); // kick of the renderer
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
		// UpdateEvent => now;
		// FrameEvent => now;
		frameCounter++;
		
		// compute timing
		now - lastTime => dur deltaTime;
		now => lastTime;

		// Update logic
		cameraUpdate(now, deltaTime);
		Update(now, deltaTime);

		// <<< "inside chuck framecount: " + frameCounter >>>;

		// End update, begin render
		// GG.Render();
		// UpdateEvent => now;
		GG.nextFrame() => now;
		// if (autoRender) { GG.Render(); } // tell renderer its safe to copy and draw
	}
} spork ~ GameLoop();


while (true) {
	1::second => now;
}
