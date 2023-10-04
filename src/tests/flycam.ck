public class FlyCam {

	GCamera @ mainCamera;
	InputManager @ IM;
	MouseManager @ MM;

	fun void init(InputManager @ im, MouseManager @ mm, GCamera @ cam) {
		im @=> this.IM;
		mm @=> this.MM;
		cam @=> this.mainCamera;
	}

	fun void init(InputManager @ im, MouseManager @ mm) {
		im @=> this.IM;
		mm @=> this.MM;
		GG.camera() @=> this.mainCamera;
	}

	@(0.0, 1.0, 0.0) => vec3 UP;
	@(1.0, 0.0, 0.0) => vec3 RIGHT;
	fun void update(time t, dur dt)
	{
		this.update(dt/second);
	}

	fun void update(float dt) {
		2.5 * dt => float cameraSpeed;
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

	fun void selfUpdate() {
		<<< "flycam: selfUpdate mode" >>>;
		now => time lastTime;
		NextFrameEvent UpdateEvent;
		while (true) {
			// time bookkeeping
			now - lastTime => dur deltaTime;
			now => lastTime;

			update(now, deltaTime);
			// update(GG.dt());

			// signal ready for next update
			// UpdateEvent => now;
			GG.nextFrame() => now;
		}
	}

}