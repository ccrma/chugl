//-----------------------------------------------------------------------------
// name: FlyCam.ck
// desc: a fly cam
// requires: KB.ck Mouse.ck
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//-----------------------------------------------------------------------------
public class FlyCam
{
    // camera reference
	GCamera @ mainCamera;
    // our mouse for deltas
	Mouse MM;
    // up vector
	@(0.0, 1.0, 0.0) => vec3 UP;
    // right vector
	@(1.0, 0.0, 0.0) => vec3 RIGHT;

    // initialize with mouse number; keyboard is handled by KB
    fun void init( int whichMouse )
    {
        spork ~ MM.start(whichMouse);
        GG.camera() @=> this.mainCamera;
    }

    // init with mouse number and a specific camera
	fun void init( int whichMouse, GCamera @ cam)
    {
        spork ~ MM.start(whichMouse);
		cam @=> this.mainCamera;
	}

	fun void update( float dt )
    {
		2.5 * dt => float cameraSpeed;
		// camera movement
		if (KB.isKeyDown(KB.KEY_W))
			mainCamera.translate(cameraSpeed * mainCamera.forward());
		if (KB.isKeyDown(KB.KEY_S))
			mainCamera.translate(-cameraSpeed * mainCamera.forward());
		if (KB.isKeyDown(KB.KEY_D))
			mainCamera.translate(cameraSpeed * mainCamera.right());
		if (KB.isKeyDown(KB.KEY_A))
			mainCamera.translate(-cameraSpeed * mainCamera.right());
		if (KB.isKeyDown(KB.KEY_Q))
			mainCamera.translate(cameraSpeed * UP);
		if (KB.isKeyDown(KB.KEY_E))
			mainCamera.translate(-cameraSpeed * UP);

		// <<< "pos", mainCamera.pos() >>>;
		// <<< "rot", mainCamera.GetRotation() >>>;
		// mouse lookaround
        .001 => float mouseSpeed;
		MM.deltas() * mouseSpeed => vec3 mouseDeltas;

		// <<< mouseDeltas >>>;
		// for mouse deltaY, rotate around right axis
		mainCamera.rotateOnLocalAxis(RIGHT, -mouseDeltas.y);
		// for mouse deltaX, rotate around (0,1,0)
		mainCamera.rotateOnWorldAxis(UP, -mouseDeltas.x);
	}

    // <<< "flycam: selfUpdate mode" >>>;
    fun void selfUpdate()
    {
		now => time lastTime;
		NextFrameEvent UpdateEvent;
		while (true)
		{
			// time bookkeeping
			now - lastTime => dur deltaTime;
			now => lastTime;
			update(deltaTime / second);
			GG.nextFrame() => now;
		}
	}
}
